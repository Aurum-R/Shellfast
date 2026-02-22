"""Tests for the shellfast text processing module."""

import os
import tempfile
import pytest
import shellfast as sf


def create_file(tmpdir, name, content):
    path = os.path.join(tmpdir, name)
    with open(path, "w") as f:
        f.write(content)
    return path


class TestCat:
    def test_read_file(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = create_file(tmpdir, "test.txt", "hello\nworld\n")
            result = sf.cat(path)
            assert "hello" in result
            assert "world" in result

    def test_number_lines(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = create_file(tmpdir, "test.txt", "a\nb\nc\n")
            result = sf.cat(path, number_lines=True)
            assert "1" in result
            assert "2" in result

    def test_nonexistent_raises(self):
        with pytest.raises(ValueError):
            sf.cat("/nonexistent_file_12345")


class TestEcho:
    def test_basic(self):
        assert sf.echo("hello") == "hello\n"

    def test_no_newline(self):
        assert sf.echo("hello", no_newline=True) == "hello"


class TestHeadAndTail:
    def test_head_default(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            lines = "\n".join(str(i) for i in range(20)) + "\n"
            path = create_file(tmpdir, "nums.txt", lines)
            result = sf.head(path)
            assert result.strip().split("\n")[:10] == [str(i) for i in range(10)]

    def test_head_n(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            lines = "\n".join(str(i) for i in range(20)) + "\n"
            path = create_file(tmpdir, "nums.txt", lines)
            result = sf.head(path, n=3)
            assert len(result.strip().split("\n")) == 3

    def test_tail_default(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            lines = "\n".join(str(i) for i in range(20)) + "\n"
            path = create_file(tmpdir, "nums.txt", lines)
            result = sf.tail(path)
            result_lines = result.strip().split("\n")
            assert result_lines[-1] == "19"

    def test_tail_n(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            lines = "\n".join(str(i) for i in range(20)) + "\n"
            path = create_file(tmpdir, "nums.txt", lines)
            result = sf.tail(path, n=5)
            assert len(result.strip().split("\n")) == 5


class TestGrep:
    def test_basic_match(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = create_file(tmpdir, "test.txt", "hello world\nfoo bar\nhello again\n")
            result = sf.grep("hello", path)
            assert len(result) == 2

    def test_case_insensitive(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = create_file(tmpdir, "test.txt", "Hello\nhello\nHELLO\n")
            result = sf.grep("hello", path, ignore_case=True)
            assert len(result) == 3

    def test_invert(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = create_file(tmpdir, "test.txt", "a\nb\nc\n")
            result = sf.grep("a", path, invert=True)
            assert len(result) == 2

    def test_count_only(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = create_file(tmpdir, "test.txt", "a\nb\na\n")
            result = sf.grep("a", path, count_only=True)
            assert isinstance(result, dict)


class TestSort:
    def test_basic_sort(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = create_file(tmpdir, "test.txt", "cherry\napple\nbanana\n")
            result = sf.sort_file(path)
            lines = result.strip().split("\n")
            assert lines == ["apple", "banana", "cherry"]

    def test_reverse(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = create_file(tmpdir, "test.txt", "a\nb\nc\n")
            result = sf.sort_file(path, reverse=True)
            lines = result.strip().split("\n")
            assert lines == ["c", "b", "a"]

    def test_numeric(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = create_file(tmpdir, "test.txt", "10\n2\n100\n1\n")
            result = sf.sort_file(path, numeric=True)
            lines = result.strip().split("\n")
            assert lines == ["1", "2", "10", "100"]

    def test_unique(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = create_file(tmpdir, "test.txt", "a\nb\na\nc\nb\n")
            result = sf.sort_file(path, unique=True)
            lines = result.strip().split("\n")
            assert lines == ["a", "b", "c"]


class TestDiff:
    def test_identical_files(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            f1 = create_file(tmpdir, "a.txt", "hello\nworld\n")
            f2 = create_file(tmpdir, "b.txt", "hello\nworld\n")
            result = sf.diff(f1, f2)
            assert "-" not in result.split("\n", 2)[-1] or "+" not in result


class TestCmp:
    def test_identical(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            f1 = create_file(tmpdir, "a.txt", "same content")
            f2 = create_file(tmpdir, "b.txt", "same content")
            result = sf.cmp(f1, f2)
            assert result["identical"] is True

    def test_different(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            f1 = create_file(tmpdir, "a.txt", "hello")
            f2 = create_file(tmpdir, "b.txt", "world")
            result = sf.cmp(f1, f2)
            assert result["identical"] is False


class TestWc:
    def test_counts(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = create_file(tmpdir, "test.txt", "hello world\nfoo bar baz\n")
            result = sf.wc(path)
            assert result["lines"] == 2
            assert result["words"] == 5

    def test_lines_only(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = create_file(tmpdir, "test.txt", "a\nb\nc\n")
            result = sf.wc(path, lines_only=True)
            assert result["lines"] == 3
            assert "words" not in result


class TestComm:
    def test_comm(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            f1 = create_file(tmpdir, "a.txt", "a\nb\nc\n")
            f2 = create_file(tmpdir, "b.txt", "b\nc\nd\n")
            result = sf.comm(f1, f2)
            assert "a" in result["only_in_first"]
            assert "d" in result["only_in_second"]
            assert "b" in result["in_both"]


class TestCut:
    def test_basic_cut(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            path = create_file(tmpdir, "test.txt", "a:b:c\nd:e:f\n")
            result = sf.cut(path, delimiter=":", fields="2")
            lines = result.strip().split("\n")
            assert lines == ["b", "e"]
