"""Tests for memory leaks in C extension modules."""

import os
import tempfile

from psleak import MemoryLeakTestCase

from personal_simple_tcl_minifier.parse import tcl_minify_file, tcl_minify_folder


class TestLeaks(MemoryLeakTestCase):
    warmup_times = 1

    times = 20

    retries = 4

    def setUp(self) -> None:
        self.malloc_env: str | None = os.getenv("PYTHONMALLOC")
        os.environ["PYTHONMALLOC"] = "malloc"

        if os.environ.get("PYTHONUNBUFFERED") != "1":
            raise ValueError("Need to set env variable PYTHONUNBUFFERED=1")

    def tearDown(self) -> None:
        if self.malloc_env is not None:
            os.environ["PYTHONMALLOC"] = self.malloc_env
        else:
            del os.environ["PYTHONMALLOC"]

    def test_tcl_minify_file(self) -> None:
        temp_file = tempfile.NamedTemporaryFile(delete=False)  # noqa: SIM115
        try:
            try:
                temp_file.write(b" set   a   1\n" * 20)
            finally:
                temp_file.close()

            self.execute(tcl_minify_file, temp_file.name)
        finally:
            os.remove(temp_file.name)

    # TODO: move duplicated tmp file/folder setup to test utils
    def test_tcl_minify_folder(self) -> None:
        with tempfile.TemporaryDirectory() as tmp_dir:
            subdir: str = os.path.join(tmp_dir, "asdf")
            os.makedirs(subdir)
            tcl_file1 = os.path.join(tmp_dir, "a.tcl")
            tcl_file2 = os.path.join(subdir, "a.tm")
            non_tcl_file1 = os.path.join(tmp_dir, "a.abc")

            starting_content: str = " set   a   1" * 20

            with open(tcl_file1, "w") as f:
                f.write(starting_content)
            with open(tcl_file2, "w") as f:
                f.write(starting_content)
            with open(non_tcl_file1, "w") as f:
                f.write(starting_content)

            tcl_minify_folder(tmp_dir)
