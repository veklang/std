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
#include <string.h>
#include <unistd.h>

/* POSIX exposes the environment as a NULL-terminated array of "KEY=VALUE"
 * strings. It is not declared by any standard header under _POSIX_C_SOURCE, so
 * declare it ourselves; the linker resolves it from libc. */
extern char **environ;

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

/* Command-line arguments. The vector is captured into the always-linked runtime
 * by the emitted `main` (runtime args.h: __vek_set_args), so env.c only reads
 * those globals — it does not own argc/argv. The Vek side builds the string[]. */
size_t __vek_env_argc(void) { return (size_t)(__vek_argc < 0 ? 0 : __vek_argc); }

__vek_string *__vek_env_arg(size_t index) {
  return __vek_string_from_cstr(__vek_argv[index]);
}

/* Environment snapshot. `environ` is a NULL-terminated "KEY=VALUE" array; the
 * Vek side asks for the count, then the key and value of each entry. Splitting
 * on the first '=' happens here in C (Vek has no substring), and each half is
 * copied into an owned Vek string. A malformed entry with no '=' (rare, but
 * permitted by POSIX) is treated as a bare key with an empty value. */
size_t __vek_env_count(void) {
  size_t n = 0;
  if (environ != NULL)
    while (environ[n] != NULL)
      n++;
  return n;
}

__vek_string *__vek_env_pair_key(size_t index) {
  const char *entry = environ[index];
  const char *eq = strchr(entry, '=');
  size_t key_len = eq != NULL ? (size_t)(eq - entry) : strlen(entry);
  return __vek_string_from_literal(entry, key_len);
}

__vek_string *__vek_env_pair_value(size_t index) {
  const char *entry = environ[index];
  const char *eq = strchr(entry, '=');
  return __vek_string_from_cstr(eq != NULL ? eq + 1 : "");
}

/* Working directory change. Returns 0 on success or the errno on failure; the
 * Vek side turns a non-zero code into Err(strerror(code)). */
int32_t __vek_env_chdir(const char *path) {
  return chdir(path) == 0 ? 0 : errno;
}

__vek_string *__vek_env_strerror(int32_t code) {
  return __vek_string_from_cstr(strerror(code));
}

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
