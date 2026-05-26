from tkinter import TclError, Tk

import pytest

from personal_simple_tcl_minifier.parse import tcl_minify


def test_empty_string():
    assert tcl_minify("") == ""


def test_escaped_quote():
    source: str = 'set someVar "escaped \\"quote\\""'
    _test_minifier(source, source)


def test_comments_with_brackets():
    source: str = """#This comment can be removed {{{
switch b {
    # match a {
     puts "Matched a" }
    b { puts "Matched b" }
}
"""
    expected_output: str = """switch b {
# match a {
puts "Matched a"}
b { puts "Matched b"}}"""

    _test_minifier(source, expected_output)


def test_semicolon_with_whitespace():
    source: str = """set foo "asfd"  ;
# Comment"""
    expected_output: str = 'set foo "asfd"'

    _test_minifier(source, expected_output)


@pytest.mark.parametrize(
    "source",
    [
        """set foo \\
    1""",
        """set foo\\
    1""",
    ],
)
def test_blackslash_newline(source: str):
    expected_output: str = "set foo 1"

    _test_minifier(source, expected_output)


_tk = Tk(useTk=False)


def _test_minifier(source: str, expected_output: str) -> None:
    output: str = tcl_minify(source)
    assert output == expected_output

    try:
        _tk.eval(output)
    except TclError as e:
        raise ValueError("Minified Tcl output not valid") from e
