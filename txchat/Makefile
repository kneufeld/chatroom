.PHONY: test pep8

test:
	py.test --cov=txchat --cov-report term-missing test/test_*.py
	coverage html

venv: Makefile setup.py
	virtualenv venv
	./venv/bin/pip install --editable .[dev]

pep8:
	pep8 *.py
