"""Tests for the shellfast system and process modules."""

import os
import pytest
import shellfast as sf


class TestUname:
    def test_basic(self):
        result = sf.uname()
        assert "sysname" in result
        assert "nodename" in result
        assert "release" in result
        assert "machine" in result
        assert result["sysname"] == "Linux"

    def test_all(self):
        result = sf.uname(all=True)
        assert "all" in result
        assert "Linux" in result["all"]


class TestWhoami:
    def test_returns_string(self):
        user = sf.whoami()
        assert isinstance(user, str)
        assert len(user) > 0

    def test_matches_env(self):
        user = sf.whoami()
        env_user = os.environ.get("USER", "")
        if env_user:
            assert user == env_user


class TestUptime:
    def test_has_fields(self):
        result = sf.uptime()
        assert "total_seconds" in result
        assert "formatted" in result
        assert "load_1" in result
        assert result["total_seconds"] > 0


class TestEnv:
    def test_returns_dict(self):
        result = sf.env()
        assert isinstance(result, dict)
        assert "PATH" in result

    def test_getenv(self):
        os.environ["SHELLFAST_TEST"] = "hello123"
        result = sf.getenv("SHELLFAST_TEST")
        assert result == "hello123"
        del os.environ["SHELLFAST_TEST"]

    def test_getenv_missing(self):
        result = sf.getenv("NONEXISTENT_VAR_12345_XYZ")
        assert result is None or result == ""


class TestExportEnv:
    def test_set_and_get(self):
        sf.export_env("SHELLFAST_EXPORT_TEST", "value42")
        assert sf.getenv("SHELLFAST_EXPORT_TEST") == "value42"
        sf.unsetenv("SHELLFAST_EXPORT_TEST")


class TestClear:
    def test_returns_escape(self):
        result = sf.clear()
        assert "\033[2J" in result


class TestCal:
    def test_current_month(self):
        result = sf.cal()
        assert "Su Mo Tu We Th Fr Sa" in result

    def test_specific_month(self):
        result = sf.cal(month=1, year=2025)
        assert "January 2025" in result


class TestDate:
    def test_default(self):
        result = sf.date()
        assert len(result) > 0

    def test_custom_format(self):
        result = sf.date("%Y")
        assert result.isdigit()
        assert len(result) == 4


class TestSleep:
    def test_short_sleep(self):
        import time
        start = time.time()
        sf.sleep(0.1)
        elapsed = time.time() - start
        assert elapsed >= 0.09


class TestId:
    def test_current_user(self):
        result = sf.id()
        assert "uid" in result
        assert "username" in result
        assert "gid" in result
        assert "groups" in result


class TestGroups:
    def test_current_user(self):
        result = sf.groups()
        assert isinstance(result, list)
        assert len(result) > 0


class TestFree:
    def test_basic(self):
        result = sf.free()
        assert "ram" in result
        assert "swap" in result
        assert "total" in result["ram"]

    def test_human_readable(self):
        result = sf.free(human_readable=True)
        assert isinstance(result["ram"]["total"], str)


class TestPs:
    def test_list_processes(self):
        result = sf.ps()
        assert isinstance(result, list)
        assert len(result) > 0
        assert "pid" in result[0]
        assert "command" in result[0]

    def test_current_process_appears(self):
        pid = os.getpid()
        result = sf.ps(all=True)
        pids = [p["pid"] for p in result]
        assert pid in pids


class TestWhereis:
    def test_find_ls(self):
        result = sf.whereis("ls")
        assert "binaries" in result
        assert len(result["binaries"]) > 0
