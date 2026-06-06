import os
import sys

from setuptools import Extension, setup

PACKAGE: str = "personal_simple_tcl_minifier"
C_FOLDER: str = f"{PACKAGE}/c_extensions"

# This is stupid and I hate that I have to do it
# pip would not accept envs or args I found online to set compiler
if os.name == "nt" and not os.path.exists("setup.cfg"):
    with open("setup.cfg", "w", encoding="utf-8") as fp:
        fp.write("""[build]
compiler=mingw32""")


args: list[str] = ["-O3", "-march=native", "-mtune=native", "-Wall"]

if (
    sys.platform == "win32"
    and not os.environ.get("PTCL_DISALLOW_UTF8_CHECK")
    and sys.getfilesystemencoding().lower() == "utf-8"
):
    print(
        "\n-----\n"
        "WARNING: Will compile using UTF-8 file paths since it seems UTF-8 support "
        "is enabled in windows settings. If you have issues, "
        "compile with env PTCL_DISALLOW_UTF8_CHECK set to 1"
        "\n-----\n",
    )
    args.append("-DPTCL_UTF8")

# Define the extension module
parse_extension = Extension(
    name=f"{PACKAGE}.parse",
    sources=[
        f"{C_FOLDER}/python_modules/parse.c",
        f"{C_FOLDER}/parse.c",
    ],
    extra_compile_args=args,
)

setup(
    ext_modules=[parse_extension],
    exclude_package_data={"": ["*.c", "*.h"]},
)
