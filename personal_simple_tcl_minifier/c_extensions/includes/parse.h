#ifndef PTCL_PARSE
#define PTCL_PARSE

#include <stddef.h>

char *tcl_minify(const char *source, size_t size, size_t *size_out);

#endif /* PTCL_PARSE */