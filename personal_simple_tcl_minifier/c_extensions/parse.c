#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "includes/parse.h"

static inline bool _is_backslash(char c)
{
    return c == '\\';
}

static inline bool _is_whitespace_or_semicolon(char c)
{
    return c == ';' || isspace(c);
}

static inline bool _is_bracket(char c)
{
    switch (c)
    {
    case '{':
    case '}':
    case '[':
    case ']':
        return true;
    default:
        return false;
    }
}

static inline bool _is_delimiter(char c)
{
    switch (c)
    {
    case '\\':
    case '"':
        return true;
    default:
        return _is_bracket(c) || _is_whitespace_or_semicolon(c);
    }
}

static inline bool _is_comment(char c, const char *tcl_minified, int index_minified)
{
    // TODO: See if this can be more generous then just newline/semicolon
    return c == '#' && (index_minified == 0 || tcl_minified[index_minified - 1] == '\n' || tcl_minified[index_minified - 1] == ';');
}

static inline bool _is_string(char c)
{
    return c == '"';
}

#define _append_char_to_minified(c)         \
    {                                       \
        tcl_minified[index_minified++] = c; \
        ++index_source;                     \
    }
#define _append_range_to_minified(s, e)                                      \
    {                                                                        \
        const size_t __token_len = e - s;                                    \
        strncpy(tcl_minified + index_minified, tcl_source + s, __token_len); \
        index_minified += __token_len;                                       \
    }

char *tcl_minify(const char *tcl_source, size_t size, size_t *size_out)
{
    size_t index_source = 0;
    char *tcl_minified = malloc((sizeof(char) * size));
    size_t index_minified = 0;

    int depth = 0;

    while (index_source < size)
    {
        char current_char = tcl_source[index_source];

        if (_is_whitespace_or_semicolon(current_char))
        {
            if (index_minified > 0)
            {
                if (_is_whitespace_or_semicolon(tcl_minified[index_minified - 1]))
                {
                    if (current_char == '\n' || (current_char == ';' && tcl_minified[index_minified - 1] != '\n'))
                    {
                        tcl_minified[index_minified - 1] = current_char;
                    }
                }
                else
                {
                    tcl_minified[index_minified++] = current_char;
                }
            }

            ++index_source;
        }
        else if (_is_bracket(current_char))
        {
            switch (current_char)
            {
            case '{':
                ++depth;
                break;
            case '}':
                if (index_minified > 0 && isspace(tcl_minified[index_minified - 1]))
                {
                    --index_minified;
                }
                --depth;
                break;
            }
            _append_char_to_minified(current_char);
        }
        else if (_is_backslash(current_char))
        {
            ++index_source;
            if (index_source < size)
            {
                if (tcl_source[index_source] == '\n')
                {
                    if (index_minified > 0 && !isspace(tcl_minified[index_minified - 1]))
                    {
                        tcl_minified[index_minified++] = ' ';
                    }

                    while (index_source < size && isspace(tcl_source[index_source]))
                    {
                        ++index_source;
                    }
                }
                else
                {
                    tcl_minified[index_minified++] = '\\';
                    _append_char_to_minified(tcl_source[index_source]);
                }
            }
            else
            {
                tcl_minified[index_minified++] = '\\';
            }
        }
        else if (_is_comment(current_char, tcl_minified, index_minified))
        {
            const size_t start = index_source;
            int open_bracket_count = 0;
            int close_bracket_count = 0;
            while (++index_source < size)
            {
                switch (tcl_source[index_source])
                {
                case '\\':
                    if (index_source + 1 < size)
                    {
                        ++index_source;
                    }
                    break;

                case '\n':
                    goto comment_end_while;

                case '{':
                    ++open_bracket_count;
                    break;
                case '}':
                    ++close_bracket_count;
                    break;
                }
            }

        comment_end_while:
            // https://wiki.tcl-lang.org/page/Why+can+I+not+place+unmatched+braces+in+Tcl+comments
            if (depth > 0 && (open_bracket_count || close_bracket_count))
            {
                depth += open_bracket_count - close_bracket_count;
                _append_range_to_minified(start, index_source)
            }
        }
        else if (_is_string(current_char))
        {
            size_t start = index_source++;
            bool skip_whitespace = false;

            while (index_source < size)
            {
                char string_current_char = tcl_source[index_source];
                if (skip_whitespace && isspace(string_current_char))
                {
                    start = ++index_source;
                    continue;
                }

                switch (string_current_char)
                {
                case '\\':
                    ++index_source;
                    if (index_source < size)
                    {
                        if (tcl_source[index_source] == '\n')
                        {
                            skip_whitespace = true;
                            _append_range_to_minified(start, index_source - 1);
                            if (index_minified > 0 && !isspace(tcl_minified[index_minified - 1]))
                            {
                                _append_char_to_minified(' ');
                            }
                            start = ++index_source;
                        }
                        else
                        {
                            ++index_source;
                        }
                    }
                    break;

                case '"':
                    ++index_source;
                    goto string_while_end;

                default:
                    skip_whitespace = false;
                    ++index_source;
                    break;
                }
            }
        string_while_end:
            _append_range_to_minified(start, index_source)
        }
        else
        {
            const size_t start = index_source++;
            while (index_source < size && !_is_delimiter(tcl_source[index_source]))
            {
                ++index_source;
            }
            _append_range_to_minified(start, index_source)
        }
    }

    while (index_minified > 0 && isspace(tcl_minified[index_minified - 1]))
    {
        --index_minified;
    }

    if (size_out != NULL)
    {
        *size_out = index_minified;
    }
    return tcl_minified;
}