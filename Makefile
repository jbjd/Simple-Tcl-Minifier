validate:
	ruff check .
	ruff format --check
	mypy . --check-untyped-defs
	codespell image_viewer tests compile_utils compile.py .ruff.toml README.md

test:
	pytest --cov=personal_compile_tools --cov-report term-missing
