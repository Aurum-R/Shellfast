#include <pybind11/pybind11.h>

namespace py = pybind11;

// Forward declarations for each submodule initializer
void init_filesystem(py::module_ &m);
void init_text(py::module_ &m);
void init_system(py::module_ &m);
void init_process(py::module_ &m);
void init_network(py::module_ &m);

PYBIND11_MODULE(_core, m) {
    m.doc() = R"doc(
        ShellFast Core C++ Module
        =========================

        This is the compiled C++ backend for the shellfast package.
        It provides blazing-fast, native implementations of 50+ Linux
        shell commands without spawning subprocesses.

        Categories:
            - Filesystem: ls, pwd, cd, mkdir, rm, cp, mv, touch, ln, find, du, chmod, chown
            - Text:       cat, echo, head, tail, grep, sort, diff, cmp, comm, wc, cut, paste, join
            - System:     uname, whoami, uptime, env, export, clear, cal, date, sleep, id, groups, free, whereis
            - Process:    ps, kill, killall
            - Network:    ping, nslookup, ifconfig
    )doc";

    init_filesystem(m);
    init_text(m);
    init_system(m);
    init_process(m);
    init_network(m);
}
