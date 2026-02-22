# 🚀 ShellFast

**Blazing-fast C++ implementations of 50+ Linux shell commands, exposed as a Python package.**

ShellFast replaces slow `subprocess.run()` calls with native C++ implementations that use direct system calls. No shell spawning, no process overhead — just raw speed.

## 📦 Installation

```bash
# From source (requires C++17 compiler, CMake, Python 3.8+)
pip install .

# Development mode
pip install -e ".[test]"
```

### Prerequisites
- **Linux** (uses POSIX APIs, /proc filesystem)
- **C++17 compiler** (GCC 8+, Clang 7+)
- **CMake 3.15+**
- **Python 3.8+**
- **pybind11** (auto-installed by build system)

## ⚡ Quick Start

```python
import shellfast as sf

# File operations
files = sf.ls("/home", all=True, long_format=True)
sf.mkdir("/tmp/mydir", parents=True)
sf.cp("src.txt", "dst.txt", recursive=True)
sf.mv("old.txt", "new.txt")
sf.rm("temp/", recursive=True, force=True)

# Text processing
content = sf.cat("file.txt", number_lines=True)
matches = sf.grep("error", "/var/log/syslog", ignore_case=True, recursive=True)
count = sf.wc("data.txt")  # {'lines': 100, 'words': 500, 'chars': 3000, 'bytes': 3000}
sorted_text = sf.sort_file("data.csv", numeric=True, key=2, separator=",")

# System info
print(sf.whoami())         # 'user'
print(sf.uname(all=True))  # {'sysname': 'Linux', ...}
print(sf.uptime())          # {'days': 5, 'hours': 3, ...}
print(sf.free(human_readable=True))  # {'ram': {'total': '15.6G', ...}}

# Process management
procs = sf.ps(all=True, sort_by="cpu")
sf.kill(1234, signal=15)

# Networking
result = sf.ping("google.com", count=4)
dns = sf.nslookup("example.com")
ifaces = sf.ifconfig()
```

## 📋 Supported Commands (48 total)

### File & Directory (14)
`ls` · `pwd` · `cd` · `mkdir` · `rmdir` · `rm` · `touch` · `cp` · `mv` · `ln` · `find` · `du` · `chmod` · `chown`

### Text Processing (13)
`cat` · `echo` · `head` · `tail` · `grep` · `sort_file` · `diff` · `cmp` · `comm` · `wc` · `cut` · `paste` · `join`

### System Info (15)
`uname` · `whoami` · `uptime` · `env` · `getenv` · `export_env` · `unsetenv` · `clear` · `cal` · `date` · `sleep` · `id` · `groups` · `free` · `whereis`

### Process Management (3)
`ps` · `kill` · `killall`

### Networking (3)
`ping` · `nslookup` · `ifconfig`

## 🧪 Testing

```bash
pip install -e ".[test]"
pytest tests/ -v
```

## 🏗️ Architecture

```
src/
├── shellfast/          # Python package
│   ├── __init__.py     # Public API
│   ├── _core.pyi       # Type stubs
│   └── py.typed        # PEP 561 marker
└── cpp/                # C++ implementations
    ├── module.cpp       # pybind11 entry point
    ├── filesystem/      # ls, cp, mv, rm, find, etc.
    ├── text/            # cat, grep, sort, diff, wc, etc.
    ├── system/          # uname, whoami, uptime, env, etc.
    ├── process/         # ps, kill, killall
    └── network/         # ping, nslookup, ifconfig
```

## 📄 License

MIT
