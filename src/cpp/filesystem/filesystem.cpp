#include "filesystem.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <cstring>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

namespace py = pybind11;
namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Utility helpers
// ---------------------------------------------------------------------------

static std::string human_readable_size(uintmax_t bytes) {
    const char* units[] = {"B", "K", "M", "G", "T", "P"};
    int i = 0;
    double size = static_cast<double>(bytes);
    while (size >= 1024.0 && i < 5) {
        size /= 1024.0;
        i++;
    }
    char buf[64];
    if (i == 0)
        snprintf(buf, sizeof(buf), "%ju", (uintmax_t)bytes);
    else
        snprintf(buf, sizeof(buf), "%.1f%s", size, units[i]);
    return std::string(buf);
}

static std::string permissions_string(fs::perms p) {
    std::string s;
    s += ((p & fs::perms::owner_read)   != fs::perms::none ? "r" : "-");
    s += ((p & fs::perms::owner_write)  != fs::perms::none ? "w" : "-");
    s += ((p & fs::perms::owner_exec)   != fs::perms::none ? "x" : "-");
    s += ((p & fs::perms::group_read)   != fs::perms::none ? "r" : "-");
    s += ((p & fs::perms::group_write)  != fs::perms::none ? "w" : "-");
    s += ((p & fs::perms::group_exec)   != fs::perms::none ? "x" : "-");
    s += ((p & fs::perms::others_read)  != fs::perms::none ? "r" : "-");
    s += ((p & fs::perms::others_write) != fs::perms::none ? "w" : "-");
    s += ((p & fs::perms::others_exec)  != fs::perms::none ? "x" : "-");
    return s;
}

static std::string format_time(fs::file_time_type ftime) {
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
    std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&tt));
    return std::string(buf);
}

static std::string get_owner(const fs::path& p) {
    struct stat st;
    if (stat(p.c_str(), &st) == 0) {
        struct passwd* pw = getpwuid(st.st_uid);
        return pw ? pw->pw_name : std::to_string(st.st_uid);
    }
    return "?";
}

static std::string get_group(const fs::path& p) {
    struct stat st;
    if (stat(p.c_str(), &st) == 0) {
        struct group* gr = getgrgid(st.st_gid);
        return gr ? gr->gr_name : std::to_string(st.st_gid);
    }
    return "?";
}

static std::string file_type_char(const fs::directory_entry& e) {
    if (e.is_symlink()) return "l";
    if (e.is_directory()) return "d";
    if (e.is_block_file()) return "b";
    if (e.is_character_file()) return "c";
    if (e.is_fifo()) return "p";
    if (e.is_socket()) return "s";
    return "-";
}

// ---------------------------------------------------------------------------
// ls — List directory contents
// ---------------------------------------------------------------------------

static py::list ls_impl(const std::string& path,
                         bool all,
                         bool long_format,
                         bool recursive,
                         const std::string& sort_by,
                         bool reverse,
                         bool human_readable,
                         bool directory_only) {
    fs::path dir_path(path);
    if (!fs::exists(dir_path))
        throw py::value_error("ls: cannot access '" + path + "': No such file or directory");

    std::vector<fs::directory_entry> entries;

    auto collect = [&](auto& iterator) {
        for (const auto& entry : iterator) {
            std::string name = entry.path().filename().string();
            if (!all && !name.empty() && name[0] == '.')
                continue;
            if (directory_only && !entry.is_directory())
                continue;
            entries.push_back(entry);
        }
    };

    if (recursive) {
        auto it = fs::recursive_directory_iterator(
            dir_path, fs::directory_options::skip_permission_denied);
        collect(it);
    } else {
        auto it = fs::directory_iterator(
            dir_path, fs::directory_options::skip_permission_denied);
        collect(it);
    }

    // Sort
    if (sort_by == "name") {
        std::sort(entries.begin(), entries.end(),
                  [](const auto& a, const auto& b) {
                      return a.path().filename() < b.path().filename();
                  });
    } else if (sort_by == "size") {
        std::sort(entries.begin(), entries.end(),
                  [](const auto& a, const auto& b) {
                      uintmax_t sa = a.is_regular_file() ? a.file_size() : 0;
                      uintmax_t sb = b.is_regular_file() ? b.file_size() : 0;
                      return sa < sb;
                  });
    } else if (sort_by == "time") {
        std::sort(entries.begin(), entries.end(),
                  [](const auto& a, const auto& b) {
                      return a.last_write_time() < b.last_write_time();
                  });
    }

    if (reverse)
        std::reverse(entries.begin(), entries.end());

    py::list result;

    for (const auto& entry : entries) {
        if (long_format) {
            py::dict info;
            info["name"]          = entry.path().filename().string();
            info["path"]          = entry.path().string();
            info["type"]          = file_type_char(entry);
            info["is_directory"]  = entry.is_directory();
            info["is_symlink"]    = entry.is_symlink();
            info["permissions"]   = permissions_string(entry.status().permissions());
            info["owner"]         = get_owner(entry.path());
            info["group"]         = get_group(entry.path());
            info["last_modified"] = format_time(entry.last_write_time());

            uintmax_t sz = entry.is_regular_file() ? entry.file_size() : 0;
            info["size"]  = sz;
            info["size_human"] = human_readable_size(sz);

            if (entry.is_symlink()) {
                std::error_code ec;
                auto target = fs::read_symlink(entry.path(), ec);
                info["symlink_target"] = ec ? "" : target.string();
            }

            result.append(info);
        } else {
            result.append(entry.path().filename().string());
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// pwd — Print working directory
// ---------------------------------------------------------------------------

static std::string pwd_impl() {
    return fs::current_path().string();
}

// ---------------------------------------------------------------------------
// cd — Change working directory
// ---------------------------------------------------------------------------

static void cd_impl(const std::string& path) {
    fs::path target(path);
    if (!fs::exists(target))
        throw py::value_error("cd: no such file or directory: " + path);
    if (!fs::is_directory(target))
        throw py::value_error("cd: not a directory: " + path);
    fs::current_path(target);
}

// ---------------------------------------------------------------------------
// mkdir — Create directories
// ---------------------------------------------------------------------------

static void mkdir_impl(const std::string& path, bool parents) {
    fs::path dir(path);
    if (parents) {
        fs::create_directories(dir);
    } else {
        if (!fs::create_directory(dir))
            throw py::value_error("mkdir: cannot create directory '" + path + "'");
    }
}

// ---------------------------------------------------------------------------
// rmdir — Remove empty directory
// ---------------------------------------------------------------------------

static void rmdir_impl(const std::string& path) {
    fs::path dir(path);
    if (!fs::exists(dir))
        throw py::value_error("rmdir: failed to remove '" + path + "': No such file or directory");
    if (!fs::is_directory(dir))
        throw py::value_error("rmdir: failed to remove '" + path + "': Not a directory");
    if (!fs::is_empty(dir))
        throw py::value_error("rmdir: failed to remove '" + path + "': Directory not empty");
    fs::remove(dir);
}

// ---------------------------------------------------------------------------
// rm — Remove files and directories
// ---------------------------------------------------------------------------

static void rm_impl(const std::string& path, bool recursive, bool force) {
    fs::path target(path);
    if (!fs::exists(target)) {
        if (!force)
            throw py::value_error("rm: cannot remove '" + path + "': No such file or directory");
        return;
    }
    if (fs::is_directory(target) && !recursive) {
        throw py::value_error("rm: cannot remove '" + path + "': Is a directory (use recursive=True)");
    }
    if (recursive) {
        fs::remove_all(target);
    } else {
        fs::remove(target);
    }
}

// ---------------------------------------------------------------------------
// touch — Create file or update timestamps
// ---------------------------------------------------------------------------

static void touch_impl(const std::string& path, bool no_create) {
    fs::path target(path);
    if (fs::exists(target)) {
        fs::last_write_time(target, fs::file_time_type::clock::now());
    } else {
        if (!no_create) {
            std::ofstream ofs(path);
            if (!ofs.is_open())
                throw py::value_error("touch: cannot touch '" + path + "': Permission denied");
        }
    }
}

// ---------------------------------------------------------------------------
// cp — Copy files and directories
// ---------------------------------------------------------------------------

static void cp_impl(const std::string& src, const std::string& dst,
                     bool recursive, bool force, bool preserve) {
    fs::path source(src);
    fs::path destination(dst);

    if (!fs::exists(source))
        throw py::value_error("cp: cannot stat '" + src + "': No such file or directory");

    auto options = fs::copy_options::none;
    if (recursive)   options |= fs::copy_options::recursive;
    if (force)       options |= fs::copy_options::overwrite_existing;
    if (preserve)    options |= fs::copy_options::copy_symlinks;

    fs::copy(source, destination, options);
}

// ---------------------------------------------------------------------------
// mv — Move/rename files
// ---------------------------------------------------------------------------

static void mv_impl(const std::string& src, const std::string& dst, bool force) {
    fs::path source(src);
    fs::path destination(dst);

    if (!fs::exists(source))
        throw py::value_error("mv: cannot stat '" + src + "': No such file or directory");

    if (fs::exists(destination) && !force)
        throw py::value_error("mv: cannot move '" + src + "' to '" + dst + "': Destination exists (use force=True)");

    fs::rename(source, destination);
}

// ---------------------------------------------------------------------------
// ln — Create links
// ---------------------------------------------------------------------------

static void ln_impl(const std::string& target, const std::string& link_name,
                     bool symbolic) {
    fs::path tgt(target);
    fs::path lnk(link_name);

    if (symbolic) {
        fs::create_symlink(tgt, lnk);
    } else {
        if (!fs::exists(tgt))
            throw py::value_error("ln: failed to access '" + target + "': No such file or directory");
        fs::create_hard_link(tgt, lnk);
    }
}

// ---------------------------------------------------------------------------
// find — Search for files
// ---------------------------------------------------------------------------

static py::list find_impl(const std::string& path,
                            const std::string& name,
                            const std::string& type,
                            long long min_size,
                            long long max_size,
                            int max_depth) {
    fs::path root(path);
    if (!fs::exists(root))
        throw py::value_error("find: '" + path + "': No such file or directory");

    py::list results;

    auto matches_name = [&](const fs::directory_entry& entry) -> bool {
        if (name.empty()) return true;
        std::string fname = entry.path().filename().string();
        // Simple glob: support '*' prefix/suffix
        if (name.front() == '*' && name.back() == '*') {
            std::string pattern = name.substr(1, name.size() - 2);
            return fname.find(pattern) != std::string::npos;
        } else if (name.front() == '*') {
            std::string suffix = name.substr(1);
            return fname.size() >= suffix.size() &&
                   fname.compare(fname.size() - suffix.size(), suffix.size(), suffix) == 0;
        } else if (name.back() == '*') {
            std::string prefix = name.substr(0, name.size() - 1);
            return fname.compare(0, prefix.size(), prefix) == 0;
        }
        return fname == name;
    };

    auto matches_type = [&](const fs::directory_entry& entry) -> bool {
        if (type.empty()) return true;
        if (type == "f") return entry.is_regular_file();
        if (type == "d") return entry.is_directory();
        if (type == "l") return entry.is_symlink();
        return true;
    };

    auto matches_size = [&](const fs::directory_entry& entry) -> bool {
        if (!entry.is_regular_file()) return (min_size < 0 && max_size < 0);
        auto sz = static_cast<long long>(entry.file_size());
        if (min_size >= 0 && sz < min_size) return false;
        if (max_size >= 0 && sz > max_size) return false;
        return true;
    };

    for (auto it = fs::recursive_directory_iterator(
             root, fs::directory_options::skip_permission_denied);
         it != fs::recursive_directory_iterator(); ++it) {
        if (max_depth >= 0 && it.depth() > max_depth) {
            it.disable_recursion_pending();
            continue;
        }
        if (matches_name(*it) && matches_type(*it) && matches_size(*it)) {
            results.append(it->path().string());
        }
    }

    return results;
}

// ---------------------------------------------------------------------------
// du — Disk usage
// ---------------------------------------------------------------------------

static py::object du_impl(const std::string& path, bool human_readable,
                            bool summary_only) {
    fs::path root(path);
    if (!fs::exists(root))
        throw py::value_error("du: cannot access '" + path + "': No such file or directory");

    if (summary_only || fs::is_regular_file(root)) {
        uintmax_t total = 0;
        if (fs::is_regular_file(root)) {
            total = fs::file_size(root);
        } else {
            for (auto& entry : fs::recursive_directory_iterator(
                     root, fs::directory_options::skip_permission_denied)) {
                if (entry.is_regular_file())
                    total += entry.file_size();
            }
        }
        py::dict res;
        res["path"] = root.string();
        res["bytes"] = total;
        res["human"] = human_readable_size(total);
        return res;
    }

    // Per-directory breakdown
    py::list results;
    std::map<std::string, uintmax_t> dir_sizes;

    for (auto& entry : fs::recursive_directory_iterator(
             root, fs::directory_options::skip_permission_denied)) {
        if (entry.is_regular_file()) {
            auto sz = entry.file_size();
            auto dir = entry.path().parent_path().string();
            dir_sizes[dir] += sz;
        }
    }

    for (auto& [dir, sz] : dir_sizes) {
        py::dict d;
        d["path"]  = dir;
        d["bytes"] = sz;
        d["human"] = human_readable_size(sz);
        results.append(d);
    }

    return results;
}

// ---------------------------------------------------------------------------
// chmod — Change file permissions
// ---------------------------------------------------------------------------

static void chmod_impl(const std::string& path, int mode, bool recursive) {
    fs::path target(path);
    if (!fs::exists(target))
        throw py::value_error("chmod: cannot access '" + path + "': No such file or directory");

    auto perms = static_cast<fs::perms>(mode);

    auto apply = [&](const fs::path& p) {
        fs::permissions(p, perms, fs::perm_options::replace);
    };

    apply(target);
    if (recursive && fs::is_directory(target)) {
        for (auto& entry : fs::recursive_directory_iterator(
                 target, fs::directory_options::skip_permission_denied)) {
            apply(entry.path());
        }
    }
}

// ---------------------------------------------------------------------------
// chown — Change file ownership (requires root)
// ---------------------------------------------------------------------------

static void chown_impl(const std::string& path, const std::string& owner,
                        const std::string& group, bool recursive) {
    fs::path target(path);
    if (!fs::exists(target))
        throw py::value_error("chown: cannot access '" + path + "': No such file or directory");

    uid_t uid = static_cast<uid_t>(-1);
    gid_t gid = static_cast<gid_t>(-1);

    if (!owner.empty()) {
        struct passwd* pw = getpwnam(owner.c_str());
        if (!pw)
            throw py::value_error("chown: invalid user: '" + owner + "'");
        uid = pw->pw_uid;
    }
    if (!group.empty()) {
        struct group* gr = getgrnam(group.c_str());
        if (!gr)
            throw py::value_error("chown: invalid group: '" + group + "'");
        gid = gr->gr_gid;
    }

    auto apply = [&](const fs::path& p) {
        if (::chown(p.c_str(), uid, gid) != 0)
            throw py::value_error("chown: changing ownership of '" + p.string() +
                                  "': " + std::strerror(errno));
    };

    apply(target);
    if (recursive && fs::is_directory(target)) {
        for (auto& entry : fs::recursive_directory_iterator(
                 target, fs::directory_options::skip_permission_denied)) {
            apply(entry.path());
        }
    }
}

// ===========================================================================
// pybind11 registration
// ===========================================================================

void init_filesystem(py::module_ &m) {

    // -- ls -----------------------------------------------------------------
    m.def("ls", &ls_impl,
        R"doc(
        List directory contents.

        Equivalent to the ``ls`` shell command. Returns file and directory names
        in the given path, with support for long format, recursive listing,
        sorting, and hidden file display.

        Args:
            path (str): Directory path to list. Defaults to current directory ".".
            all (bool): If True, include hidden files (starting with '.').
                        Equivalent to ``ls -a``.
            long_format (bool): If True, return detailed dicts with name, path,
                                type, permissions, owner, group, size, last_modified.
                                Equivalent to ``ls -l``.
            recursive (bool): If True, list directories recursively.
                              Equivalent to ``ls -R``.
            sort_by (str): Sort entries by "name", "size", or "time".
                           Equivalent to ``ls -S`` (size) or ``ls -t`` (time).
            reverse (bool): If True, reverse the sort order.
                            Equivalent to ``ls -r``.
            human_readable (bool): If True, show sizes in human-readable format (K, M, G).
                                   Equivalent to ``ls -h``.
            directory_only (bool): If True, only list directories.
                                   Equivalent to ``ls -d``.

        Returns:
            list: A list of filenames (str) or dicts (if long_format=True).

        Raises:
            ValueError: If path does not exist.
        )doc",
        py::arg("path") = ".",
        py::arg("all") = false,
        py::arg("long_format") = false,
        py::arg("recursive") = false,
        py::arg("sort_by") = "name",
        py::arg("reverse") = false,
        py::arg("human_readable") = false,
        py::arg("directory_only") = false);

    // -- pwd ----------------------------------------------------------------
    m.def("pwd", &pwd_impl,
        R"doc(
        Print working directory.

        Equivalent to the ``pwd`` shell command. Returns the absolute path of
        the current working directory as a string.

        Returns:
            str: Absolute path of the current working directory.
        )doc");

    // -- cd -----------------------------------------------------------------
    m.def("cd", &cd_impl,
        R"doc(
        Change the current working directory.

        Equivalent to the ``cd`` shell command. Changes the process's current
        working directory to the specified path.

        Args:
            path (str): Target directory path.

        Raises:
            ValueError: If path does not exist or is not a directory.
        )doc",
        py::arg("path"));

    // -- mkdir --------------------------------------------------------------
    m.def("mkdir", &mkdir_impl,
        R"doc(
        Create a directory.

        Equivalent to the ``mkdir`` shell command. Creates a new directory at
        the specified path.

        Args:
            path (str): Path of the directory to create.
            parents (bool): If True, create parent directories as needed.
                            Equivalent to ``mkdir -p``.

        Raises:
            ValueError: If directory cannot be created.
        )doc",
        py::arg("path"),
        py::arg("parents") = false);

    // -- rmdir --------------------------------------------------------------
    m.def("rmdir", &rmdir_impl,
        R"doc(
        Remove an empty directory.

        Equivalent to the ``rmdir`` shell command. Removes the specified
        directory only if it is empty.

        Args:
            path (str): Path of the empty directory to remove.

        Raises:
            ValueError: If path doesn't exist, is not a directory, or is not empty.
        )doc",
        py::arg("path"));

    // -- rm -----------------------------------------------------------------
    m.def("rm", &rm_impl,
        R"doc(
        Remove files or directories.

        Equivalent to the ``rm`` shell command. Removes files, and optionally
        directories recursively.

        Args:
            path (str): Path of the file or directory to remove.
            recursive (bool): If True, remove directories and their contents
                              recursively. Equivalent to ``rm -r``.
            force (bool): If True, ignore nonexistent files and never prompt.
                          Equivalent to ``rm -f``.

        Raises:
            ValueError: If path doesn't exist (unless force=True) or trying to
                        remove a directory without recursive=True.
        )doc",
        py::arg("path"),
        py::arg("recursive") = false,
        py::arg("force") = false);

    // -- touch --------------------------------------------------------------
    m.def("touch", &touch_impl,
        R"doc(
        Create an empty file or update its timestamp.

        Equivalent to the ``touch`` shell command. If the file exists, updates
        its last-modified timestamp. If not, creates a new empty file.

        Args:
            path (str): Path of the file to touch.
            no_create (bool): If True, do not create the file if it doesn't exist.
                              Equivalent to ``touch -c``.

        Raises:
            ValueError: If the file cannot be created.
        )doc",
        py::arg("path"),
        py::arg("no_create") = false);

    // -- cp -----------------------------------------------------------------
    m.def("cp", &cp_impl,
        R"doc(
        Copy files or directories.

        Equivalent to the ``cp`` shell command. Copies a source file or
        directory to a destination.

        Args:
            src (str): Source path.
            dst (str): Destination path.
            recursive (bool): If True, copy directories recursively.
                              Equivalent to ``cp -r``.
            force (bool): If True, overwrite existing files.
                          Equivalent to ``cp -f``.
            preserve (bool): If True, preserve symlinks instead of following them.
                             Equivalent to ``cp -P``.

        Raises:
            ValueError: If source does not exist.
        )doc",
        py::arg("src"),
        py::arg("dst"),
        py::arg("recursive") = false,
        py::arg("force") = false,
        py::arg("preserve") = false);

    // -- mv -----------------------------------------------------------------
    m.def("mv", &mv_impl,
        R"doc(
        Move or rename files and directories.

        Equivalent to the ``mv`` shell command. Moves a file or directory from
        source to destination. Can also be used to rename.

        Args:
            src (str): Source path.
            dst (str): Destination path.
            force (bool): If True, overwrite existing destination without prompting.
                          Equivalent to ``mv -f``.

        Raises:
            ValueError: If source does not exist or destination exists without force.
        )doc",
        py::arg("src"),
        py::arg("dst"),
        py::arg("force") = false);

    // -- ln -----------------------------------------------------------------
    m.def("ln", &ln_impl,
        R"doc(
        Create hard or symbolic links.

        Equivalent to the ``ln`` shell command. Creates a link to a target file
        or directory.

        Args:
            target (str): The file or directory to link to.
            link_name (str): The name of the link to create.
            symbolic (bool): If True, create a symbolic (soft) link.
                             Equivalent to ``ln -s``. Defaults to False (hard link).

        Raises:
            ValueError: If target does not exist (for hard links).
        )doc",
        py::arg("target"),
        py::arg("link_name"),
        py::arg("symbolic") = false);

    // -- find ---------------------------------------------------------------
    m.def("find", &find_impl,
        R"doc(
        Search for files in a directory hierarchy.

        Equivalent to the ``find`` shell command. Recursively searches for files
        matching the given criteria.

        Args:
            path (str): Root directory to search from.
            name (str): Filename pattern to match. Supports simple globs:
                        ``"*.txt"`` (suffix), ``"test*"`` (prefix),
                        ``"*log*"`` (contains). Empty string matches all.
            type (str): File type filter: "f" (regular file), "d" (directory),
                        "l" (symlink). Empty string matches all.
                        Equivalent to ``find -type``.
            min_size (int): Minimum file size in bytes. -1 to ignore.
                            Equivalent to ``find -size +N``.
            max_size (int): Maximum file size in bytes. -1 to ignore.
                            Equivalent to ``find -size -N``.
            max_depth (int): Maximum directory depth to search. -1 for unlimited.
                             Equivalent to ``find -maxdepth``.

        Returns:
            list[str]: List of matching file paths.

        Raises:
            ValueError: If path does not exist.
        )doc",
        py::arg("path") = ".",
        py::arg("name") = "",
        py::arg("type") = "",
        py::arg("min_size") = -1,
        py::arg("max_size") = -1,
        py::arg("max_depth") = -1);

    // -- du -----------------------------------------------------------------
    m.def("du", &du_impl,
        R"doc(
        Estimate file and directory space usage.

        Equivalent to the ``du`` shell command. Calculates disk usage for a
        given path.

        Args:
            path (str): File or directory path.
            human_readable (bool): If True, include human-readable size strings.
                                   Equivalent to ``du -h``.
            summary_only (bool): If True, return only the total for the path.
                                 Equivalent to ``du -s``.

        Returns:
            dict or list[dict]: A dict with keys "path", "bytes", "human" if
            summary_only=True, or a list of such dicts per subdirectory.

        Raises:
            ValueError: If path does not exist.
        )doc",
        py::arg("path") = ".",
        py::arg("human_readable") = false,
        py::arg("summary_only") = true);

    // -- chmod --------------------------------------------------------------
    m.def("chmod", &chmod_impl,
        R"doc(
        Change file mode (permissions).

        Equivalent to the ``chmod`` shell command. Sets the permission bits of a
        file using an octal mode value.

        Args:
            path (str): File or directory path.
            mode (int): Octal permission mode, e.g. 0o755, 0o644.
            recursive (bool): If True, apply permissions recursively to all
                              files and subdirectories. Equivalent to ``chmod -R``.

        Raises:
            ValueError: If path does not exist.
        )doc",
        py::arg("path"),
        py::arg("mode"),
        py::arg("recursive") = false);

    // -- chown --------------------------------------------------------------
    m.def("chown", &chown_impl,
        R"doc(
        Change file owner and group.

        Equivalent to the ``chown`` shell command. Changes the user and/or group
        ownership of a file or directory. Typically requires root privileges.

        Args:
            path (str): File or directory path.
            owner (str): New owner username. Empty string to leave unchanged.
            group (str): New group name. Empty string to leave unchanged.
            recursive (bool): If True, apply changes recursively.
                              Equivalent to ``chown -R``.

        Raises:
            ValueError: If path doesn't exist, user/group is invalid, or
                        insufficient privileges.
        )doc",
        py::arg("path"),
        py::arg("owner") = "",
        py::arg("group") = "",
        py::arg("recursive") = false);
}
