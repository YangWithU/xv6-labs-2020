#include "kernel/types.h"
#include "user/user.h"

void sieve(int *pleft) {
  int cur_prime = 0;
  int ret = read(pleft[0], &cur_prime, sizeof(int));
  if (ret == 0) {
    fprintf(1, "EOF\n");
    exit(0);
  } else if (ret < 0) {
    fprintf(1, "read() failed\n");
    exit(1);
  }

  printf("prime %d\n", cur_prime);
  int pright[2] = {0};
  ret = pipe(pright);
  if (ret < 0) {
    fprintf(1, "pipe(pright) failed\n");
    exit(1);
  }

  int pid = fork();
  if (pid < 0) {
    fprintf(1, "child fork() failed\n");
    exit(1);
  }

  if (pid > 0) { // parent
    close(pright[0]);
    int n_read = 0, cur_num = 0;
    while ((n_read = read(pleft[0], &cur_num, sizeof(int))) > 0) {
      if (cur_num % cur_prime != 0) {
        write(pright[1], &cur_num, sizeof(int));
      }
    }

    // 写完所有数据，关闭管道并等待子进程结束
    close(pright[1]);
    wait(0);
    exit(0);
  } else {
    close(pright[1]);
    close(pleft[0]);
    // close(pleft[1]); 这里必须不能关闭fd,因为fd会重用,递归之后下次read就失败了

    sieve(pright);
  }
}

int main() {
  int fds[2] = {0};
  int ret = pipe(fds);
  if (ret < 0) {
    fprintf(1, "pipe(fds) failed\n");
    exit(1);
  }

  int pid = fork();
  if (pid < 0) {
    fprintf(1, "initial fork() failed\n");
    exit(1);
  }

  if (pid == 0) { // child
    close(fds[1]);
    sieve(fds);
  } else {
    close(fds[0]);
    for (int i = 2; i <= 35; i++) {
      write(fds[1], &i, sizeof(int));
    }
    close(fds[1]);
    wait(0);
  }
  exit(0);
}