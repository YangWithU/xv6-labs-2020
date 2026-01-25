#include "kernel/types.h"
#include "user/user.h"

int xv6strlen(const char *s) {
  const char *p = s;
  while (*p) {
    p++;
  }
  return p - s;
}

void only_parent_2_child() {
  char buf[8] = {0};
  int pipefds[2] = {0};

  int ret = pipe(pipefds);
  if (ret < 0) {
    fprintf(1, "pipe() failed\n");
    exit(1);
  }

  int pid = fork();
  if (pid < 0) {
    fprintf(1, "fork() failed\n");
    exit(1);
  }

  if (pid == 0) { // child
    close(pipefds[1]);
    int readfd = dup(pipefds[0]);
    if (readfd < 0) {
      fprintf(1, "child dup() failed\n");
      exit(1);
    }

    read(readfd, buf, sizeof(buf));
    printf("%d: received ping\n", pid);

    close(pipefds[0]);
    close(readfd);
  } else {
    close(pipefds[0]);
    int writefd = dup(pipefds[1]);
    if (writefd < 0) {
      fprintf(1, "parent dup() failed\n");
      exit(1);
    }

    const char *msg = "hi\n";
    write(writefd, msg, xv6strlen(msg) + 1);
    printf("%d: sent ping\n", pid);

    close(pipefds[0]);
    close(writefd);
  }

  exit(0);
}

int main() {
  int p2c[2] = {0}, c2p[2] = {0};
  int ret = pipe(p2c);
  char msg[1] = "!";
  char rbuf[1] = {0};
  if (ret < 0) {
    fprintf(1, "pipe(p2c) failed\n");
    exit(1);
  }

  ret = pipe(c2p);
  if (ret < 0) {
    fprintf(1, "pipe(c2p) failed\n");
    exit(1);
  }

  int pid = fork();
  if (pid < 0) {
    fprintf(1, "fork() failed\n");
    exit(1);
  }

  if (pid == 0) { // child
    close(p2c[1]);
    close(c2p[0]);

    int readfd = dup(p2c[0]);
    int writefd = dup(c2p[1]);

    read(readfd, rbuf, 1);
    printf("%d: received ping\n", pid);

    write(writefd, msg, 1);

    close(readfd);
    close(writefd);
    close(p2c[0]);
    close(c2p[1]);
  } else {
    close(c2p[1]);
    close(p2c[0]);

    int readfd = dup(c2p[0]);
    int writefd = dup(p2c[1]);

    write(writefd, msg, 1);

    read(readfd, rbuf, 1);
    printf("%d: received pong\n", pid);

    wait(0);

    close(readfd);
    close(writefd);
    close(c2p[0]);
    close(p2c[1]);
  }
  exit(0);
}