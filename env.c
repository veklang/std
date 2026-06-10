/* C backing for std:env, compiled and linked when a program imports "std:env"
 * (declared by `extern "env.c";` in env.vek). Unlike io.c, env consumes the
 * runtime construction surface (__vek_string_from_cstr, __vek_panic_cstr), so
 * it includes vek_runtime.h for the *declarations* — it must NOT define
 * VEK_RUNTIME_IMPLEMENTATION (only the emitted main.c owns the single impl
 * instantiation; this TU sees the VEK_RUNTIME_API-declared half).
 *
 * POSIX-only for now; all functions operate on the host process environment,
 * which is not safe to read or write concurrently per POSIX. */
#if !defined(_POSIX_C_SOURCE) || _POSIX_C_SOURCE < 200809L
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include "vek_runtime.h"
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

/* Vek lowers nullable raw pointers as a `{is_null, value}` struct rather than
   a tagged-NULL pointer, so a single extern returning `cstr?` doesn't ABI-
   match the natural `getenv()` return. Split into two calls: `has` reports
   presence, `get_or_empty` returns the value (or a sentinel empty string if
   unset) so the result is always a valid non-null cstr. Callers use `has`
   first to distinguish "unset" from "set to empty". */
bool __vek_env_has(const char *name) { return getenv(name) != NULL; }

const char *__vek_env_get_or_empty(const char *name) {
  const char *v = getenv(name);
  return v != NULL ? v : "";
}

void __vek_env_set(const char *name, const char *value) {
  /* Ignore failure: setenv returns -1 on ENOMEM or invalid name, which we
     surface as a no-op rather than a panic — the user-visible API has no
     error channel. Callers that need detection can re-read with var(). */
  (void)setenv(name, value, 1);
}

void __vek_env_unset(const char *name) { (void)unsetenv(name); }

VEK_NORETURN void __vek_env_exit(int32_t code) { exit(code); }

VEK_NORETURN void __vek_env_abort(void) { abort(); }

__vek_string *__vek_env_cwd(void) {
  /* Start at 4096 and grow on ERANGE. POSIX doesn't bound pathname length
     reliably, so the dynamic-grow path is required for correctness on
     filesystems that allow deeper paths than PATH_MAX. */
  size_t cap = 4096;
  char *buf = (char *)malloc(cap);
  if (buf == NULL)
    __vek_panic_cstr("env.current_dir: out of memory");
  while (getcwd(buf, cap) == NULL) {
    if (errno != ERANGE) {
      free(buf);
      __vek_panic_cstr("env.current_dir: getcwd failed");
    }
    cap *= 2;
    char *grown = (char *)realloc(buf, cap);
    if (grown == NULL) {
      free(buf);
      __vek_panic_cstr("env.current_dir: out of memory");
    }
    buf = grown;
  }
  __vek_string *result = __vek_string_from_cstr(buf);
  free(buf);
  return result;
}
