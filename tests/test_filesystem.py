"""Tests for the shellfast filesystem module."""

import os
import tempfile
import pytest
import shellfast as sf


class TestPwd:
    def test_returns_current_directory(self):
        result = sf.pwd()
        assert result == os.getcwd()

    def test_returns_string(self):
        assert isinstance(sf.pwd(), str)


class TestCd:
    def test_change_to_tmp(self):
        original = sf.pwd()
        sf.cd("/tmp")
        assert sf.pwd() == "/tmp"
        sf.cd(original)

    def test_nonexistent_raises(self):
        with pytest.raises(ValueError, match="no such file"):
            sf.cd("/nonexistent_dir_12345")


class TestMkdirAndRmdir:
    def test_create_and_remove(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = os.path.join(tmpdir, "testdir")
            sf.mkdir(path)
            assert os.path.isdir(path)
            sf.rmdir(path)
            assert not os.path.exists(path)

    def test_parents(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = os.path.join(tmpdir, "a", "b", "c")
            sf.mkdir(path, parents=True)
            assert os.path.isdir(path)

    def test_rmdir_nonempty_raises(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            sf.touch(os.path.join(tmpdir, "file.txt"))
            with pytest.raises(ValueError, match="not empty"):
                sf.rmdir(tmpdir)


class TestLs:
    def test_list_tmp(self):
        result = sf.ls("/tmp")
        assert isinstance(result, list)

    def test_hidden_files(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            sf.touch(os.path.join(tmpdir, ".hidden"))
            sf.touch(os.path.join(tmpdir, "visible"))

            without_all = sf.ls(tmpdir, all=False)
            with_all = sf.ls(tmpdir, all=True)

            assert "visible" in without_all
            assert ".hidden" not in without_all
            assert ".hidden" in with_all

    def test_long_format(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            sf.touch(os.path.join(tmpdir, "file.txt"))
            result = sf.ls(tmpdir, long_format=True)
            assert len(result) > 0
            assert isinstance(result[0], dict)
            assert "name" in result[0]
            assert "permissions" in result[0]
            assert "size" in result[0]

    def test_sort_by_name(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            for name in ["ccc", "aaa", "bbb"]:
                sf.touch(os.path.join(tmpdir, name))
            result = sf.ls(tmpdir, sort_by="name")
            assert result == ["aaa", "bbb", "ccc"]

    def test_nonexistent_raises(self):
        with pytest.raises(ValueError):
            sf.ls("/nonexistent_12345")


class TestTouchAndRm:
    def test_create_file(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = os.path.join(tmpdir, "newfile.txt")
            sf.touch(path)
            assert os.path.exists(path)
            assert os.path.getsize(path) == 0

    def test_rm_file(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = os.path.join(tmpdir, "remove_me.txt")
            sf.touch(path)
            sf.rm(path)
            assert not os.path.exists(path)

    def test_rm_force_nonexistent(self):
        sf.rm("/nonexistent_file_12345", force=True)

    def test_rm_directory_without_recursive_raises(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            subdir = os.path.join(tmpdir, "subdir")
            sf.mkdir(subdir)
            sf.touch(os.path.join(subdir, "file"))
            with pytest.raises(ValueError, match="Is a directory"):
                sf.rm(subdir)

    def test_rm_recursive(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            subdir = os.path.join(tmpdir, "subdir")
            sf.mkdir(subdir)
            sf.touch(os.path.join(subdir, "file"))
            sf.rm(subdir, recursive=True)
            assert not os.path.exists(subdir)


class TestCpAndMv:
    def test_copy_file(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            src = os.path.join(tmpdir, "src.txt")
            dst = os.path.join(tmpdir, "dst.txt")
            with open(src, "w") as f:
                f.write("hello")
            sf.cp(src, dst)
            assert os.path.exists(dst)
            with open(dst) as f:
                assert f.read() == "hello"

    def test_move_file(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            src = os.path.join(tmpdir, "src.txt")
            dst = os.path.join(tmpdir, "dst.txt")
            with open(src, "w") as f:
                f.write("hello")
            sf.mv(src, dst)
            assert not os.path.exists(src)
            assert os.path.exists(dst)


class TestLn:
    def test_symbolic_link(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            target = os.path.join(tmpdir, "target.txt")
            link = os.path.join(tmpdir, "link.txt")
            sf.touch(target)
            sf.ln(target, link, symbolic=True)
            assert os.path.islink(link)

    def test_hard_link(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            target = os.path.join(tmpdir, "target.txt")
            link = os.path.join(tmpdir, "link.txt")
            sf.touch(target)
            sf.ln(target, link, symbolic=False)
            assert os.path.exists(link)
            assert os.stat(target).st_ino == os.stat(link).st_ino


class TestFind:
    def test_find_by_name(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            sf.touch(os.path.join(tmpdir, "hello.txt"))
            sf.touch(os.path.join(tmpdir, "world.py"))
            result = sf.find(tmpdir, name="*.txt")
            assert len(result) == 1
            assert "hello.txt" in result[0]

    def test_find_by_type(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            sf.touch(os.path.join(tmpdir, "file.txt"))
            sf.mkdir(os.path.join(tmpdir, "subdir"))
            dirs = sf.find(tmpdir, type="d")
            files = sf.find(tmpdir, type="f")
            assert len(dirs) == 1
            assert len(files) == 1


class TestDu:
    def test_summary(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = os.path.join(tmpdir, "file.txt")
            with open(path, "w") as f:
                f.write("x" * 100)
            result = sf.du(tmpdir, summary_only=True)
            assert "bytes" in result
            assert result["bytes"] >= 100


class TestChmod:
    def test_change_permissions(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = os.path.join(tmpdir, "file.txt")
            sf.touch(path)
            sf.chmod(path, 0o644)
            mode = os.stat(path).st_mode & 0o777
            assert mode == 0o644
