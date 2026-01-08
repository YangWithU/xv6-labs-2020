#include "kernel/param.h"
#include "kernel/types.h"
#include "user/user.h"

#define ONE_LINE_MAX 1024

uint32 getline(char *buf) {
  uint32 n_read = 0;
  char c;

  while (read(0, &c, 1)) {
    if (n_read >= ONE_LINE_MAX) {
      fprintf(2, "stdin line too long!\n");
      exit(1);
    }

    if (c == '\n') {
      break;
    }
    buf[n_read++] = c;
  };

  buf[n_read] = 0;
  return n_read;
}

int parse_stdin(char *buf, char **dst, int arg_left) {
  int arg_cnt = 0;
  char *start = 0;
  while (*buf && arg_cnt < arg_left) {
    // skip leading
    while (*buf == ' ') {
      buf++;
    }

    if (*buf == '\0') {
      break;
    }

    start = buf;

    while (*buf != '\0' && *buf != ' ') {
      buf++;
    }

    dst[arg_cnt++] = start;

    if (*buf != '\0') {
      *buf = '\0';
      buf++;
    }
  }
  if (*buf != '\0' && arg_cnt >= arg_left) {
    fprintf(2, "xargs: too many args!\n");
    exit(1);
  }
  return arg_cnt;
}

void printarg(int n, char **args) {
  for (int i = 0; i < n; i++) {
    printf("%s ", args[i]);
  }
  printf("\n");
}

// 从stdin读取所有内容，解析，将内容append到xargs自己的argv
// echo hello too | xargs echo bye
int main(int argc, char *argv[]) {
  if (argc <= 1) {
    fprintf(1, "usage: xargs [command] [args...]");
    exit(1);
  }

  // for (int i = 0; i < argc; i++) {
  //   printf("argv[%d]: %s ", i, argv[i]);
  // }
  // printf("\n");

  char buf[ONE_LINE_MAX] = {0};
  while (getline(buf)) {
    char *tot_args[MAXARG] = {0};

    memmove(tot_args, argv + 1, sizeof(char *) * (argc - 1)); // save argv
    // int ret = parse_stdin(buf, tot_args + argc - 1, MAXARG - argc + 1);
    // printarg(ret + argc - 1, tot_args);

    parse_stdin(buf, tot_args + argc - 1, MAXARG - argc + 1);
    int pid = fork();
    if (pid == 0) {
      exec(argv[1], tot_args); // 还得把自己名字带上...
    } else if (pid > 0) {
      wait(0);
    } else {
      fprintf(2, "fork failed!\n");
      exit(1);
    }
  }
  exit(0);
}