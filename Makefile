validate:
	ruff check .
	ruff format --check
	clang-format -n -Werror $(shell python -c "from glob import glob;print(' '.join(glob('**/*.c',recursive=True)+glob('**/*.h',recursive=True)))")
	codespell .

build-parse:
	python setup.py build_ext --inplace

test:
	python -m unittest discover tests
