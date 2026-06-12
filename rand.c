/* C backing for std:rand, compiled and linked when a program imports "std:rand"
 * (declared by `extern "rand.c";` in rand.vek). Provides only OS entropy for
 * `from_os()`; the generator itself is pure Vek.
 *
 * Includes vek_runtime.h for the __vek_panic_cstr declaration (the spec has
 * from_os panic when the host entropy source is unavailable, and a u64 return
 * carries no error channel). It must NOT define VEK_RUNTIME_IMPLEMENTATION — the
 * emitted main.c owns the single impl instantiation; this TU sees only the
 * VEK_RUNTIME_API-declared half. */
#include "vek_runtime.h"
#include <stdint.h>
#include <stdio.h>

/* Read 8 bytes of entropy from /dev/urandom as a u64. Two independent calls
 * yield independent generators. Panics if the source cannot be opened or does
 * not yield a full 8 bytes. Uses stdio (no POSIX feature macros needed); the
 * path is the portable POSIX entropy source named by the spec. */
uint64_t __vek_rand_os_u64(void) {
  FILE *source = fopen("/dev/urandom", "rb");
  if (source == NULL)
    __vek_panic_cstr("rand.from_os: host entropy source unavailable");

  uint64_t value = 0;
  size_t got = fread(&value, 1, sizeof(value), source);
  fclose(source);

  if (got != sizeof(value))
    __vek_panic_cstr("rand.from_os: host entropy read failed");
  return value;
}
