from tkinter import TclError, Tk

from personal_simple_tcl_minifier.parse import tcl_minify

_tk = Tk()


def test_escaped_quote():
    source: str = '''set someVar "escaped \\"quote\\""'''
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


def _test_minifier(source: str, expected_output: str) -> None:
    output: str = tcl_minify(source)
    assert output == expected_output

    try:
        _tk.eval(output)
    except TclError as e:
        raise ValueError("Minified Tcl output not valid") from e
