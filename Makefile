validate:
	ruff check .
	ruff format --check
	mypy . --check-untyped-defs
	codespell image_viewer tests compile_utils compile.py .ruff.toml README.md

test:
	pytest --cov=personal_simple_tcl_minifier --cov-report term-missing
