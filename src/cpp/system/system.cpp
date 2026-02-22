#include "system.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <fstream>
#include <ctime>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <cstring>
#include <filesystem>

#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

namespace py = pybind11;
namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// uname — System information
// ---------------------------------------------------------------------------

static py::dict uname_impl(bool all) {
    struct utsname buf;
    if (uname(&buf) != 0)
        throw py::value_error("uname: unable to get system information");

    py::dict result;
    result["sysname"]  = std::string(buf.sysname);
    result["nodename"] = std::string(buf.nodename);
    result["release"]  = std::string(buf.release);
    result["version"]  = std::string(buf.version);
    result["machine"]  = std::string(buf.machine);

    if (all) {
        result["all"] = std::string(buf.sysname) + " " +
                       std::string(buf.nodename) + " " +
                       std::string(buf.release) + " " +
                       std::string(buf.version) + " " +
                       std::string(buf.machine);
    }
    return result;
}

// ---------------------------------------------------------------------------
// whoami — Current username
// ---------------------------------------------------------------------------

static std::string whoami_impl() {
    uid_t uid = getuid();
    struct passwd* pw = getpwuid(uid);
    if (pw)
        return std::string(pw->pw_name);

    char* user = getlogin();
    if (user)
        return std::string(user);

    char* env_user = std::getenv("USER");
    if (env_user)
        return std::string(env_user);

    return std::to_string(uid);
}

// ---------------------------------------------------------------------------
// uptime — System uptime
// ---------------------------------------------------------------------------

static py::dict uptime_impl() {
    struct sysinfo info;
    if (sysinfo(&info) != 0)
        throw py::value_error("uptime: unable to get system info");

    long total_seconds = info.uptime;
    int days    = total_seconds / 86400;
    int hours   = (total_seconds % 86400) / 3600;
    int minutes = (total_seconds % 3600) / 60;
    int seconds = total_seconds % 60;

    py::dict result;
    result["total_seconds"] = total_seconds;
    result["days"]          = days;
    result["hours"]         = hours;
    result["minutes"]       = minutes;
    result["seconds"]       = seconds;

    std::ostringstream oss;
    if (days > 0) oss << days << " day" << (days > 1 ? "s" : "") << ", ";
    oss << hours << ":" << (minutes < 10 ? "0" : "") << minutes << ":"
        << (seconds < 10 ? "0" : "") << seconds;
    result["formatted"] = oss.str();

    // Load averages
    result["load_1"]  = info.loads[0] / 65536.0;
    result["load_5"]  = info.loads[1] / 65536.0;
    result["load_15"] = info.loads[2] / 65536.0;

    return result;
}

// ---------------------------------------------------------------------------
// env — Get environment variables
// ---------------------------------------------------------------------------

extern char **environ;

static py::dict env_impl() {
    py::dict result;
    for (char **env = environ; *env != nullptr; env++) {
        std::string entry(*env);
        auto pos = entry.find('=');
        if (pos != std::string::npos) {
            result[py::cast(entry.substr(0, pos))] = entry.substr(pos + 1);
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// getenv — Get single environment variable
// ---------------------------------------------------------------------------

static py::object getenv_impl(const std::string& name, const std::string& default_val) {
    const char* val = std::getenv(name.c_str());
    if (val)
        return py::cast(std::string(val));
    if (!default_val.empty())
        return py::cast(default_val);
    return py::none();
}

// ---------------------------------------------------------------------------
// export / setenv — Set environment variable
// ---------------------------------------------------------------------------

static void export_impl(const std::string& name, const std::string& value,
                          bool overwrite) {
    if (setenv(name.c_str(), value.c_str(), overwrite ? 1 : 0) != 0)
        throw py::value_error("export: failed to set '" + name + "'");
}

// ---------------------------------------------------------------------------
// unsetenv — Unset environment variable
// ---------------------------------------------------------------------------

static void unsetenv_impl(const std::string& name) {
    if (::unsetenv(name.c_str()) != 0)
        throw py::value_error("unsetenv: failed to unset '" + name + "'");
}

// ---------------------------------------------------------------------------
// clear — Clear terminal
// ---------------------------------------------------------------------------

static std::string clear_impl() {
    return "\033[2J\033[H";
}

// ---------------------------------------------------------------------------
// cal — Calendar
// ---------------------------------------------------------------------------

static std::string cal_impl(int month, int year) {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    struct tm* tm_now = std::localtime(&t);

    if (year <= 0) year = tm_now->tm_year + 1900;
    if (month <= 0) month = tm_now->tm_mon + 1;

    // Day of week for the 1st of the month (Zeller-like)
    struct tm first_day = {};
    first_day.tm_year = year - 1900;
    first_day.tm_mon = month - 1;
    first_day.tm_mday = 1;
    mktime(&first_day);
    int start_dow = first_day.tm_wday; // 0=Sun

    // Days in month
    int days_in_month;
    if (month == 2) {
        bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        days_in_month = leap ? 29 : 28;
    } else if (month == 4 || month == 6 || month == 9 || month == 11) {
        days_in_month = 30;
    } else {
        days_in_month = 31;
    }

    const char* month_names[] = {"January", "February", "March", "April",
                                  "May", "June", "July", "August",
                                  "September", "October", "November", "December"};

    std::ostringstream oss;
    std::string header = std::string(month_names[month - 1]) + " " + std::to_string(year);
    int pad = (20 - static_cast<int>(header.size())) / 2;
    if (pad > 0) oss << std::string(pad, ' ');
    oss << header << "\n";
    oss << "Su Mo Tu We Th Fr Sa\n";

    // Leading spaces
    for (int i = 0; i < start_dow; i++)
        oss << "   ";

    for (int day = 1; day <= days_in_month; day++) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%2d", day);
        oss << buf;
        if ((start_dow + day) % 7 == 0)
            oss << "\n";
        else
            oss << " ";
    }
    if ((start_dow + days_in_month) % 7 != 0)
        oss << "\n";

    return oss.str();
}

// ---------------------------------------------------------------------------
// date — Display date/time
// ---------------------------------------------------------------------------

static std::string date_impl(const std::string& format) {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    struct tm* tm = std::localtime(&t);

    std::string fmt = format.empty() ? "%a %b %e %H:%M:%S %Z %Y" : format;
    char buf[256];
    std::strftime(buf, sizeof(buf), fmt.c_str(), tm);
    return std::string(buf);
}

// ---------------------------------------------------------------------------
// sleep — Suspend execution
// ---------------------------------------------------------------------------

static void sleep_impl(double seconds) {
    auto duration = std::chrono::duration<double>(seconds);
    std::this_thread::sleep_for(duration);
}

// ---------------------------------------------------------------------------
// id — User identity info
// ---------------------------------------------------------------------------

static py::dict id_impl(const std::string& username) {
    py::dict result;

    uid_t uid;
    struct passwd* pw;

    if (username.empty()) {
        uid = getuid();
        pw = getpwuid(uid);
    } else {
        pw = getpwnam(username.c_str());
        if (!pw)
            throw py::value_error("id: '" + username + "': no such user");
        uid = pw->pw_uid;
    }

    result["uid"]      = static_cast<int>(uid);
    result["username"] = pw ? std::string(pw->pw_name) : std::to_string(uid);
    result["gid"]      = pw ? static_cast<int>(pw->pw_gid) : -1;

    if (pw) {
        struct group* gr = getgrgid(pw->pw_gid);
        result["group"] = gr ? std::string(gr->gr_name) : std::to_string(pw->pw_gid);
    }

    // Supplementary groups
    if (pw) {
        int ngroups = 64;
        std::vector<gid_t> groups(ngroups);
        if (getgrouplist(pw->pw_name, pw->pw_gid, groups.data(), &ngroups) == -1) {
            groups.resize(ngroups);
            getgrouplist(pw->pw_name, pw->pw_gid, groups.data(), &ngroups);
        }
        py::list grp_list;
        for (int i = 0; i < ngroups; i++) {
            struct group* g = getgrgid(groups[i]);
            py::dict gd;
            gd["gid"] = static_cast<int>(groups[i]);
            gd["name"] = g ? std::string(g->gr_name) : std::to_string(groups[i]);
            grp_list.append(gd);
        }
        result["groups"] = grp_list;
    }

    return result;
}

// ---------------------------------------------------------------------------
// groups — User group memberships
// ---------------------------------------------------------------------------

static py::list groups_impl(const std::string& username) {
    struct passwd* pw;
    if (username.empty()) {
        pw = getpwuid(getuid());
    } else {
        pw = getpwnam(username.c_str());
    }
    if (!pw)
        throw py::value_error("groups: unknown user '" + username + "'");

    int ngroups = 64;
    std::vector<gid_t> gids(ngroups);
    if (getgrouplist(pw->pw_name, pw->pw_gid, gids.data(), &ngroups) == -1) {
        gids.resize(ngroups);
        getgrouplist(pw->pw_name, pw->pw_gid, gids.data(), &ngroups);
    }

    py::list result;
    for (int i = 0; i < ngroups; i++) {
        struct group* gr = getgrgid(gids[i]);
        result.append(gr ? std::string(gr->gr_name) : std::to_string(gids[i]));
    }
    return result;
}

// ---------------------------------------------------------------------------
// free — Memory usage
// ---------------------------------------------------------------------------

static py::dict free_impl(bool human_readable) {
    std::ifstream ifs("/proc/meminfo");
    if (!ifs.is_open())
        throw py::value_error("free: cannot read /proc/meminfo");

    std::map<std::string, long> mem;
    std::string line;
    while (std::getline(ifs, line)) {
        std::istringstream iss(line);
        std::string key;
        long value;
        std::string unit;
        iss >> key >> value >> unit;
        key.pop_back(); // Remove ':'
        mem[key] = value; // in kB
    }

    auto format_kb = [&](long kb) -> py::object {
        if (human_readable) {
            double val = kb;
            const char* units[] = {"K", "M", "G", "T"};
            int i = 0;
            while (val >= 1024.0 && i < 3) { val /= 1024.0; i++; }
            char buf[32];
            snprintf(buf, sizeof(buf), "%.1f%s", val, units[i]);
            return py::cast(std::string(buf));
        }
        return py::cast(kb);
    };

    py::dict result;

    // RAM
    py::dict ram;
    ram["total"]     = format_kb(mem["MemTotal"]);
    ram["used"]      = format_kb(mem["MemTotal"] - mem["MemAvailable"]);
    ram["free"]      = format_kb(mem["MemFree"]);
    ram["available"] = format_kb(mem["MemAvailable"]);
    ram["buffers"]   = format_kb(mem["Buffers"]);
    ram["cached"]    = format_kb(mem["Cached"]);
    result["ram"] = ram;

    // Swap
    py::dict swap;
    swap["total"] = format_kb(mem["SwapTotal"]);
    swap["used"]  = format_kb(mem["SwapTotal"] - mem["SwapFree"]);
    swap["free"]  = format_kb(mem["SwapFree"]);
    result["swap"] = swap;

    return result;
}

// ---------------------------------------------------------------------------
// whereis — Locate binary, source, and man pages
// ---------------------------------------------------------------------------

static py::dict whereis_impl(const std::string& command) {
    py::dict result;
    result["command"] = command;

    // Search PATH for binary
    py::list binaries;
    const char* path_env = std::getenv("PATH");
    if (path_env) {
        std::istringstream iss(path_env);
        std::string dir;
        while (std::getline(iss, dir, ':')) {
            fs::path candidate = fs::path(dir) / command;
            if (fs::exists(candidate) && !fs::is_directory(candidate))
                binaries.append(candidate.string());
        }
    }
    result["binaries"] = binaries;

    // Search common man page locations
    py::list man_pages;
    std::vector<std::string> man_dirs = {
        "/usr/share/man", "/usr/local/share/man", "/usr/man"
    };
    for (const auto& mdir : man_dirs) {
        if (!fs::exists(mdir)) continue;
        for (auto& entry : fs::recursive_directory_iterator(
                 mdir, fs::directory_options::skip_permission_denied)) {
            if (entry.is_regular_file()) {
                std::string fname = entry.path().filename().string();
                if (fname.find(command) == 0)
                    man_pages.append(entry.path().string());
            }
        }
    }
    result["man_pages"] = man_pages;

    // Search common source locations
    py::list sources;
    std::vector<std::string> src_dirs = {"/usr/src", "/usr/local/src"};
    for (const auto& sdir : src_dirs) {
        if (!fs::exists(sdir)) continue;
        for (auto& entry : fs::directory_iterator(
                 sdir, fs::directory_options::skip_permission_denied)) {
            std::string fname = entry.path().filename().string();
            if (fname.find(command) != std::string::npos)
                sources.append(entry.path().string());
        }
    }
    result["sources"] = sources;

    return result;
}

// ===========================================================================
// pybind11 registration
// ===========================================================================

void init_system(py::module_ &m) {

    // -- uname --------------------------------------------------------------
    m.def("uname", &uname_impl,
        R"doc(
        Return system information.

        Equivalent to the ``uname`` shell command. Returns OS name, hostname,
        kernel release, version, and machine architecture.

        Args:
            all (bool): If True, include a combined "all" string field.
                        Equivalent to ``uname -a``.

        Returns:
            dict: Keys "sysname", "nodename", "release", "version", "machine",
                  and optionally "all".

        Raises:
            ValueError: If system info cannot be retrieved.
        )doc",
        py::arg("all") = false);

    // -- whoami -------------------------------------------------------------
    m.def("whoami", &whoami_impl,
        R"doc(
        Return the current username.

        Equivalent to the ``whoami`` shell command. Returns the username
        associated with the current effective user ID.

        Returns:
            str: The current username.
        )doc");

    // -- uptime -------------------------------------------------------------
    m.def("uptime", &uptime_impl,
        R"doc(
        Return system uptime and load averages.

        Equivalent to the ``uptime`` shell command. Returns how long the
        system has been running along with load averages.

        Returns:
            dict: Keys "total_seconds", "days", "hours", "minutes", "seconds",
                  "formatted", "load_1", "load_5", "load_15".

        Raises:
            ValueError: If system info cannot be retrieved.
        )doc");

    // -- env ----------------------------------------------------------------
    m.def("env", &env_impl,
        R"doc(
        Return all environment variables.

        Equivalent to the ``env`` shell command. Returns a dict of all
        currently set environment variables.

        Returns:
            dict: Mapping of environment variable names to their values.
        )doc");

    // -- getenv -------------------------------------------------------------
    m.def("getenv", &getenv_impl,
        R"doc(
        Get the value of a single environment variable.

        Similar to reading a specific variable with ``echo $VAR``.

        Args:
            name (str): The environment variable name.
            default_val (str): Default value if the variable is not set.

        Returns:
            str or None: The variable value, or default, or None.
        )doc",
        py::arg("name"),
        py::arg("default_val") = "");

    // -- export / setenv ----------------------------------------------------
    m.def("export_env", &export_impl,
        R"doc(
        Set an environment variable.

        Equivalent to the ``export`` shell command. Sets a variable in the
        current process environment.

        Args:
            name (str): The environment variable name.
            value (str): The value to set.
            overwrite (bool): If True, overwrite existing value.
                              Defaults to True.

        Raises:
            ValueError: If the variable cannot be set.
        )doc",
        py::arg("name"),
        py::arg("value"),
        py::arg("overwrite") = true);

    // -- unsetenv -----------------------------------------------------------
    m.def("unsetenv", &unsetenv_impl,
        R"doc(
        Remove an environment variable.

        Equivalent to ``unset VAR`` in shell. Removes the specified variable
        from the current process environment.

        Args:
            name (str): The environment variable name to remove.

        Raises:
            ValueError: If the variable cannot be unset.
        )doc",
        py::arg("name"));

    // -- clear --------------------------------------------------------------
    m.def("clear", &clear_impl,
        R"doc(
        Return ANSI escape codes to clear the terminal.

        Equivalent to the ``clear`` shell command. Returns the escape sequence
        that, when printed, clears the terminal screen.

        Returns:
            str: ANSI escape sequence to clear the screen.
        )doc");

    // -- cal ----------------------------------------------------------------
    m.def("cal", &cal_impl,
        R"doc(
        Display a calendar for a given month and year.

        Equivalent to the ``cal`` shell command. Returns a text-based
        calendar for the specified month.

        Args:
            month (int): Month number (1-12). Defaults to current month.
                         Equivalent to ``cal -m``.
            year (int): Year (e.g. 2025). Defaults to current year.

        Returns:
            str: Formatted calendar text.
        )doc",
        py::arg("month") = -1,
        py::arg("year") = -1);

    // -- date ---------------------------------------------------------------
    m.def("date", &date_impl,
        R"doc(
        Display the current date and time.

        Equivalent to the ``date`` shell command. Returns the current date
        and time formatted according to the given strftime format string.

        Args:
            format (str): strftime-compatible format string.
                          Default: "%a %b %e %H:%M:%S %Z %Y".
                          Equivalent to ``date +FORMAT``.

        Returns:
            str: Formatted date/time string.
        )doc",
        py::arg("format") = "");

    // -- sleep --------------------------------------------------------------
    m.def("sleep", &sleep_impl,
        R"doc(
        Suspend execution for a given number of seconds.

        Equivalent to the ``sleep`` shell command. Pauses the current thread
        for the specified duration. Supports fractional seconds.

        Args:
            seconds (float): Number of seconds to sleep.
        )doc",
        py::arg("seconds"));

    // -- id -----------------------------------------------------------------
    m.def("id", &id_impl,
        R"doc(
        Return user identity information.

        Equivalent to the ``id`` shell command. Returns UID, GID, username,
        primary group, and list of supplementary groups.

        Args:
            username (str): Username to look up. Empty for current user.

        Returns:
            dict: Keys "uid", "username", "gid", "group", "groups".

        Raises:
            ValueError: If the user does not exist.
        )doc",
        py::arg("username") = "");

    // -- groups -------------------------------------------------------------
    m.def("groups", &groups_impl,
        R"doc(
        List groups a user belongs to.

        Equivalent to the ``groups`` shell command. Returns the list of group
        names the specified user is a member of.

        Args:
            username (str): Username to look up. Empty for current user.

        Returns:
            list[str]: List of group names.

        Raises:
            ValueError: If the user does not exist.
        )doc",
        py::arg("username") = "");

    // -- free ---------------------------------------------------------------
    m.def("free", &free_impl,
        R"doc(
        Display memory usage information.

        Equivalent to the ``free`` shell command. Reads /proc/meminfo and
        returns RAM and swap usage statistics.

        Args:
            human_readable (bool): If True, format sizes as K/M/G strings.
                                   Equivalent to ``free -h``.

        Returns:
            dict: Keys "ram" and "swap", each containing total, used, free, etc.

        Raises:
            ValueError: If /proc/meminfo cannot be read.
        )doc",
        py::arg("human_readable") = false);

    // -- whereis ------------------------------------------------------------
    m.def("whereis", &whereis_impl,
        R"doc(
        Locate the binary, source, and man pages for a command.

        Equivalent to the ``whereis`` shell command. Searches PATH for the
        binary and known directories for man pages and source files.

        Args:
            command (str): Command name to locate.

        Returns:
            dict: Keys "command", "binaries" (list), "man_pages" (list),
                  "sources" (list).
        )doc",
        py::arg("command"));
}
