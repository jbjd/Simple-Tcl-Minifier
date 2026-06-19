import os
import shutil
import tempfile
from collections.abc import Iterable
from typing import IO


class TestFile:
    __slots__ = ("content", "expected", "name")

    def __init__(
        self, name: str, content: bytes, expected: bytes | None = None
    ) -> None:
        self.name: str = name
        self.content: bytes = content
        self.expected: bytes = expected


class TmpTclFile:
    __slots__ = ("_test_file", "_tmp_file")

    def __init__(self, test_file: TestFile) -> None:
        self._test_file: TestFile = test_file
        self._tmp_file: IO[bytes]

    def __enter__(self) -> str:
        self._tmp_file = tempfile.NamedTemporaryFile(
            prefix=self._test_file.name, delete=False
        )

        try:
            self._tmp_file.write(self._test_file.content)
            self._tmp_file.close()
        except OSError:
            self._tmp_file.close()
            self._clean_up()
            raise

        return self._tmp_file.name

    def __exit__(self, exc_type, exc_val, exc_tb) -> None:  # noqa: ANN001
        if exc_type is None and self._test_file.expected is not None:
            with open(self._tmp_file.name, "rb") as fp:
                assert fp.read() == self._test_file.expected

        self._clean_up()

        return False  # Do not suppress exception if present

    def _clean_up(self) -> None:
        os.remove(self._tmp_file.name)


class TmpTclFolder:
    __slots__ = ("_temp_folder", "_test_files")

    def __init__(self, test_files: Iterable[TestFile]) -> None:
        self._test_files: Iterable[TestFile] = test_files
        self._temp_folder: IO[bytes]

    def __enter__(self) -> str:
        self._temp_folder = tempfile.TemporaryDirectory(delete=False)

        try:
            for test_file in self._test_files:
                dir_name: str = os.path.dirname(test_file.name)
                if dir_name:
                    os.makedirs(
                        os.path.join(self._temp_folder.name, dir_name),
                        exist_ok=True,
                    )
                with open(
                    os.path.join(self._temp_folder.name, test_file.name), "wb"
                ) as fp:
                    fp.write(test_file.content)
        except OSError:
            self._clean_up()
            raise

        return self._temp_folder.name

    def __exit__(self, exc_type, exc_val, exc_tb) -> None:  # noqa: ANN001
        self._clean_up()

        return False  # Do not suppress exception if present

    def _clean_up(self) -> None:
        shutil.rmtree(self._temp_folder.name)
