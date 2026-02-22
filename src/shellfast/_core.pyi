"""Type stubs for shellfast._core C++ extension module."""

from typing import Any, Dict, List, Optional, Union

# ── File & Directory Commands ────────────────────────────────────────────────

def ls(
    path: str = ".",
    all: bool = False,
    long_format: bool = False,
    recursive: bool = False,
    sort_by: str = "name",
    reverse: bool = False,
    human_readable: bool = False,
    directory_only: bool = False,
) -> List[Union[str, Dict[str, Any]]]:
    """List directory contents. Equivalent to ``ls``."""
    ...

def pwd() -> str:
    """Print working directory. Equivalent to ``pwd``."""
    ...

def cd(path: str) -> None:
    """Change working directory. Equivalent to ``cd``."""
    ...

def mkdir(path: str, parents: bool = False) -> None:
    """Create directory. Equivalent to ``mkdir``."""
    ...

def rmdir(path: str) -> None:
    """Remove empty directory. Equivalent to ``rmdir``."""
    ...

def rm(path: str, recursive: bool = False, force: bool = False) -> None:
    """Remove files/directories. Equivalent to ``rm``."""
    ...

def touch(path: str, no_create: bool = False) -> None:
    """Create file or update timestamp. Equivalent to ``touch``."""
    ...

def cp(src: str, dst: str, recursive: bool = False, force: bool = False, preserve: bool = False) -> None:
    """Copy files/directories. Equivalent to ``cp``."""
    ...

def mv(src: str, dst: str, force: bool = False) -> None:
    """Move/rename files. Equivalent to ``mv``."""
    ...

def ln(target: str, link_name: str, symbolic: bool = False) -> None:
    """Create hard or symbolic links. Equivalent to ``ln``."""
    ...

def find(
    path: str = ".",
    name: str = "",
    type: str = "",
    min_size: int = -1,
    max_size: int = -1,
    max_depth: int = -1,
) -> List[str]:
    """Search files in directory hierarchy. Equivalent to ``find``."""
    ...

def du(path: str = ".", human_readable: bool = False, summary_only: bool = True) -> Union[Dict[str, Any], List[Dict[str, Any]]]:
    """Estimate disk usage. Equivalent to ``du``."""
    ...

def chmod(path: str, mode: int, recursive: bool = False) -> None:
    """Change file permissions. Equivalent to ``chmod``."""
    ...

def chown(path: str, owner: str = "", group: str = "", recursive: bool = False) -> None:
    """Change file ownership. Equivalent to ``chown``."""
    ...

# ── Text Processing Commands ─────────────────────────────────────────────────

def cat(path: str, number_lines: bool = False, squeeze_blank: bool = False) -> str:
    """Display file contents. Equivalent to ``cat``."""
    ...

def echo(text: str, no_newline: bool = False) -> str:
    """Return a string. Equivalent to ``echo``."""
    ...

def head(path: str, n: int = 10, bytes: int = -1) -> str:
    """Return first N lines. Equivalent to ``head``."""
    ...

def tail(path: str, n: int = 10, bytes: int = -1) -> str:
    """Return last N lines. Equivalent to ``tail``."""
    ...

def grep(
    pattern: str,
    path: str,
    ignore_case: bool = False,
    recursive: bool = False,
    line_numbers: bool = True,
    count_only: bool = False,
    invert: bool = False,
    files_only: bool = False,
    whole_word: bool = False,
) -> Union[List[Dict[str, Any]], Dict[str, int], List[str]]:
    """Search for pattern in files. Equivalent to ``grep``."""
    ...

def sort_file(
    path: str,
    reverse: bool = False,
    numeric: bool = False,
    unique: bool = False,
    key: int = 0,
    separator: str = "",
    ignore_case: bool = False,
) -> str:
    """Sort lines of a file. Equivalent to ``sort``."""
    ...

def diff(file1: str, file2: str, unified: bool = True, context_lines: int = 3) -> str:
    """Compare two files line by line. Equivalent to ``diff``."""
    ...

def cmp(file1: str, file2: str, silent: bool = False) -> Dict[str, Any]:
    """Compare two files byte by byte. Equivalent to ``cmp``."""
    ...

def comm(file1: str, file2: str) -> Dict[str, List[str]]:
    """Compare two sorted files. Equivalent to ``comm``."""
    ...

def wc(
    path: str,
    lines_only: bool = False,
    words_only: bool = False,
    chars_only: bool = False,
    bytes_only: bool = False,
) -> Dict[str, Any]:
    """Count lines/words/chars/bytes. Equivalent to ``wc``."""
    ...

def cut(path: str, delimiter: str = "\t", fields: str = "1") -> str:
    """Extract fields from each line. Equivalent to ``cut``."""
    ...

def paste(files: List[str], delimiter: str = "\t") -> str:
    """Merge lines of files. Equivalent to ``paste``."""
    ...

def join(file1: str, file2: str, field1: int = 1, field2: int = 1, separator: str = "") -> str:
    """Join two files on a common field. Equivalent to ``join``."""
    ...

# ── System Commands ──────────────────────────────────────────────────────────

def uname(all: bool = False) -> Dict[str, str]:
    """Return system information. Equivalent to ``uname``."""
    ...

def whoami() -> str:
    """Return current username. Equivalent to ``whoami``."""
    ...

def uptime() -> Dict[str, Any]:
    """Return system uptime. Equivalent to ``uptime``."""
    ...

def env() -> Dict[str, str]:
    """Return all environment variables. Equivalent to ``env``."""
    ...

def getenv(name: str, default_val: str = "") -> Optional[str]:
    """Get single environment variable."""
    ...

def export_env(name: str, value: str, overwrite: bool = True) -> None:
    """Set an environment variable. Equivalent to ``export``."""
    ...

def unsetenv(name: str) -> None:
    """Remove an environment variable. Equivalent to ``unset``."""
    ...

def clear() -> str:
    """Return ANSI escape to clear terminal. Equivalent to ``clear``."""
    ...

def cal(month: int = -1, year: int = -1) -> str:
    """Display a calendar. Equivalent to ``cal``."""
    ...

def date(format: str = "") -> str:
    """Display current date/time. Equivalent to ``date``."""
    ...

def sleep(seconds: float) -> None:
    """Suspend execution. Equivalent to ``sleep``."""
    ...

def id(username: str = "") -> Dict[str, Any]:
    """Return user identity info. Equivalent to ``id``."""
    ...

def groups(username: str = "") -> List[str]:
    """List user groups. Equivalent to ``groups``."""
    ...

def free(human_readable: bool = False) -> Dict[str, Any]:
    """Display memory usage. Equivalent to ``free``."""
    ...

def whereis(command: str) -> Dict[str, Any]:
    """Locate binary, source, and man pages. Equivalent to ``whereis``."""
    ...

# ── Process Commands ─────────────────────────────────────────────────────────

def ps(all: bool = True, sort_by: str = "pid") -> List[Dict[str, Any]]:
    """List running processes. Equivalent to ``ps``."""
    ...

def kill(pid: int, signal: int = 15) -> None:
    """Send signal to a process. Equivalent to ``kill``."""
    ...

def killall(name: str, signal: int = 15) -> Dict[str, Any]:
    """Kill all processes by name. Equivalent to ``killall``."""
    ...

# ── Network Commands ─────────────────────────────────────────────────────────

def ping(host: str, count: int = 4, timeout: float = 2.0) -> Dict[str, Any]:
    """Send ICMP echo requests. Equivalent to ``ping``."""
    ...

def nslookup(hostname: str, ipv6: bool = False) -> Dict[str, Any]:
    """DNS hostname resolution. Equivalent to ``nslookup``."""
    ...

def ifconfig(interface_name: str = "") -> List[Dict[str, Any]]:
    """Display network interface info. Equivalent to ``ifconfig``."""
    ...
