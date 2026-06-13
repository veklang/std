/* C backing for std:fmt, compiled and linked when a program imports "std:fmt"
 * (declared by `extern "fmt.c";` in fmt.vek). Only the float formatters are
 * C-backed — Vek has no host printf.
 *
 * These return a plain, caller-owned `cstr` (a malloc'd, NUL-terminated buffer);
 * the Vek side adopts it with `string.adopt_cstr`, which takes ownership without
 * copying and frees it on drop. That keeps this TU within the ordinary,
 * function-only C ABI: it never touches the runtime construction surface, so it
 * does not include vek_runtime.h and needs no trusted-source privilege. On any
 * allocation failure the function returns NULL, which adopt_cstr reports as a
 * null-source panic. */
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* malloc a fresh NUL-terminated copy of `s`, or NULL on allocation failure. */
static char *dup_cstr(const char *s) {
  size_t size = strlen(s) + 1;
  char *out = (char *)malloc(size);
  if (out != NULL)
    memcpy(out, s, size);
  return out;
}

/* snprintf into an exactly-sized malloc'd buffer and return it for the caller to
 * own. `spec` is "%.*f" (fixed) or "%.*e" (scientific). Non-finite values format
 * as "NaN" / "Infinity" / "-Infinity" per spec — matching Vek's float literal
 * keywords and the default Format output, not the host libc's "nan"/"inf".
 * Returns NULL on failure. */
static char *fmt_with(const char *spec, double value, size_t precision) {
  if (isnan(value))
    return dup_cstr("NaN");
  if (isinf(value))
    return dup_cstr(value < 0.0 ? "-Infinity" : "Infinity");

  int prec = precision > (size_t)INT_MAX ? INT_MAX : (int)precision;
  int needed = snprintf(NULL, 0, spec, prec, value);
  if (needed < 0)
    return NULL;

  size_t size = (size_t)needed + 1;
  char *buf = (char *)malloc(size);
  if (buf == NULL)
    return NULL;
  (void)snprintf(buf, size, spec, prec, value);
  return buf;
}

const char *__vek_fmt_float(double value, size_t precision) {
  return fmt_with("%.*f", value, precision);
}

const char *__vek_fmt_float_exp(double value, size_t precision) {
  return fmt_with("%.*e", value, precision);
}
