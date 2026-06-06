import os
import tempfile
import unittest
from tkinter import TclError, Tk

from personal_simple_tcl_minifier.parse import tcl_minify, tcl_minify_file


class ParseTests(unittest.TestCase):
    def setUp(self):
        self._tk = Tk(baseName="test", useTk=False)

    def test_empty_string(self):
        assert tcl_minify("") == ""

    def test_escaped_quote(self):
        source: str = 'set someVar "escaped \\"quote\\""'
        self._test_minifier(source, source)

    def test_semicolon(self):
        source: str = """
proc a {} {
    set a 1; # a
}"""
        expected_output: str = """proc a {} {
set a 1}"""

        self._test_minifier(source, expected_output)

    def test_semicolon_with_whitespace(self):
        source: str = """set foo "asfd"  ;
# Comment"""
        expected_output: str = 'set foo "asfd"'

        self._test_minifier(source, expected_output)

    def test_comment_with_brackets(self):
        source: str = """#This comment can be removed {{{
switch b {
    # match a {
    set a "Matched a" }
    b { set a "Matched b" }
}
"""
        expected_output: str = """switch b {
# match a {
set a "Matched a"}
b { set a "Matched b"}}"""

        self._test_minifier(source, expected_output)

    def test_comment_with_backslash(self):
        source: str = '''# This is all \\
            a comment
set a "asdf"'''
        expected_output: str = 'set a "asdf"'
        self._test_minifier(source, expected_output)

    def test_comment_with_backslash_at_end(self):
        source: str = "# This is all \\"
        expected_output: str = ""
        self._test_minifier(source, expected_output)

    def test_comment_in_function(self):
        source: str = """
proc do {some} {
    # foo bar
    return 0}"""
        expected_output: str = """
proc do {some} {
return 0}""".strip()
        self._test_minifier(source, expected_output)

    def test_blackslash_newline(self):
        source: str = """set foo \\
        1"""
        expected_output: str = "set foo 1"

        self._test_minifier(source, expected_output)

    def test_blackslash_newline2(self):
        source: str = """set foo\\
        1"""
        expected_output: str = "set foo 1"

        self._test_minifier(source, expected_output)

    def test_blackslash_at_start(self):
        source: str = """\\
set foo 1"""
        expected_output: str = "set foo 1"

        self._test_minifier(source, expected_output)

    def test_backslash_space(self):
        source: str = '''set a "world  \\ 
            t"'''  # noqa: W291
        expected_output: str = '''set a "world  \\ 
            t"'''  # noqa: W291
        self._test_minifier(source, expected_output)

    def test_backslash_literal(self):
        source: str = '''set s "world  \\\\
            t"'''
        expected_output: str = '''set s "world  \\\\
            t"'''
        self._test_minifier(source, expected_output)

    def test_backslash_literal2(self):
        source: str = """
if { ! true } {
    append re \\\\
}"""
        expected_output: str = """
if { ! true} {
append re \\\\}""".strip()
        self._test_minifier(source, expected_output)

    def test_backslash_complex(self):
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
        self._test_minifier(source, expected_output)

    def test_broken_string(self):
        source: str = '"asdf'
        self._test_minifier(source, source, False)

    def test_minify_file(self):
        # Ensure non-ascii handled
        temp_file = tempfile.NamedTemporaryFile(prefix="三島", delete=False)  # noqa: SIM115
        try:
            try:
                temp_file.write(b" set   a   1")
            finally:
                temp_file.close()

            tcl_minify_file(temp_file.name)

            with open(temp_file.name, encoding="utf-8") as fp:
                assert fp.read() == "set a 1"
        finally:
            os.remove(temp_file.name)

    def _test_minifier(
        self, source: str, expected_output: str, validate: bool = True
    ) -> None:
        output: str = tcl_minify(source)
        assert output == expected_output, f"{output}\n\n---!=---\n\n{expected_output}"

        if validate:
            try:
                self._tk.eval(output)
            except TclError as e:
                raise ValueError("Minified Tcl output not valid") from e
