ifeq ($(OS),Windows_NT)
	PYTHON := python
	override COMPILED_EXT := pyd
	OS_FLAGS :=
else
	PYTHON := python3
	override COMPILED_EXT := so
	OS_FLAGS := -fPIC
endif

# Base prefix ignores venv
PYTHON_BASE_PREFIX := $(shell $(PYTHON) -c "import sys;print(sys.base_prefix)")
PYTHON_VERSION := $(shell $(PYTHON) -c "import sys;print('.'.join(str(a) for a in sys.version_info[:2]))")

ifeq ($(OS),Windows_NT)
	override PYTHON_LIBS := $(PYTHON_BASE_PREFIX)/libs/
	override PYTHON_INCLUDES := $(PYTHON_BASE_PREFIX)/include/
	override PYTHON_DLL := $(shell $(PYTHON) -c "import sys;print(''.join(str(a) for a in sys.version_info[:2]))")
else
	override PYTHON_LIBS := $(PYTHON_BASE_PREFIX)/libs/python$(PYTHON_VERSION)/
	override PYTHON_INCLUDES := $(PYTHON_BASE_PREFIX)/include/python$(PYTHON_VERSION)/
	override PYTHON_DLL := $(PYTHON_VERSION)
endif

OPTIMIZATION_FLAG := -O0
override SOURCE := personal_simple_tcl_minifier
override C_SOURCE := $(SOURCE)/c_extensions
override C_INCLUDES := $(C_SOURCE)/includes
override C_PYTHON_MODULES := $(C_SOURCE)/python_modules

C_FLAGS := -L$(PYTHON_LIBS) -I$(PYTHON_INCLUDES) -lpython$(PYTHON_DLL) $(OPTIMIZATION_FLAG) -march=native -mtune=native -s -shared -Wall -Werror $(OS_FLAGS)


validate:
	ruff check .
	ruff format --check
	mypy . --check-untyped-defs
	codespell image_viewer tests compile_utils compile.py .ruff.toml README.md

build-parse:
	gcc $(C_PYTHON_MODULES)/parse.c $(C_SOURCE)/tcl_parse.c $(C_FLAGS) -I$(C_INCLUDES) -o $(SOURCE)/parse.$(COMPILED_EXT)

test:
	pytest --cov=$(SOURCE) --cov-report term-missing
