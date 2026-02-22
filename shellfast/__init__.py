"""
ShellFast — Blazing-fast C++ implementations of 50+ Linux shell commands.

This package provides native C++ implementations of common Linux shell
commands, exposed as Python functions. Instead of spawning subprocesses,
ShellFast uses direct system calls and the C++ standard library, delivering
significant performance improvements.

Categories:
    - **File & Directory**: ls, pwd, cd, mkdir, rmdir, rm, touch, cp, mv, ln, find, du, chmod, chown
    - **Text Processing**: cat, echo, head, tail, grep, sort_file, diff, cmp, comm, wc, cut, paste, join
    - **System Info**: uname, whoami, uptime, env, getenv, export_env, unsetenv, clear, cal, date, sleep, id, groups, free, whereis
    - **Process Management**: ps, kill, killall
    - **Networking**: ping, nslookup, ifconfig

Example:
    >>> import shellfast as sf
    >>> sf.pwd()
    '/home/user'
    >>> files = sf.ls("/tmp", all=True, long_format=True)
    >>> matches = sf.grep("error", "/var/log/syslog", ignore_case=True)
    >>> info = sf.uname(all=True)
"""

__version__ = "0.1.0"
__author__ = "rajar"

from shellfast._core import (
    # ── File & Directory Commands ──────────────────────────────────────────
    ls,
    pwd,
    cd,
    mkdir,
    rmdir,
    rm,
    touch,
    cp,
    mv,
    ln,
    find,
    du,
    chmod,
    chown,

    # ── Text Processing Commands ──────────────────────────────────────────
    cat,
    echo,
    head,
    tail,
    grep,
    sort_file,
    diff,
    cmp,
    comm,
    wc,
    cut,
    paste,
    join,

    # ── System Info Commands ──────────────────────────────────────────────
    uname,
    whoami,
    uptime,
    env,
    getenv,
    export_env,
    unsetenv,
    clear,
    cal,
    date,
    sleep,
    id,
    groups,
    free,
    whereis,

    # ── Process Management Commands ───────────────────────────────────────
    ps,
    kill,
    killall,

    # ── Networking Commands ───────────────────────────────────────────────
    ping,
    nslookup,
    ifconfig,
)

__all__ = [
    # File & Directory
    "ls", "pwd", "cd", "mkdir", "rmdir", "rm", "touch",
    "cp", "mv", "ln", "find", "du", "chmod", "chown",
    # Text Processing
    "cat", "echo", "head", "tail", "grep", "sort_file",
    "diff", "cmp", "comm", "wc", "cut", "paste", "join",
    # System
    "uname", "whoami", "uptime", "env", "getenv", "export_env",
    "unsetenv", "clear", "cal", "date", "sleep", "id", "groups",
    "free", "whereis",
    # Process
    "ps", "kill", "killall",
    # Network
    "ping", "nslookup", "ifconfig",
]
