C_AND_H_FILES = $(shell python -c "from glob import glob;print(' '.join(glob('personal_simple_tcl_minifier/**/*.[ch]',recursive=True)))")

format:
	ruff check . --fix
	clang-format -i $(C_AND_H_FILES)

validate:
	ruff check .
	ruff format --check
	clang-format -n -Werror $(C_AND_H_FILES)
	codespell .

build-parse:
	python setup.py build_ext --inplace

test:
	python -m unittest tests/test_parse.py

override PYTHONUNBUFFERED=1
export PYTHONUNBUFFERED

test-memory-leak:
	python -m unittest tests/test_memory_leak.py