#include "kernel/types.h"
#include "user/user.h"

// 需要做到的事情：
// 1. 将参数存到a0寄存器(RISC-V约定)
// 2. 用户态程序执行 ecall 暂停用户程序，切换内核模式
// 3. 内核将用户程序所有寄存器的值保存到 struct trapframe
// 4. 内核根据系统调用号 找到 sys_sleep 函数并执行

int parse_param(const char *param) {
  while (*param) {
    if (*param < '0' || *param > '9') {
      return -1;
    }
    param++;
  }
  return atoi(param);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(1, "usage: sleep [seconds]; e.g sleep 114\n");
    exit(1);
  }
  int ret = parse_param(argv[1]);
  if (ret < 0) {
    fprintf(1, "invalid argument: %s\n", argv[1]);
    exit(1);
  }
  sleep(ret);
  exit(0); // success
}