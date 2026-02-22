#include "text.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <regex>
#include <map>
#include <set>
#include <numeric>
#include <cstring>

namespace py = pybind11;
namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Utility: Read entire file into string
// ---------------------------------------------------------------------------

static std::string read_file(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open())
        throw py::value_error("Cannot open file: " + path);
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

static std::vector<std::string> read_lines(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open())
        throw py::value_error("Cannot open file: " + path);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(ifs, line))
        lines.push_back(line);
    return lines;
}

// ---------------------------------------------------------------------------
// cat — Concatenate and display file contents
// ---------------------------------------------------------------------------

static std::string cat_impl(const std::string& path,
                              bool number_lines,
                              bool squeeze_blank) {
    auto lines = read_lines(path);
    std::ostringstream oss;
    int line_num = 1;
    bool prev_blank = false;

    for (const auto& line : lines) {
        bool is_blank = line.empty() || line.find_first_not_of(" \t\r\n") == std::string::npos;

        if (squeeze_blank && is_blank && prev_blank)
            continue;

        if (number_lines)
            oss << "     " << line_num++ << "\t";

        oss << line << "\n";
        prev_blank = is_blank;
    }
    return oss.str();
}

// ---------------------------------------------------------------------------
// echo — Return a string (print equivalent)
// ---------------------------------------------------------------------------

static std::string echo_impl(const std::string& text, bool no_newline) {
    if (no_newline)
        return text;
    return text + "\n";
}

// ---------------------------------------------------------------------------
// head — First N lines of a file
// ---------------------------------------------------------------------------

static std::string head_impl(const std::string& path, int n, int bytes) {
    if (bytes > 0) {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs.is_open())
            throw py::value_error("head: cannot open '" + path + "'");
        std::string buf(bytes, '\0');
        ifs.read(&buf[0], bytes);
        buf.resize(ifs.gcount());
        return buf;
    }

    auto lines = read_lines(path);
    std::ostringstream oss;
    int count = std::min(n, static_cast<int>(lines.size()));
    for (int i = 0; i < count; i++)
        oss << lines[i] << "\n";
    return oss.str();
}

// ---------------------------------------------------------------------------
// tail — Last N lines of a file
// ---------------------------------------------------------------------------

static std::string tail_impl(const std::string& path, int n, int bytes) {
    if (bytes > 0) {
        std::ifstream ifs(path, std::ios::binary | std::ios::ate);
        if (!ifs.is_open())
            throw py::value_error("tail: cannot open '" + path + "'");
        auto size = ifs.tellg();
        int read_bytes = std::min(static_cast<int>(size), bytes);
        ifs.seekg(-read_bytes, std::ios::end);
        std::string buf(read_bytes, '\0');
        ifs.read(&buf[0], read_bytes);
        return buf;
    }

    auto lines = read_lines(path);
    int total = static_cast<int>(lines.size());
    int start = std::max(0, total - n);
    std::ostringstream oss;
    for (int i = start; i < total; i++)
        oss << lines[i] << "\n";
    return oss.str();
}

// ---------------------------------------------------------------------------
// grep — Search for patterns in files
// ---------------------------------------------------------------------------

static py::object grep_impl(const std::string& pattern,
                              const std::string& path,
                              bool ignore_case,
                              bool recursive,
                              bool line_numbers,
                              bool count_only,
                              bool invert,
                              bool files_only,
                              bool whole_word) {
    std::vector<std::string> files_to_search;

    auto collect_files = [&](const std::string& p) {
        fs::path fpath(p);
        if (!fs::exists(fpath))
            throw py::value_error("grep: " + p + ": No such file or directory");

        if (fs::is_regular_file(fpath)) {
            files_to_search.push_back(p);
        } else if (fs::is_directory(fpath) && recursive) {
            for (auto& entry : fs::recursive_directory_iterator(
                     fpath, fs::directory_options::skip_permission_denied)) {
                if (entry.is_regular_file())
                    files_to_search.push_back(entry.path().string());
            }
        } else if (fs::is_directory(fpath)) {
            throw py::value_error("grep: " + p + ": Is a directory (use recursive=True)");
        }
    };

    collect_files(path);

    // Build regex
    std::string regex_pattern = pattern;
    if (whole_word)
        regex_pattern = "\\b" + regex_pattern + "\\b";

    auto flags = std::regex_constants::ECMAScript;
    if (ignore_case)
        flags |= std::regex_constants::icase;

    std::regex re;
    try {
        re = std::regex(regex_pattern, flags);
    } catch (const std::regex_error& e) {
        throw py::value_error("grep: invalid regex pattern: " + std::string(e.what()));
    }

    bool multi_file = files_to_search.size() > 1;

    if (count_only) {
        py::dict counts;
        for (const auto& file : files_to_search) {
            auto lines = read_lines(file);
            int match_count = 0;
            for (const auto& line : lines) {
                bool match = std::regex_search(line, re);
                if (invert) match = !match;
                if (match) match_count++;
            }
            counts[py::cast(file)] = match_count;
        }
        return counts;
    }

    if (files_only) {
        py::list matching_files;
        for (const auto& file : files_to_search) {
            auto lines = read_lines(file);
            for (const auto& line : lines) {
                bool match = std::regex_search(line, re);
                if (invert) match = !match;
                if (match) {
                    matching_files.append(file);
                    break;
                }
            }
        }
        return matching_files;
    }

    py::list results;
    for (const auto& file : files_to_search) {
        auto lines = read_lines(file);
        for (int i = 0; i < static_cast<int>(lines.size()); i++) {
            bool match = std::regex_search(lines[i], re);
            if (invert) match = !match;
            if (match) {
                py::dict entry;
                if (multi_file)
                    entry["file"] = file;
                if (line_numbers)
                    entry["line_number"] = i + 1;
                entry["line"] = lines[i];
                results.append(entry);
            }
        }
    }

    return results;
}

// ---------------------------------------------------------------------------
// sort — Sort lines of a file
// ---------------------------------------------------------------------------

static std::string sort_impl(const std::string& path,
                               bool reverse,
                               bool numeric,
                               bool unique,
                               int key,
                               const std::string& separator,
                               bool ignore_case) {
    auto lines = read_lines(path);

    auto get_key = [&](const std::string& line) -> std::string {
        if (key <= 0) return line;
        std::istringstream iss(line);
        std::string token;
        char sep = separator.empty() ? ' ' : separator[0];

        if (separator.empty()) {
            for (int i = 0; i < key; i++) {
                if (!(iss >> token)) return "";
            }
        } else {
            std::string temp = line;
            for (int i = 1; i < key; i++) {
                auto pos = temp.find(sep);
                if (pos == std::string::npos) return "";
                temp = temp.substr(pos + 1);
            }
            auto pos = temp.find(sep);
            token = (pos != std::string::npos) ? temp.substr(0, pos) : temp;
        }
        return token;
    };

    if (numeric) {
        std::sort(lines.begin(), lines.end(),
                  [&](const std::string& a, const std::string& b) {
                      try {
                          double va = std::stod(get_key(a));
                          double vb = std::stod(get_key(b));
                          return va < vb;
                      } catch (...) {
                          return a < b;
                      }
                  });
    } else if (ignore_case) {
        std::sort(lines.begin(), lines.end(),
                  [&](const std::string& a, const std::string& b) {
                      std::string ka = get_key(a), kb = get_key(b);
                      std::transform(ka.begin(), ka.end(), ka.begin(), ::tolower);
                      std::transform(kb.begin(), kb.end(), kb.begin(), ::tolower);
                      return ka < kb;
                  });
    } else {
        std::sort(lines.begin(), lines.end(),
                  [&](const std::string& a, const std::string& b) {
                      return get_key(a) < get_key(b);
                  });
    }

    if (reverse)
        std::reverse(lines.begin(), lines.end());

    if (unique) {
        lines.erase(std::unique(lines.begin(), lines.end()), lines.end());
    }

    std::ostringstream oss;
    for (const auto& line : lines)
        oss << line << "\n";
    return oss.str();
}

// ---------------------------------------------------------------------------
// diff — Compare two files line by line
// ---------------------------------------------------------------------------

static std::string diff_impl(const std::string& file1, const std::string& file2,
                               bool unified, int context_lines) {
    auto lines1 = read_lines(file1);
    auto lines2 = read_lines(file2);

    int n = static_cast<int>(lines1.size());
    int m = static_cast<int>(lines2.size());

    // LCS-based diff using dynamic programming
    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));
    for (int i = 1; i <= n; i++) {
        for (int j = 1; j <= m; j++) {
            if (lines1[i - 1] == lines2[j - 1])
                dp[i][j] = dp[i - 1][j - 1] + 1;
            else
                dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
        }
    }

    // Backtrack to find diff
    struct DiffLine { char type; std::string text; int line1; int line2; };
    std::vector<DiffLine> diffs;
    int i = n, j = m;
    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 && lines1[i - 1] == lines2[j - 1]) {
            diffs.push_back({' ', lines1[i - 1], i, j});
            i--; j--;
        } else if (j > 0 && (i == 0 || dp[i][j - 1] >= dp[i - 1][j])) {
            diffs.push_back({'+', lines2[j - 1], 0, j});
            j--;
        } else {
            diffs.push_back({'-', lines1[i - 1], i, 0});
            i--;
        }
    }
    std::reverse(diffs.begin(), diffs.end());

    std::ostringstream oss;
    if (unified) {
        oss << "--- " << file1 << "\n";
        oss << "+++ " << file2 << "\n";
    }

    for (const auto& d : diffs) {
        if (d.type != ' ' || unified) {
            oss << d.type << " " << d.text << "\n";
        }
    }

    return oss.str();
}

// ---------------------------------------------------------------------------
// cmp — Compare two files byte by byte
// ---------------------------------------------------------------------------

static py::dict cmp_impl(const std::string& file1, const std::string& file2,
                           bool silent) {
    std::ifstream ifs1(file1, std::ios::binary);
    std::ifstream ifs2(file2, std::ios::binary);
    if (!ifs1.is_open())
        throw py::value_error("cmp: " + file1 + ": No such file or directory");
    if (!ifs2.is_open())
        throw py::value_error("cmp: " + file2 + ": No such file or directory");

    long byte_offset = 0;
    int line_number = 1;
    bool identical = true;

    char c1, c2;
    while (ifs1.get(c1) && ifs2.get(c2)) {
        byte_offset++;
        if (c1 == '\n') line_number++;
        if (c1 != c2) {
            identical = false;
            break;
        }
    }

    if (identical && (ifs1.get(c1) || ifs2.get(c2))) {
        identical = false;
        byte_offset++;
    }

    py::dict result;
    result["identical"] = identical;
    if (!identical && !silent) {
        result["byte_offset"] = byte_offset;
        result["line_number"] = line_number;
        result["message"] = file1 + " " + file2 + " differ: byte " +
                           std::to_string(byte_offset) + ", line " +
                           std::to_string(line_number);
    }
    return result;
}

// ---------------------------------------------------------------------------
// comm — Compare two sorted files line by line
// ---------------------------------------------------------------------------

static py::dict comm_impl(const std::string& file1, const std::string& file2) {
    auto lines1 = read_lines(file1);
    auto lines2 = read_lines(file2);

    std::set<std::string> set1(lines1.begin(), lines1.end());
    std::set<std::string> set2(lines2.begin(), lines2.end());

    py::list only_in_1, only_in_2, in_both;

    for (const auto& line : set1) {
        if (set2.count(line))
            in_both.append(line);
        else
            only_in_1.append(line);
    }
    for (const auto& line : set2) {
        if (!set1.count(line))
            only_in_2.append(line);
    }

    py::dict result;
    result["only_in_first"]  = only_in_1;
    result["only_in_second"] = only_in_2;
    result["in_both"]        = in_both;
    return result;
}

// ---------------------------------------------------------------------------
// wc — Word, line, character, byte count
// ---------------------------------------------------------------------------

static py::dict wc_impl(const std::string& path,
                          bool lines_only,
                          bool words_only,
                          bool chars_only,
                          bool bytes_only) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open())
        throw py::value_error("wc: " + path + ": No such file or directory");

    long line_count = 0, word_count = 0, char_count = 0, byte_count = 0;
    bool in_word = false;

    char c;
    while (ifs.get(c)) {
        byte_count++;
        char_count++;
        if (c == '\n') line_count++;
        if (std::isspace(static_cast<unsigned char>(c))) {
            in_word = false;
        } else if (!in_word) {
            in_word = true;
            word_count++;
        }
    }

    py::dict result;
    result["file"] = path;

    if (lines_only)      { result["lines"] = line_count; return result; }
    if (words_only)      { result["words"] = word_count; return result; }
    if (chars_only)      { result["chars"] = char_count; return result; }
    if (bytes_only)      { result["bytes"] = byte_count; return result; }

    result["lines"] = line_count;
    result["words"] = word_count;
    result["chars"] = char_count;
    result["bytes"] = byte_count;
    return result;
}

// ---------------------------------------------------------------------------
// cut — Remove sections from each line of files
// ---------------------------------------------------------------------------

static std::string cut_impl(const std::string& path,
                              const std::string& delimiter,
                              const std::string& fields) {
    auto lines = read_lines(path);
    char sep = delimiter.empty() ? '\t' : delimiter[0];

    // Parse fields like "1,3" or "1-3" or "2"
    std::set<int> field_set;
    std::istringstream fss(fields);
    std::string part;
    while (std::getline(fss, part, ',')) {
        auto dash = part.find('-');
        if (dash != std::string::npos) {
            int start = std::stoi(part.substr(0, dash));
            int end = std::stoi(part.substr(dash + 1));
            for (int i = start; i <= end; i++)
                field_set.insert(i);
        } else {
            field_set.insert(std::stoi(part));
        }
    }

    std::ostringstream oss;
    for (const auto& line : lines) {
        std::vector<std::string> tokens;
        std::istringstream lss(line);
        std::string token;
        while (std::getline(lss, token, sep))
            tokens.push_back(token);

        bool first = true;
        for (int f : field_set) {
            if (f >= 1 && f <= static_cast<int>(tokens.size())) {
                if (!first) oss << sep;
                oss << tokens[f - 1];
                first = false;
            }
        }
        oss << "\n";
    }
    return oss.str();
}

// ---------------------------------------------------------------------------
// paste — Merge lines of files
// ---------------------------------------------------------------------------

static std::string paste_impl(const std::vector<std::string>& files,
                                const std::string& delimiter) {
    std::vector<std::vector<std::string>> all_lines;
    size_t max_lines = 0;

    for (const auto& file : files) {
        auto lines = read_lines(file);
        max_lines = std::max(max_lines, lines.size());
        all_lines.push_back(std::move(lines));
    }

    std::string sep = delimiter.empty() ? "\t" : delimiter;
    std::ostringstream oss;

    for (size_t i = 0; i < max_lines; i++) {
        for (size_t f = 0; f < all_lines.size(); f++) {
            if (f > 0) oss << sep;
            if (i < all_lines[f].size())
                oss << all_lines[f][i];
        }
        oss << "\n";
    }

    return oss.str();
}

// ---------------------------------------------------------------------------
// join — Join lines of two files on a common field
// ---------------------------------------------------------------------------

static std::string join_impl(const std::string& file1, const std::string& file2,
                               int field1, int field2,
                               const std::string& separator) {
    auto lines1 = read_lines(file1);
    auto lines2 = read_lines(file2);
    char sep = separator.empty() ? ' ' : separator[0];

    auto get_field = [&](const std::string& line, int field) -> std::string {
        std::istringstream iss(line);
        std::string token;
        if (sep == ' ') {
            for (int i = 0; i < field; i++) {
                if (!(iss >> token)) return "";
            }
        } else {
            std::string temp = line;
            for (int i = 1; i < field; i++) {
                auto pos = temp.find(sep);
                if (pos == std::string::npos) return "";
                temp = temp.substr(pos + 1);
            }
            auto pos = temp.find(sep);
            token = (pos != std::string::npos) ? temp.substr(0, pos) : temp;
        }
        return token;
    };

    // Build index on file2
    std::multimap<std::string, std::string> index2;
    for (const auto& line : lines2) {
        std::string key = get_field(line, field2);
        index2.emplace(key, line);
    }

    std::ostringstream oss;
    for (const auto& line : lines1) {
        std::string key = get_field(line, field1);
        auto range = index2.equal_range(key);
        for (auto it = range.first; it != range.second; ++it) {
            oss << line << sep << it->second << "\n";
        }
    }

    return oss.str();
}

// ===========================================================================
// pybind11 registration
// ===========================================================================

void init_text(py::module_ &m) {

    // -- cat ----------------------------------------------------------------
    m.def("cat", &cat_impl,
        R"doc(
        Concatenate and display file contents.

        Equivalent to the ``cat`` shell command. Reads the entire file and
        returns its contents as a string.

        Args:
            path (str): Path to the file to read.
            number_lines (bool): If True, number all output lines.
                                 Equivalent to ``cat -n``.
            squeeze_blank (bool): If True, suppress repeated empty lines.
                                  Equivalent to ``cat -s``.

        Returns:
            str: The file contents.

        Raises:
            ValueError: If the file cannot be opened.
        )doc",
        py::arg("path"),
        py::arg("number_lines") = false,
        py::arg("squeeze_blank") = false);

    // -- echo ---------------------------------------------------------------
    m.def("echo", &echo_impl,
        R"doc(
        Return a string, optionally without trailing newline.

        Equivalent to the ``echo`` shell command. Returns the provided text
        as a string.

        Args:
            text (str): The text to return.
            no_newline (bool): If True, omit the trailing newline.
                               Equivalent to ``echo -n``.

        Returns:
            str: The text with or without a trailing newline.
        )doc",
        py::arg("text"),
        py::arg("no_newline") = false);

    // -- head ---------------------------------------------------------------
    m.def("head", &head_impl,
        R"doc(
        Output the first N lines of a file.

        Equivalent to the ``head`` shell command. Returns the first N lines
        or first N bytes of a file.

        Args:
            path (str): Path to the file.
            n (int): Number of lines to return. Defaults to 10.
                     Equivalent to ``head -n``.
            bytes (int): If > 0, return first N bytes instead of lines.
                         Equivalent to ``head -c``.

        Returns:
            str: The first N lines or bytes of the file.

        Raises:
            ValueError: If the file cannot be opened.
        )doc",
        py::arg("path"),
        py::arg("n") = 10,
        py::arg("bytes") = -1);

    // -- tail ---------------------------------------------------------------
    m.def("tail", &tail_impl,
        R"doc(
        Output the last N lines of a file.

        Equivalent to the ``tail`` shell command. Returns the last N lines
        or last N bytes of a file.

        Args:
            path (str): Path to the file.
            n (int): Number of lines to return. Defaults to 10.
                     Equivalent to ``tail -n``.
            bytes (int): If > 0, return last N bytes instead of lines.
                         Equivalent to ``tail -c``.

        Returns:
            str: The last N lines or bytes of the file.

        Raises:
            ValueError: If the file cannot be opened.
        )doc",
        py::arg("path"),
        py::arg("n") = 10,
        py::arg("bytes") = -1);

    // -- grep ---------------------------------------------------------------
    m.def("grep", &grep_impl,
        R"doc(
        Search for a pattern in files.

        Equivalent to the ``grep`` shell command. Searches for lines matching
        a regex pattern within one or more files.

        Args:
            pattern (str): Regular expression pattern to search for.
            path (str): File or directory path to search in.
            ignore_case (bool): If True, perform case-insensitive matching.
                                Equivalent to ``grep -i``.
            recursive (bool): If True, search directories recursively.
                              Equivalent to ``grep -r``.
            line_numbers (bool): If True, include line numbers in results.
                                 Equivalent to ``grep -n``.
            count_only (bool): If True, return only match counts per file.
                               Equivalent to ``grep -c``.
            invert (bool): If True, select non-matching lines.
                           Equivalent to ``grep -v``.
            files_only (bool): If True, return only filenames that match.
                               Equivalent to ``grep -l``.
            whole_word (bool): If True, match whole words only.
                               Equivalent to ``grep -w``.

        Returns:
            list[dict] | dict | list[str]: Match results depending on flags.

        Raises:
            ValueError: If file doesn't exist or pattern is invalid.
        )doc",
        py::arg("pattern"),
        py::arg("path"),
        py::arg("ignore_case") = false,
        py::arg("recursive") = false,
        py::arg("line_numbers") = true,
        py::arg("count_only") = false,
        py::arg("invert") = false,
        py::arg("files_only") = false,
        py::arg("whole_word") = false);

    // -- sort ---------------------------------------------------------------
    m.def("sort_file", &sort_impl,
        R"doc(
        Sort lines of a text file.

        Equivalent to the ``sort`` shell command. Reads the file and returns
        its lines sorted according to the specified criteria.

        Args:
            path (str): Path to the file to sort.
            reverse (bool): If True, reverse the sort order.
                            Equivalent to ``sort -r``.
            numeric (bool): If True, sort by numeric value.
                            Equivalent to ``sort -n``.
            unique (bool): If True, output only unique lines.
                           Equivalent to ``sort -u``.
            key (int): Sort by the Nth field (1-indexed). 0 = entire line.
                       Equivalent to ``sort -k``.
            separator (str): Field separator character.
                             Equivalent to ``sort -t``.
            ignore_case (bool): If True, ignore case when sorting.
                                Equivalent to ``sort -f``.

        Returns:
            str: Sorted file contents.

        Raises:
            ValueError: If the file cannot be opened.
        )doc",
        py::arg("path"),
        py::arg("reverse") = false,
        py::arg("numeric") = false,
        py::arg("unique") = false,
        py::arg("key") = 0,
        py::arg("separator") = "",
        py::arg("ignore_case") = false);

    // -- diff ---------------------------------------------------------------
    m.def("diff", &diff_impl,
        R"doc(
        Compare two files line by line.

        Equivalent to the ``diff`` shell command. Uses an LCS-based algorithm
        to compute the differences between two files.

        Args:
            file1 (str): Path to the first file.
            file2 (str): Path to the second file.
            unified (bool): If True, output in unified diff format.
                            Equivalent to ``diff -u``.
            context_lines (int): Number of context lines (for future use).

        Returns:
            str: Unified or standard diff output.

        Raises:
            ValueError: If either file cannot be opened.
        )doc",
        py::arg("file1"),
        py::arg("file2"),
        py::arg("unified") = true,
        py::arg("context_lines") = 3);

    // -- cmp ----------------------------------------------------------------
    m.def("cmp", &cmp_impl,
        R"doc(
        Compare two files byte by byte.

        Equivalent to the ``cmp`` shell command. Reports the first byte and
        line number where the two files differ.

        Args:
            file1 (str): Path to the first file.
            file2 (str): Path to the second file.
            silent (bool): If True, only set the 'identical' flag, no details.
                           Equivalent to ``cmp -s``.

        Returns:
            dict: A dict with keys "identical" (bool), and optionally
                  "byte_offset", "line_number", "message".

        Raises:
            ValueError: If either file cannot be opened.
        )doc",
        py::arg("file1"),
        py::arg("file2"),
        py::arg("silent") = false);

    // -- comm ---------------------------------------------------------------
    m.def("comm", &comm_impl,
        R"doc(
        Compare two sorted files line by line.

        Equivalent to the ``comm`` shell command. Produces three sets: lines
        only in the first file, only in the second, and in both.

        Args:
            file1 (str): Path to the first sorted file.
            file2 (str): Path to the second sorted file.

        Returns:
            dict: A dict with keys "only_in_first" (list), "only_in_second" (list),
                  "in_both" (list).

        Raises:
            ValueError: If either file cannot be opened.
        )doc",
        py::arg("file1"),
        py::arg("file2"));

    // -- wc -----------------------------------------------------------------
    m.def("wc", &wc_impl,
        R"doc(
        Count lines, words, characters, and bytes in a file.

        Equivalent to the ``wc`` shell command. Returns a dictionary with
        counts for the specified file.

        Args:
            path (str): Path to the file.
            lines_only (bool): If True, return only line count.
                               Equivalent to ``wc -l``.
            words_only (bool): If True, return only word count.
                               Equivalent to ``wc -w``.
            chars_only (bool): If True, return only character count.
                               Equivalent to ``wc -m``.
            bytes_only (bool): If True, return only byte count.
                               Equivalent to ``wc -c``.

        Returns:
            dict: A dict with keys "file", "lines", "words", "chars", "bytes"
                  (or a subset based on flags).

        Raises:
            ValueError: If the file cannot be opened.
        )doc",
        py::arg("path"),
        py::arg("lines_only") = false,
        py::arg("words_only") = false,
        py::arg("chars_only") = false,
        py::arg("bytes_only") = false);

    // -- cut ----------------------------------------------------------------
    m.def("cut", &cut_impl,
        R"doc(
        Remove sections from each line of a file.

        Equivalent to the ``cut`` shell command. Extracts specified fields
        from each line, using a delimiter.

        Args:
            path (str): Path to the file.
            delimiter (str): Field delimiter character. Default is tab.
                             Equivalent to ``cut -d``.
            fields (str): Comma-separated field numbers (1-indexed) or ranges.
                          e.g. "1,3" or "2-4" or "1,3-5".
                          Equivalent to ``cut -f``.

        Returns:
            str: Extracted fields from each line.

        Raises:
            ValueError: If the file cannot be opened.
        )doc",
        py::arg("path"),
        py::arg("delimiter") = "\t",
        py::arg("fields") = "1");

    // -- paste --------------------------------------------------------------
    m.def("paste", &paste_impl,
        R"doc(
        Merge lines of files side by side.

        Equivalent to the ``paste`` shell command. Reads corresponding lines
        from multiple files and joins them with a delimiter.

        Args:
            files (list[str]): List of file paths to merge.
            delimiter (str): Delimiter between fields. Default is tab.
                             Equivalent to ``paste -d``.

        Returns:
            str: Merged lines.

        Raises:
            ValueError: If any file cannot be opened.
        )doc",
        py::arg("files"),
        py::arg("delimiter") = "\t");

    // -- join ---------------------------------------------------------------
    m.def("join", &join_impl,
        R"doc(
        Join lines of two files on a common field.

        Equivalent to the ``join`` shell command. For each pair of lines with
        a matching key field, outputs the joined line.

        Args:
            file1 (str): Path to the first file.
            file2 (str): Path to the second file.
            field1 (int): Join field in file1 (1-indexed). Default is 1.
                          Equivalent to ``join -1``.
            field2 (int): Join field in file2 (1-indexed). Default is 1.
                          Equivalent to ``join -2``.
            separator (str): Field separator. Default is space.
                             Equivalent to ``join -t``.

        Returns:
            str: Joined lines.

        Raises:
            ValueError: If either file cannot be opened.
        )doc",
        py::arg("file1"),
        py::arg("file2"),
        py::arg("field1") = 1,
        py::arg("field2") = 1,
        py::arg("separator") = "");
}
