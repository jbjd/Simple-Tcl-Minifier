import os
import tkinter as tk

tcl_path: str = tk.Tcl().eval("info library")

tcl_files = [
    entry.path
    for entry in os.scandir(tcl_path[:-6] + "tcl8\\8.4")
    if entry.is_file() and entry.path.endswith(".tm")
]
tcl_files += [
    entry.path
    for entry in os.scandir(tcl_path[:-6] + "tcl8\\8.5")
    if entry.is_file() and entry.path.endswith(".tm") and "test" not in entry.path
]
tcl_files += [
    entry.path
    for entry in os.scandir(tcl_path)
    if entry.is_file() and entry.path.endswith(".tcl") and "parray" not in entry.path
]
tcl_files += [
    entry.path
    for entry in os.scandir(tcl_path + "\\opt0.4")
    if entry.is_file() and entry.path.endswith(".tcl")
]
tcl_files += [
    entry.path
    for entry in os.scandir(tcl_path[:-6] + "tk8.6")
    if entry.is_file() and entry.path.endswith(".tcl")
]
tcl_files += [
    entry.path
    for entry in os.scandir(tcl_path[:-6] + "tk8.6\\ttk")
    if entry.is_file() and "Theme" not in entry.path
]

tcl_outs = [
    t.replace("8.6", "").replace(
        "AppData\\Local\\Programs\\Python\\Python312\\tcl", "Downloads\\tmp\\0.1.0"
    )
    for t in tcl_files
]


if __name__ == "__main__":
    from personal_simple_tcl_minifier.parse import tcl_minify

    for a, b in zip(tcl_files, tcl_outs):  # noqa: B905
        with open(a) as fp:
            s = tcl_minify(fp.read())

        os.makedirs(os.path.dirname(b), exist_ok=True)
        with open(b, "w") as fp:
            fp.write(s)
