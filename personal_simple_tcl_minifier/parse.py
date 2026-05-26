def tcl_minify(source: str) -> str:
    tokens: list[str] = _tcl_parse(source)

    _tcl_optimize(tokens)

    return _tcl_unparse(tokens)


def _tcl_parse(source: str) -> list[str]:
    token_start: int = 0
    token_end: int = 0
    source_end: int = len(source)

    tokens: list[str] = []

    while token_start < source_end:
        first_char: str = source[token_start]

        if _is_whitespace_or_semicolon(first_char):
            token_start += 1
            if tokens:
                if _is_whitespace_or_semicolon(tokens[-1]):
                    if first_char == "\n" or (
                        first_char == ";" and tokens[-1] in (" ", "\t")
                    ):
                        tokens[-1] = first_char
                else:
                    tokens.append(first_char)

            continue

        if first_char in ("[", "]", "{", "}", "\\"):
            token_start += 1
            tokens.append(first_char)
            continue

        if _is_token_comment(first_char, tokens):
            comment_end: int = source.find("\n", token_start + 1)
            if comment_end == -1:
                tokens.append(source[token_start:])
                token_end = source_end
            else:
                tokens.append(source[token_start:comment_end])
                token_end = comment_end
        elif first_char == '"':
            token_end = token_start + 1
            while token_end < source_end and (
                source[token_end] != '"' or source[token_end - 1] == "\\"
            ):
                token_end += 1
            token_end += 1
            string_token: str = source[token_start:token_end]
            tokens.append(string_token)
        else:
            token_end = token_start + 1
            while token_end < source_end and not _is_delimiter(source[token_end]):
                token_end += 1
            token: str = source[token_start:token_end]
            tokens.append(token)

        token_start = token_end

    return tokens


def _tcl_optimize(tokens: list[str]) -> None:
    if not tokens:
        return

    new_tokens: list[str] = []
    depth: int = 0

    for token in tokens:
        if token == "{":
            depth += 1
        elif token == "}":
            depth -= 1

        if token == "}" and new_tokens and new_tokens[-1].isspace():
            new_tokens[-1] = token
        elif token == "\n" and (not new_tokens or new_tokens[-1] in "\n\\"):
            if new_tokens and new_tokens[-1] == "\\":
                if len(new_tokens) > 1 and new_tokens[-2].isspace():
                    new_tokens.pop()
                else:
                    new_tokens[-1] = " "
        elif _is_token_comment(token, new_tokens):
            # https://wiki.tcl-lang.org/page/Why+can+I+not+place+unmatched+braces+in+Tcl+comments
            if depth > 0:
                depth_change: int = token.count("{") - token.count("}")

                if depth_change:
                    depth += depth_change
                    new_tokens.append(token)
        else:
            new_tokens.append(token)

    while new_tokens and new_tokens[-1].isspace():
        new_tokens.pop()

    tokens[:] = new_tokens


def _tcl_unparse(tokens: list[str]) -> str:
    return "".join(tokens)


def _is_whitespace_or_semicolon(token: str) -> bool:
    return token.isspace() or token == ";"


def _is_delimiter(token: str) -> bool:
    return token.isspace() or token in (";", "[", "]", "{", "}", "\\")


def _is_token_comment(token: str, previous_tokens: list[str]) -> bool:
    return token[0] == "#" and (
        not previous_tokens or previous_tokens[-1][-1:] in ("\n", ";")
    )
