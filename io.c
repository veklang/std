/* C backing for std:io, compiled and linked when a program imports "std:io"
 * (declared by `extern "io.c";` in io.vek). Pure libc — no Vek runtime header
 * dependency, since these take and return plain `const char *`. */
#include <stdio.h>

void __vek_io_write_stdout_cstr(const char *message) {
  fputs(message != NULL ? message : "(null)", stdout);
}

void __vek_io_write_stderr_cstr(const char *message) {
  fputs(message != NULL ? message : "(null)", stderr);
}
