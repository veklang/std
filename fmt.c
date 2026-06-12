/* C backing for std:fmt, compiled and linked when a program imports "std:fmt"
 * (declared by `extern "fmt.c";` in fmt.vek). Only the float formatters are
 * C-backed — Vek has no host printf — and they consume the runtime construction
 * surface (__vek_string_from_cstr, __vek_panic_cstr), so this TU includes
 * vek_runtime.h for the *declarations* and must NOT define
 * VEK_RUNTIME_IMPLEMENTATION (the emitted main.c owns the single impl). */
#include "vek_runtime.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/* snprintf into a heap buffer sized exactly to the formatted length, then hand
 * an owned __vek_string back to Vek. `spec` is "%.*f" (fixed) or "%.*e"
 * (scientific). Non-finite values format as "NaN" / "inf" / "-inf" per spec,
 * independent of the host libc's printf spelling (glibc would print "nan",
 * "inf"). */
static __vek_string *fmt_with(const char *spec, double value, size_t precision) {
  if (isnan(value))
    return __vek_string_from_cstr("NaN");
  if (isinf(value))
    return __vek_string_from_cstr(value < 0.0 ? "-inf" : "inf");

  int prec = precision > (size_t)INT_MAX ? INT_MAX : (int)precision;
  int needed = snprintf(NULL, 0, spec, prec, value);
  if (needed < 0)
    __vek_panic_cstr("fmt: float formatting failed");

  size_t size = (size_t)needed + 1;
  char *buf = (char *)malloc(size);
  if (buf == NULL)
    __vek_panic_cstr("fmt: out of memory");
  (void)snprintf(buf, size, spec, prec, value);

  __vek_string *result = __vek_string_from_cstr(buf);
  free(buf);
  return result;
}

__vek_string *__vek_fmt_float(double value, size_t precision) {
  return fmt_with("%.*f", value, precision);
}

__vek_string *__vek_fmt_float_exp(double value, size_t precision) {
  return fmt_with("%.*e", value, precision);
}
