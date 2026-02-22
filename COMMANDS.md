# ShellFast — Complete Command Reference

> **48 Linux commands** implemented natively in C++ with pybind11 bindings.  
> Every function listed below is callable as `shellfast.<function_name>(...)`.

---

## 1. File & Directory Commands (14)

### `ls` — List directory contents
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Show hidden | `all=True` | `ls -a` | Include files starting with `.` |
| Long format | `long_format=True` | `ls -l` | Return dicts with name, path, type, permissions, owner, group, size, last_modified, symlink_target |
| Recursive | `recursive=True` | `ls -R` | List subdirectories recursively |
| Sort by size | `sort_by="size"` | `ls -S` | Sort entries by file size |
| Sort by time | `sort_by="time"` | `ls -t` | Sort entries by last modification time |
| Sort by name | `sort_by="name"` | `ls` (default) | Alphabetical sort |
| Reverse | `reverse=True` | `ls -r` | Reverse sort order |
| Human sizes | `human_readable=True` | `ls -h` | Show sizes as K/M/G |
| Dirs only | `directory_only=True` | `ls -d` | Only list directories |

**Returns:** `list[str]` or `list[dict]` (when `long_format=True`)

---

### `pwd` — Print working directory
No flags. Returns the absolute path of the current working directory.

**Returns:** `str`

---

### `cd` — Change directory
| Argument | Description |
|----------|-------------|
| `path` | Target directory path |

**Raises:** `ValueError` if path doesn't exist or is not a directory.

---

### `mkdir` — Create directories
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Parents | `parents=True` | `mkdir -p` | Create parent directories as needed |

---

### `rmdir` — Remove empty directory
No flags. Removes only empty directories.

**Raises:** `ValueError` if directory is not empty, doesn't exist, or is not a directory.

---

### `rm` — Remove files and directories
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Recursive | `recursive=True` | `rm -r` | Remove directories and contents recursively |
| Force | `force=True` | `rm -f` | Ignore nonexistent files, never error |

---

### `touch` — Create file or update timestamp
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| No create | `no_create=True` | `touch -c` | Don't create file if it doesn't exist |

---

### `cp` — Copy files and directories
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Recursive | `recursive=True` | `cp -r` | Copy directories recursively |
| Force | `force=True` | `cp -f` | Overwrite existing files |
| Preserve | `preserve=True` | `cp -P` | Preserve symlinks (don't follow) |

---

### `mv` — Move or rename files
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Force | `force=True` | `mv -f` | Overwrite destination without prompting |

---

### `ln` — Create links
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Symbolic | `symbolic=True` | `ln -s` | Create symbolic (soft) link instead of hard link |

---

### `find` — Search for files in a directory hierarchy
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Name pattern | `name="*.txt"` | `find -name` | Glob pattern matching (prefix `*x`, suffix `x*`, contains `*x*`, exact) |
| Type filter | `type="f"` | `find -type` | `"f"` = file, `"d"` = directory, `"l"` = symlink |
| Min size | `min_size=1024` | `find -size +N` | Minimum file size in bytes |
| Max size | `max_size=1048576` | `find -size -N` | Maximum file size in bytes |
| Max depth | `max_depth=3` | `find -maxdepth` | Maximum directory recursion depth |

**Returns:** `list[str]` (matching file paths)

---

### `du` — Estimate disk usage
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Human readable | `human_readable=True` | `du -h` | Sizes as K/M/G strings |
| Summary only | `summary_only=True` | `du -s` | Return only total for the path |

**Returns:** `dict` (with keys `path`, `bytes`, `human`) or `list[dict]` per subdirectory.

---

### `chmod` — Change file permissions
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Mode | `mode=0o755` | `chmod 755` | Octal permission bits |
| Recursive | `recursive=True` | `chmod -R` | Apply to all files/subdirectories |

---

### `chown` — Change file ownership
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Owner | `owner="user"` | `chown user` | New owner username |
| Group | `group="grp"` | `chown :grp` | New group name |
| Recursive | `recursive=True` | `chown -R` | Apply recursively |

> **Note:** Requires root privileges.

---

## 2. Text Processing Commands (13)

### `cat` — Display file contents
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Number lines | `number_lines=True` | `cat -n` | Prefix each line with its number |
| Squeeze blank | `squeeze_blank=True` | `cat -s` | Suppress repeated empty lines |

**Returns:** `str`

---

### `echo` — Return a string
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| No newline | `no_newline=True` | `echo -n` | Omit trailing newline |

**Returns:** `str`

---

### `head` — First N lines of a file
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Lines | `n=10` | `head -n` | Number of lines to return (default 10) |
| Bytes | `bytes=100` | `head -c` | Return first N bytes instead |

**Returns:** `str`

---

### `tail` — Last N lines of a file
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Lines | `n=10` | `tail -n` | Number of lines to return (default 10) |
| Bytes | `bytes=100` | `tail -c` | Return last N bytes instead |

**Returns:** `str`

---

### `grep` — Search for patterns in files
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Case insensitive | `ignore_case=True` | `grep -i` | Match regardless of case |
| Recursive | `recursive=True` | `grep -r` | Search directories recursively |
| Line numbers | `line_numbers=True` | `grep -n` | Include line numbers in results (default True) |
| Count only | `count_only=True` | `grep -c` | Return only match counts per file |
| Invert match | `invert=True` | `grep -v` | Select non-matching lines |
| Files only | `files_only=True` | `grep -l` | Return only filenames that contain a match |
| Whole word | `whole_word=True` | `grep -w` | Match whole words only |

**Returns:** `list[dict]` (with `file`, `line_number`, `line`) or `dict` (counts) or `list[str]` (filenames)

---

### `sort_file` — Sort lines of a file
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Reverse | `reverse=True` | `sort -r` | Reverse sort order |
| Numeric | `numeric=True` | `sort -n` | Sort by numeric value |
| Unique | `unique=True` | `sort -u` | Output only unique lines |
| Key field | `key=2` | `sort -k` | Sort by Nth field (1-indexed, 0=whole line) |
| Separator | `separator=","` | `sort -t` | Field separator character |
| Ignore case | `ignore_case=True` | `sort -f` | Case-insensitive sorting |

**Returns:** `str`

---

### `diff` — Compare two files line by line
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Unified | `unified=True` | `diff -u` | Unified diff format (default True) |
| Context lines | `context_lines=3` | `diff -U N` | Context lines around changes |

Uses an **LCS-based** (Longest Common Subsequence) diff algorithm.

**Returns:** `str`

---

### `cmp` — Compare two files byte by byte
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Silent | `silent=True` | `cmp -s` | Only return `identical` flag, no details |

**Returns:** `dict` with keys `identical` (bool), `byte_offset`, `line_number`, `message`.

---

### `comm` — Compare two sorted files
No flags. Returns three sets of lines.

**Returns:** `dict` with keys `only_in_first`, `only_in_second`, `in_both` (each `list[str]`).

---

### `wc` — Word, line, character, and byte count
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Lines only | `lines_only=True` | `wc -l` | Return only line count |
| Words only | `words_only=True` | `wc -w` | Return only word count |
| Chars only | `chars_only=True` | `wc -m` | Return only character count |
| Bytes only | `bytes_only=True` | `wc -c` | Return only byte count |

**Returns:** `dict` with keys `file`, `lines`, `words`, `chars`, `bytes` (or subset).

---

### `cut` — Extract fields from each line
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Delimiter | `delimiter=":"` | `cut -d` | Field delimiter (default tab) |
| Fields | `fields="1,3"` | `cut -f` | Comma-separated field numbers or ranges like `"2-4"` |

**Returns:** `str`

---

### `paste` — Merge lines of files side by side
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Delimiter | `delimiter=","` | `paste -d` | Delimiter between merged fields (default tab) |

Takes `files: list[str]`.

**Returns:** `str`

---

### `join` — Join two files on a common field
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Field 1 | `field1=1` | `join -1` | Join field in first file (1-indexed) |
| Field 2 | `field2=1` | `join -2` | Join field in second file (1-indexed) |
| Separator | `separator=","` | `join -t` | Field separator character |

**Returns:** `str`

---

## 3. System Info Commands (15)

### `uname` — System information
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| All | `all=True` | `uname -a` | Include combined info string |

**Returns:** `dict` with keys `sysname`, `nodename`, `release`, `version`, `machine`, and optionally `all`.

---

### `whoami` — Current username
No flags.

**Returns:** `str`

---

### `uptime` — System uptime and load averages
No flags.

**Returns:** `dict` with keys `total_seconds`, `days`, `hours`, `minutes`, `seconds`, `formatted`, `load_1`, `load_5`, `load_15`.

---

### `env` — All environment variables
No flags.

**Returns:** `dict[str, str]` mapping variable names to values.

---

### `getenv` — Get single environment variable
| Argument | Description |
|----------|-------------|
| `name` | Variable name |
| `default_val` | Default if not set |

**Returns:** `str` or `None`

---

### `export_env` — Set environment variable
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Overwrite | `overwrite=True` | — | Overwrite if variable exists (default True) |

Equivalent to `export VAR=value`.

---

### `unsetenv` — Remove environment variable
Equivalent to `unset VAR`.

---

### `clear` — Clear terminal screen
No flags. Returns ANSI escape sequence `\033[2J\033[H`.

**Returns:** `str`

---

### `cal` — Display calendar
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Month | `month=6` | `cal -m` | Month number 1–12 (default: current) |
| Year | `year=2025` | `cal 2025` | Year (default: current) |

**Returns:** `str` (formatted calendar)

---

### `date` — Display date and time
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Format | `format="%Y-%m-%d"` | `date +FORMAT` | strftime-compatible format string |

Default format: `"%a %b %e %H:%M:%S %Z %Y"`

**Returns:** `str`

---

### `sleep` — Suspend execution
| Argument | Description |
|----------|-------------|
| `seconds` | Duration (supports fractional, e.g. `0.5`) |

---

### `id` — User identity information
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Username | `username=""` | `id user` | Lookup specific user (empty = current user) |

**Returns:** `dict` with keys `uid`, `username`, `gid`, `group`, `groups` (list of `{gid, name}` dicts).

---

### `groups` — List user's group memberships
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Username | `username=""` | `groups user` | Lookup specific user (empty = current user) |

**Returns:** `list[str]`

---

### `free` — Memory usage information
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Human readable | `human_readable=True` | `free -h` | Show sizes as K/M/G strings |

Reads `/proc/meminfo`. Returns RAM and swap stats.

**Returns:** `dict` with keys `ram` and `swap`, each containing `total`, `used`, `free`, `available`, `buffers`, `cached`.

---

### `whereis` — Locate binary, source, and man pages
| Argument | Description |
|----------|-------------|
| `command` | Command name to search for |

Searches `$PATH` for binaries, `/usr/share/man` etc. for man pages, `/usr/src` for sources.

**Returns:** `dict` with keys `command`, `binaries` (list), `man_pages` (list), `sources` (list).

---

## 4. Process Management Commands (3)

### `ps` — List running processes
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| All users | `all=True` | `ps aux` | Show processes from all users (default True) |
| Sort by | `sort_by="cpu"` | `ps --sort` | Sort by `"cpu"`, `"mem"`, or `"pid"` |

Reads `/proc/[pid]/stat` and `/proc/[pid]/status` for each process.

**Returns:** `list[dict]` — each dict contains:

| Key | Type | Description |
|-----|------|-------------|
| `pid` | int | Process ID |
| `ppid` | int | Parent process ID |
| `command` | str | Process name |
| `cmdline` | str | Full command line |
| `state` | str | Process state (R/S/D/Z/T) |
| `cpu_percent` | float | CPU usage percentage |
| `mem_kb` | float | Memory usage in KB |
| `threads` | int | Number of threads |
| `uid` | str | User ID |
| `priority` | int | Process priority |
| `nice` | int | Nice value |

---

### `kill` — Send signal to a process
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Signal | `signal=15` | `kill -SIGNAL` | Signal number: 1=SIGHUP, 2=SIGINT, 9=SIGKILL, 15=SIGTERM (default) |

---

### `killall` — Kill all processes by name
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Signal | `signal=15` | `killall -SIGNAL` | Signal number (default 15=SIGTERM) |

Matches on `/proc/[pid]/comm` (exact match).

**Returns:** `dict` with keys `killed`, `failed`, `name`, `signal`.

---

## 5. Networking Commands (3)

### `ping` — Send ICMP echo requests
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Count | `count=4` | `ping -c` | Number of echo requests (default 4) |
| Timeout | `timeout=2.0` | `ping -W` | Per-request timeout in seconds |

> **Note:** Requires `CAP_NET_RAW` or root for ICMP sockets. Falls back to DNS resolution only if raw sockets are unavailable.

**Returns:** `dict` with keys `host`, `ip`, `reachable`, `packets_sent`, `packets_received`, `packet_loss`, `rtt_min_ms`, `rtt_avg_ms`, `rtt_max_ms`.

---

### `nslookup` — DNS hostname resolution
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| IPv6 | `ipv6=True` | `nslookup -type=AAAA` | Prefer IPv6 addresses |

**Returns:** `dict` with keys `hostname`, `addresses` (list of `{address, family}` dicts), `canonical_name`.

---

### `ifconfig` — Network interface information
| Flag | Argument | Shell Equivalent | Description |
|------|----------|------------------|-------------|
| Interface | `interface_name="eth0"` | `ifconfig eth0` | Query specific interface (empty = all) |

Uses `getifaddrs()` and reads `/sys/class/net/` for MAC and MTU.

**Returns:** `list[dict]` — each dict contains:

| Key | Type | Description |
|-----|------|-------------|
| `name` | str | Interface name (e.g. `eth0`) |
| `ipv4_address` | str | IPv4 address |
| `ipv4_netmask` | str | Subnet mask |
| `ipv4_broadcast` | str | Broadcast address |
| `ipv6_address` | str | IPv6 address |
| `mac_address` | str | Hardware (MAC) address |
| `mtu` | int | Maximum transmission unit |
| `is_up` | bool | Interface is UP |
| `is_running` | bool | Interface is RUNNING |
| `is_loopback` | bool | Interface is loopback |

---

## Summary

| Category | Count | Commands |
|----------|-------|----------|
| File & Directory | 14 | ls, pwd, cd, mkdir, rmdir, rm, touch, cp, mv, ln, find, du, chmod, chown |
| Text Processing | 13 | cat, echo, head, tail, grep, sort_file, diff, cmp, comm, wc, cut, paste, join |
| System Info | 15 | uname, whoami, uptime, env, getenv, export_env, unsetenv, clear, cal, date, sleep, id, groups, free, whereis |
| Process Mgmt | 3 | ps, kill, killall |
| Networking | 3 | ping, nslookup, ifconfig |
| **Total** | **48** | |
