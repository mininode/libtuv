/* Copyright 2015 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/select.h>

#include <uv.h>

#include "runner.h"


/* Do platform-specific initialization. */
int platform_init(int argc, char **argv) {

  /* Disable stdio output buffering. */
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);
  signal(SIGPIPE, SIG_IGN);

  if (realpath(argv[0], executable_path) == NULL) {
    perror("realpath");
    return -1;
  }

  return 0;
}

//-----------------------------------------------------------------------------

/* Invoke "argv[0] test-name [test-part]". Store process info in *p. */
/* Make sure that all stdio output of the processes is buffered up. */
int process_start(const char* name, const char* part, process_info_t* p,
                  int is_helper) {
  FILE* stdout_file;
  const char* arg;
  const char* args[16];
  int n;
  pid_t pid;

  stdout_file = tmpfile();
  if (!stdout_file) {
    perror("tmpfile");
    return -1;
  }

  p->terminated = 0;
  p->status = 0;

  pid = fork();

  if (pid < 0) {
    perror("fork");
    return -1;
  }

  if (pid == 0) {
    /* child */
    n = 0;

    args[n++] = executable_path;
    args[n++] = name;
    args[n++] = part;
    args[n++] = NULL;

    dup2(fileno(stdout_file), STDOUT_FILENO);
    dup2(fileno(stdout_file), STDERR_FILENO);
    execvp(args[0], (char* const*)args);
    perror("execvp()");
    _exit(127);
  }

  /* parent */
  p->pid = pid;
  p->name = strdup(name);
  p->stdout_file = stdout_file;

  return 0;
}

/* This function is run inside a pthread. We do this so that we can possibly
 * timeout.
 */
static void* dowait(void* data) {
  dowait_args* args = (dowait_args*)data;

  int i, r;
  process_info_t* p;

  for (i = 0; i < args->n; i++) {
    p = (process_info_t*)(args->vec + i * sizeof(process_info_t));
    if (p->terminated) continue;
    r = waitpid(p->pid, &p->status, 0);
    if (r < 0) {
      perror("waitpid");
      return NULL;
    }
    p->terminated = 1;
  }

  if (args->pipe[1] >= 0) {
    /* Write a character to the main thread to notify it about this. */
    ssize_t r;

    do
      r = write(args->pipe[1], "", 1);
    while (r == -1 && errno == EINTR);
  }

  return NULL;
}


/* Wait for all `n` processes in `vec` to terminate. */
/* Time out after `timeout` msec, or never if timeout == -1 */
/* Return 0 if all processes are terminated, -1 on error, -2 on timeout. */
int process_wait(process_info_t* vec, int n, int timeout) {
  int i;
  int r;
  int retval;
  process_info_t* p;
  dowait_args args;
  pthread_t tid;
  struct timeval tv;
  fd_set fds;

  args.vec = vec;
  args.n = n;
  args.pipe[0] = -1;
  args.pipe[1] = -1;

  /* The simple case is where there is no timeout */
  if (timeout == -1) {
    dowait(&args);
    return 0;
  }

  /* Hard case. Do the wait with a timeout.
   *
   * Assumption: we are the only ones making this call right now. Otherwise
   * we'd need to lock vec.
   */

  r = pipe((int*)&(args.pipe));
  if (r) {
    perror("pipe()");
    return -1;
  }

  r = pthread_create(&tid, NULL, dowait, &args);
  if (r) {
    perror("pthread_create()");
    retval = -1;
    goto terminate;
  }

  tv.tv_sec = timeout / 1000;
  tv.tv_usec = 0;

  FD_ZERO(&fds);
  FD_SET(args.pipe[0], &fds);

  r = select(args.pipe[0] + 1, &fds, NULL, NULL, &tv);

  if (r == -1) {
    perror("select()");
    retval = -1;

  } else if (r) {
    /* The thread completed successfully. */
    retval = 0;

  } else {
    /* Timeout. Kill all the children. */
    for (i = 0; i < n; i++) {
      p = (process_info_t*)(vec + i * sizeof(process_info_t));
      kill(p->pid, SIGTERM);
    }
    retval = -2;

    /* Wait for thread to finish. */
    r = pthread_join(tid, NULL);
    if (r) {
      perror("pthread_join");
      retval = -1;
    }
  }

terminate:
  close(args.pipe[0]);
  close(args.pipe[1]);
  return retval;
}

/* Return the exit code of process p. */
/* On error, return -1. */
int process_reap(process_info_t *p) {
  if (WIFEXITED(p->status)) {
    return WEXITSTATUS(p->status);
  } else  {
    return p->status; /* ? */
  }
}

int process_read_last_line(process_info_t *p, char* buffer, size_t buffer_len) {
  char* ptr;

  int r = fseek(p->stdout_file, 0, SEEK_SET);
  if (r < 0) {
    perror("fseek");
    return -1;
  }

  buffer[0] = '\0';

  while (fgets(buffer, buffer_len, p->stdout_file) != NULL) {
    for (ptr = buffer; *ptr && *ptr != '\r' && *ptr != '\n'; ptr++);
    *ptr = '\0';
  }

  if (ferror(p->stdout_file)) {
    perror("read");
    buffer[0] = '\0';
    return -1;
  }
  return 0;
}

void process_cleanup(process_info_t *p) {
  fclose(p->stdout_file);
  free(p->name);
}
