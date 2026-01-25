#include "kernel/fs.h"
#include "kernel/stat.h"
#include "kernel/types.h"
#include "user/user.h"

#define BUF_SIZE 512

void find_func(char *path, char *target) {
  int fd = 0;
  if ((fd = open(path, 0)) < 0) {
    fprintf(2, "find: cannot open %s\n", path);
    exit(1);
  }

  struct stat st;
  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    exit(1);
  }

  char buf[BUF_SIZE], *p;
  switch (st.type) {
  case T_FILE:
    if (strcmp(path + strlen(path) - strlen(target), target) == 0) {
      printf("%s\n", path);
    }
    break;
  case T_DIR:
    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
      printf("find: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';
    struct dirent de;
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
      if (de.inum == 0) {
        continue;
      }
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if (stat(buf, &st) < 0) {
        printf("find: cannot stat %s\n", buf);
        continue;
      }
      if (strcmp(buf + strlen(buf) - 2, "/.") == 0) {
        continue;
      }
      if (strcmp(buf + strlen(buf) - 3, "/..") == 0) {
        continue;
      }
      find_func(buf, target);
    }
    break;
  }
  close(fd);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(1, "usage: find [path] [file_name]\n");
    exit(1);
  }

  // find的路径是一个文件，且名称和目标相同，返回
  if (strcmp(argv[1], argv[2]) == 0) { // 找的是自己,直接返回
    printf("%s\n", argv[1]);
    exit(0);
  }

  char target[BUF_SIZE] = {0};
  target[0] = '/';
  strcpy(target + 1, argv[2]);
  find_func(argv[1], target);

  exit(0);
}