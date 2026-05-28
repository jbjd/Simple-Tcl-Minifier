validate:
	ruff check .
	ruff format --check
	mypy . --check-untyped-defs
	codespell image_viewer tests compile_utils compile.py .ruff.toml README.md

build-parse:
	python setup.py build_ext --inplace

test:
	pytest --cov=$(SOURCE) --cov-report term-missing
