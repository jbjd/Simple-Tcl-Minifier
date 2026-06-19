"""Tests for memory leaks in C extension modules."""

import os

from psleak import MemoryLeakTestCase

from personal_simple_tcl_minifier.parse import tcl_minify_file, tcl_minify_folder
from tests.utils import TestFile, TmpTclFile, TmpTclFolder


class TestLeaks(MemoryLeakTestCase):
    warmup_times = 1

    times = 40

    retries = 4

    __slots__ = ("sample_large_tcl_script",)

    def setUp(self) -> None:
        self.malloc_env: str | None = os.getenv("PYTHONMALLOC")
        os.environ["PYTHONMALLOC"] = "malloc"

        if os.environ.get("PYTHONUNBUFFERED") != "1":
            raise ValueError("Need to set env variable PYTHONUNBUFFERED=1")

        self.sample_large_tcl_script: bytes = b" set   a   1\n" * 80

    def tearDown(self) -> None:
        if self.malloc_env is not None:
            os.environ["PYTHONMALLOC"] = self.malloc_env
        else:
            del os.environ["PYTHONMALLOC"]

    def test_tcl_minify_file(self) -> None:
        with TmpTclFile(TestFile("三島", self.sample_large_tcl_script)) as tmp_file:
            self.execute(tcl_minify_file, tmp_file)

    # TODO: move duplicated tmp file/folder setup to test utils
    def test_tcl_minify_folder(self) -> None:
        starting_content: bytes = self.sample_large_tcl_script

        file1 = TestFile("a.abc", starting_content)
        file2 = TestFile("tcl/tcl8.4/a.tcl", starting_content)
        file3 = TestFile("tk/a.tm", starting_content)

        with TmpTclFolder([file1, file2, file3]) as tmp_dir:
            tcl_minify_folder(tmp_dir)
