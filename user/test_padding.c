#include "kernel/stat.h"
#include "kernel/types.h"
#include "user/user.h"

#include <stddef.h>

#define INPUT_BUF 128
#define MAX_HISTORY 10

struct spinlock {
  unsigned int locked;
  char *name;
  struct cpu *cpu;
};

// 测试不同的结构体布局
struct cons_original {
  struct spinlock lock;
  char buf[INPUT_BUF];
  unsigned int r;
  unsigned int w;
  unsigned int e;
  char history[MAX_HISTORY][INPUT_BUF];
};

struct cons_optimized {
  struct spinlock lock;
  unsigned int r;
  unsigned int w;
  unsigned int e;
  char buf[INPUT_BUF];
  char history[MAX_HISTORY][INPUT_BUF];
};

int main() {
  printf("原始布局:\n");
  printf("  sizeof(spinlock) = %d\n", sizeof(struct spinlock));
  printf("  offsetof(buf) = %d\n", offsetof(struct cons_original, buf));
  printf("  offsetof(r) = %d\n", offsetof(struct cons_original, r));
  printf("  offsetof(w) = %d\n", offsetof(struct cons_original, w));
  printf("  offsetof(e) = %d\n", offsetof(struct cons_original, e));
  printf("  offsetof(history) = %d\n", offsetof(struct cons_original, history));
  printf("  total size = %d\n\n", sizeof(struct cons_original));

  printf("优化布局:\n");
  printf("  sizeof(spinlock) = %d\n", sizeof(struct spinlock));
  printf("  offsetof(r) = %d\n", offsetof(struct cons_optimized, r));
  printf("  offsetof(w) = %d\n", offsetof(struct cons_optimized, w));
  printf("  offsetof(e) = %d\n", offsetof(struct cons_optimized, e));
  printf("  offsetof(buf) = %d\n", offsetof(struct cons_optimized, buf));
  printf("  offsetof(history) = %d\n",
         offsetof(struct cons_optimized, history));
  printf("  total size = %d\n", sizeof(struct cons_optimized));

  exit(0);
}