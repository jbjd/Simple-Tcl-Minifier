import os
import sys

if sys.platform != "win32":
    os.environ["CC"] = "gcc"

from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext

PACKAGE: str = "personal_simple_tcl_minifier"
C_FOLDER: str = f"{PACKAGE}/c_extensions"


class CustomBuildExt(build_ext):
    def finalize_options(self) -> None:
        super().finalize_options()

        if sys.platform == "win32":
            self.compiler = "mingw32"


args: list[str] = [
    "-O3",
    "-march=native",
    "-mtune=native",
    "-flto",
    "-Wall",
    "-Werror",
]

PTCL_DISALLOW_UTF8_CHECK: str = "PTCL_DISALLOW_UTF8_CHECK"

if sys.platform == "win32" and not os.environ.get(PTCL_DISALLOW_UTF8_CHECK):
    import ctypes

    # Check if code page in UTF-8
    if ctypes.windll.kernel32.GetACP() == 65001:
        print(
            "\n-----\n"
            "WARNING: Will compile using UTF-8 file paths since it seems UTF-8 support "
            "is enabled in windows settings. If you have issues, "
            f"compile with env {PTCL_DISALLOW_UTF8_CHECK} set to 1"
            "\n-----\n",
        )
        args.append("-DPTCL_UTF8")

# Define the extension module
parse_extension = Extension(
    name=PACKAGE + ".parse",
    sources=[
        C_FOLDER + "/python_modules/parse.c",
        C_FOLDER + "/parse.c",
    ],
    extra_compile_args=args,
)

setup(
    ext_modules=[parse_extension],
    exclude_package_data={"": ["*.c", "*.h"]},
    cmdclass={"build_ext": CustomBuildExt},
)
