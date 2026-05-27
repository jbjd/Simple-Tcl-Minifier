from tkinter import TclError, Tk

import pytest

from personal_simple_tcl_minifier.parse import tcl_minify


def test_empty_string():
    assert tcl_minify("") == ""


def test_escaped_quote():
    source: str = 'set someVar "escaped \\"quote\\""'
    _test_minifier(source, source)


def test_semicolon_with_whitespace():
    source: str = """set foo "asfd"  ;
# Comment"""
    expected_output: str = 'set foo "asfd"'

    _test_minifier(source, expected_output)


def test_comment_with_brackets():
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


def test_comment_with_backslash():
    source: str = '''# This is all \\
              a comment
puts "asdf"'''
    expected_output: str = 'puts "asdf"'
    _test_minifier(source, expected_output)


def test_comment_with_backslash_at_end():
    source: str = "# This is all \\"
    expected_output: str = ""
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


def test_backslash_space():
    source: str = '''puts "world  \\ 
              t"'''  # noqa: W291
    expected_output: str = '''puts "world  \\ 
              t"'''  # noqa: W291
    _test_minifier(source, expected_output)


def test_backslash_literal():
    source: str = '''puts "world  \\\\
              t"'''
    expected_output: str = '''puts "world  \\\\
              t"'''
    _test_minifier(source, expected_output)


def test_backslash_literal2():
    source: str = """
if { ! true } {
    append re \\\\
}"""
    expected_output: str = """
if { ! true} {
append re \\\\}""".strip()
    _test_minifier(source, expected_output)


def test_backslash_complex():
    source: str = """proc ::tcl::clock::scan { args } {
    return -code error \\
        -errorcode [list CLOCK wrongNumArgs] \\
        "wrong \\# args: should be\\
        \\"$cmdName string\\
        ?-base seconds?\\
        ?-format string? ?-gmt boolean?\\
        ?-locale LOCALE? ?-timezone ZONE?\\""
}"""
    expected_output: str = (
        "proc ::tcl::clock::scan { args} {\n"
        "return -code error -errorcode [list CLOCK wrongNumArgs] "
        '"wrong \\# args: should be \\"$cmdName string ?-base seconds?'
        ' ?-format string? ?-gmt boolean? ?-locale LOCALE? ?-timezone ZONE?\\""}'
    )
    _test_minifier(source, expected_output)


_tk = Tk(useTk=False)


def _test_minifier(source: str, expected_output: str) -> None:
    output: str = tcl_minify(source)
    assert output == expected_output, f"{output}\n\n---!=---\n\n{expected_output}"

    try:
        _tk.eval(output)
    except TclError as e:
        raise ValueError("Minified Tcl output not valid") from e
