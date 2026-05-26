import os
import tkinter as tk


def get_tcl_sources(tcl_path: str) -> list[str]:
    tcl_files: list = [entry for entry in os.scandir(tcl_path) if entry.is_file()]

    tcl_sources: list[str] = []

    for file in tcl_files:
        with open(file, encoding="utf-8") as fp:
            tcl_sources.append(fp.read())

    return tcl_sources


if __name__ == "__main__":
    import timeit

    from personal_simple_tcl_minifier.parse import tcl_minify  # noqa: F401

    tcl_path: str = tk.Tcl().eval("info library")

    tcl_sources: list[str] = get_tcl_sources(tcl_path)
    INTERATIONS: int = 10

    duration: float = timeit.timeit(
        """
for s in tcl_sources:
    tcl_minify(s)
""",
        number=INTERATIONS,
        globals=globals(),
    )

    with open("version.txt", encoding="utf-8") as fp:
        version: str = fp.read().strip()

    print(f"v{version} -", "Parsed tcl files in", tcl_path)
    print(INTERATIONS, "iterations took:", duration)
