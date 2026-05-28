import os

from setuptools import Extension, setup

os.environ["CC"] = "gcc"

PACKAGE: str = "personal_simple_tcl_minifier"
C_FOLDER: str = f"{PACKAGE}/c_extensions"

# This is stupid and I hate that I have to do it
# pip would not accept envs or args I found online to set compiler
if os.name == "nt" and not os.path.exists("setup.cfg"):
    with open("setup.cfg", "w", encoding="utf-8") as fp:
        fp.write("""[build]
compiler=mingw32""")

# Define the extension module
parse_extension = Extension(
    name=f"{PACKAGE}.parse",
    sources=[
        f"{C_FOLDER}/python_modules/parse.c",
        f"{C_FOLDER}/tcl_parse.c",
    ],
    extra_compile_args=["-O3", "-Wall"],
)

setup(
    ext_modules=[parse_extension],
    exclude_package_data={"": ["*.c", "*.h"]},
)
