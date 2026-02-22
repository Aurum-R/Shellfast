#include "process.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstring>
#include <algorithm>

#include <signal.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>

namespace py = pybind11;
namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Helper: Read a /proc/[pid]/... file
// ---------------------------------------------------------------------------

static std::string read_proc_file(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) return "";
    std::ostringstream oss;
    oss << ifs.rdbuf();
    return oss.str();
}

// ---------------------------------------------------------------------------
// ps — List running processes
// ---------------------------------------------------------------------------

static py::list ps_impl(bool all, const std::string& sort_by) {
    py::list result;
    fs::path proc_dir("/proc");

    if (!fs::exists(proc_dir))
        throw py::value_error("ps: /proc filesystem not available");

    uid_t current_uid = getuid();
    long page_size = sysconf(_SC_PAGESIZE);
    long clk_tck = sysconf(_SC_CLK_TCK);

    // Read system uptime for CPU% calculation
    double sys_uptime = 0;
    {
        std::ifstream ifs("/proc/uptime");
        if (ifs.is_open()) ifs >> sys_uptime;
    }

    for (auto& entry : fs::directory_iterator(proc_dir)) {
        if (!entry.is_directory()) continue;
        std::string dirname = entry.path().filename().string();

        // Only numeric directories (PIDs)
        if (!std::all_of(dirname.begin(), dirname.end(), ::isdigit))
            continue;

        int pid = std::stoi(dirname);
        std::string proc_path = "/proc/" + dirname;

        // Read stat file
        std::string stat_content = read_proc_file(proc_path + "/stat");
        if (stat_content.empty()) continue;

        // Parse /proc/[pid]/stat
        // Format: pid (comm) state ppid pgrp ...
        auto comm_start = stat_content.find('(');
        auto comm_end = stat_content.rfind(')');
        if (comm_start == std::string::npos || comm_end == std::string::npos)
            continue;

        std::string command = stat_content.substr(comm_start + 1, comm_end - comm_start - 1);
        std::string after_comm = stat_content.substr(comm_end + 2);

        std::istringstream iss(after_comm);
        std::string state;
        int ppid;
        iss >> state >> ppid;

        // Skip fields to get to utime and stime (fields 14 and 15 after comm)
        std::string skip;
        for (int i = 0; i < 9; i++) iss >> skip; // pgrp, session, tty, tpgid, flags, minflt, cminflt, majflt, cmajflt
        unsigned long utime, stime;
        iss >> utime >> stime;

        // Skip to starttime (field 22)
        unsigned long cutime, cstime;
        iss >> cutime >> cstime;
        long priority, nice;
        iss >> priority >> nice;
        long num_threads;
        iss >> num_threads;
        iss >> skip; // itrealvalue
        unsigned long long starttime;
        iss >> starttime;

        // vsize and rss
        unsigned long vsize;
        long rss;
        iss >> vsize >> rss;

        // CPU percentage
        double total_time = static_cast<double>(utime + stime) / clk_tck;
        double proc_uptime = sys_uptime - (static_cast<double>(starttime) / clk_tck);
        double cpu_percent = proc_uptime > 0 ? (total_time / proc_uptime) * 100.0 : 0.0;

        // Memory
        double mem_kb = rss * (page_size / 1024.0);

        // UID check for non-all mode
        std::string uid_str = "";
        {
            std::ifstream status_ifs(proc_path + "/status");
            std::string line;
            uid_t proc_uid = 0;
            while (std::getline(status_ifs, line)) {
                if (line.find("Uid:") == 0) {
                    std::istringstream uid_iss(line.substr(4));
                    uid_iss >> proc_uid;
                    break;
                }
            }
            if (!all && proc_uid != current_uid) continue;
            uid_str = std::to_string(proc_uid);
        }

        // Read cmdline for full command
        std::string cmdline = read_proc_file(proc_path + "/cmdline");
        std::replace(cmdline.begin(), cmdline.end(), '\0', ' ');
        if (cmdline.empty()) cmdline = "[" + command + "]";

        py::dict proc;
        proc["pid"]         = pid;
        proc["ppid"]        = ppid;
        proc["command"]     = command;
        proc["cmdline"]     = cmdline;
        proc["state"]       = state;
        proc["cpu_percent"] = cpu_percent;
        proc["mem_kb"]      = mem_kb;
        proc["threads"]     = num_threads;
        proc["uid"]         = uid_str;
        proc["priority"]    = priority;
        proc["nice"]        = nice;

        result.append(proc);
    }

    // Sort
    if (!sort_by.empty()) {
        auto items = result.cast<std::vector<py::dict>>();
        std::sort(items.begin(), items.end(),
                  [&](const py::dict& a, const py::dict& b) {
                      if (sort_by == "cpu")
                          return a["cpu_percent"].cast<double>() > b["cpu_percent"].cast<double>();
                      if (sort_by == "mem")
                          return a["mem_kb"].cast<double>() > b["mem_kb"].cast<double>();
                      if (sort_by == "pid")
                          return a["pid"].cast<int>() < b["pid"].cast<int>();
                      return a["pid"].cast<int>() < b["pid"].cast<int>();
                  });
        result = py::list();
        for (auto& item : items)
            result.append(item);
    }

    return result;
}

// ---------------------------------------------------------------------------
// kill — Send signal to a process
// ---------------------------------------------------------------------------

static void kill_impl(int pid, int sig) {
    if (::kill(pid, sig) != 0) {
        throw py::value_error("kill: (" + std::to_string(pid) + ") - " +
                              std::strerror(errno));
    }
}

// ---------------------------------------------------------------------------
// killall — Kill processes by name
// ---------------------------------------------------------------------------

static py::dict killall_impl(const std::string& name, int sig) {
    int killed = 0;
    int failed = 0;

    fs::path proc_dir("/proc");
    for (auto& entry : fs::directory_iterator(proc_dir)) {
        if (!entry.is_directory()) continue;
        std::string dirname = entry.path().filename().string();
        if (!std::all_of(dirname.begin(), dirname.end(), ::isdigit))
            continue;

        std::string comm = read_proc_file("/proc/" + dirname + "/comm");
        // Remove trailing newline
        while (!comm.empty() && (comm.back() == '\n' || comm.back() == '\r'))
            comm.pop_back();

        if (comm == name) {
            int pid = std::stoi(dirname);
            if (::kill(pid, sig) == 0)
                killed++;
            else
                failed++;
        }
    }

    if (killed == 0 && failed == 0)
        throw py::value_error("killall: no process found with name '" + name + "'");

    py::dict result;
    result["killed"] = killed;
    result["failed"] = failed;
    result["name"]   = name;
    result["signal"] = sig;
    return result;
}

// ===========================================================================
// pybind11 registration
// ===========================================================================

void init_process(py::module_ &m) {

    // -- ps -----------------------------------------------------------------
    m.def("ps", &ps_impl,
        R"doc(
        List running processes.

        Equivalent to the ``ps`` shell command. Reads the /proc filesystem
        to list currently running processes with CPU, memory, and state info.

        Args:
            all (bool): If True, show processes from all users.
                        Equivalent to ``ps aux`` vs ``ps``.
            sort_by (str): Sort results by "cpu", "mem", or "pid".

        Returns:
            list[dict]: Each dict contains "pid", "ppid", "command", "cmdline",
                        "state", "cpu_percent", "mem_kb", "threads", "uid",
                        "priority", "nice".

        Raises:
            ValueError: If /proc filesystem is not available.
        )doc",
        py::arg("all") = true,
        py::arg("sort_by") = "pid");

    // -- kill ---------------------------------------------------------------
    m.def("kill", &kill_impl,
        R"doc(
        Send a signal to a process.

        Equivalent to the ``kill`` shell command. Sends the specified signal
        to a process identified by its PID.

        Args:
            pid (int): Process ID to send signal to.
            signal (int): Signal number. Common values:
                          1 (SIGHUP), 2 (SIGINT), 9 (SIGKILL),
                          15 (SIGTERM, default).
                          Equivalent to ``kill -SIGNAL PID``.

        Raises:
            ValueError: If the signal cannot be sent (e.g., no such process,
                        permission denied).
        )doc",
        py::arg("pid"),
        py::arg("signal") = 15);

    // -- killall ------------------------------------------------------------
    m.def("killall", &killall_impl,
        R"doc(
        Kill all processes matching a name.

        Equivalent to the ``killall`` shell command. Finds and sends a signal
        to all processes matching the given name.

        Args:
            name (str): Process name to match (exact match on /proc/PID/comm).
            signal (int): Signal number to send. Default is 15 (SIGTERM).
                          Equivalent to ``killall -SIGNAL name``.

        Returns:
            dict: Keys "killed" (int), "failed" (int), "name", "signal".

        Raises:
            ValueError: If no process with the given name is found.
        )doc",
        py::arg("name"),
        py::arg("signal") = 15);
}
