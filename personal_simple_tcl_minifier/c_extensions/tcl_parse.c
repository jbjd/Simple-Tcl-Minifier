#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "includes/tcl_parse.h"

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

static inline bool _is_comment(char c, const char *tcl_source, int index_source)
{
    // TODO: See if this can be more generous then just newline/semicolon
    return c == '#' && (index_source == 0 || tcl_source[index_source - 1] == '\n' || tcl_source[index_source - 1] == ';');
}

static inline bool _is_string(char c)
{
    return c == '"';
}

#define _append_char_to_minified(c)       \
    {                                     \
        tcl_minified[index_minified] = c; \
        ++index_minified;                 \
        ++index_source;                   \
    }
#define _append_range_to_minified(s, e)                                        \
    {                                                                          \
        size_t token_len = index_source - start;                               \
        strncpy(tcl_minified + index_minified, tcl_source + start, token_len); \
        index_minified += token_len;                                           \
    }

char *tcl_minify(const char *tcl_source, size_t source_len)
{
    size_t index_source = 0;
    char *tcl_minified = malloc((sizeof(char) * source_len) + 1);
    size_t index_minified = 0;

    int depth = 0;

    while (index_source < source_len)
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
                    ++index_source;
                }
                else
                {
                    _append_char_to_minified(current_char);
                }
            }
            else
            {
                ++index_source;
            }
        }
        else if (_is_bracket(current_char))
        {
            switch (current_char)
            {
            case '{':
                ++depth;
            case '}':
                --depth;
            }
            _append_char_to_minified(current_char);
        }
        else if (_is_backslash(current_char))
        {
            _append_char_to_minified('\\');

            if (index_source < source_len)
            {
                if (tcl_source[index_source] == '\n')
                {
                    if (index_minified > 1 && !isspace(tcl_minified[index_minified - 2]))
                    {
                        tcl_minified[index_minified - 1] = ' ';
                    }
                    else
                    {
                        --index_minified;
                    }
                    do
                    {
                        ++index_source;
                    } while (index_source < source_len && isspace(tcl_source[index_source]));
                }
                else
                {
                    _append_char_to_minified(tcl_source[index_source]);
                }
            }
        }
        else if (_is_comment(current_char, tcl_source, index_source))
        {
            const size_t start = index_source++;
            int open_bracket_count = 0;
            int close_bracket_count = 0;
            while (index_source < source_len)
            {
                switch (tcl_source[index_source])
                {
                case '\\':
                    if (index_source + 1 < source_len)
                    {
                        ++index_source;
                    }
                    ++index_source;
                    break;

                case '\n':
                    goto comment_end_while;

                case '{':
                    ++open_bracket_count;
                    ++index_source;
                    break;
                case '}':
                    ++close_bracket_count;
                    ++index_source;
                    break;
                default:
                    ++index_source;
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

            while (index_source < source_len)
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
                    if (index_source < source_len)
                    {
                        if (tcl_source[index_source] == '\n')
                        {
                            skip_whitespace = true;
                            _append_range_to_minified(start, index_source - 1);
                            start = ++index_source;
                        }
                        else
                        {
                            ++index_source;
                        }
                    }
                    break;

                case '"':
                    goto string_while_end;

                default:
                    skip_whitespace = false;
                    ++index_source;
                    break;
                }
            }
        string_while_end:
            ++index_source;
            _append_range_to_minified(start, index_source)
        }
        else
        {
            const size_t start = index_source++;
            while (index_source < source_len && !_is_delimiter(tcl_source[index_source]))
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

    tcl_minified[index_minified] = '\0';
    return tcl_minified;
}