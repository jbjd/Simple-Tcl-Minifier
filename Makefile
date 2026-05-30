validate:
	ruff check .
	ruff format --check
	mypy . --check-untyped-defs
	codespell .

build-parse:
	python setup.py build_ext --inplace

test:
	python -m unittest discover tests
