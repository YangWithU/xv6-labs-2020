#pragma once

// Long-term locks for processes
#include "kernel/types.h"
#include "kernel/spinlock.h"

struct sleeplock {
  uint locked;       // Is the lock held?
  struct spinlock lk; // spinlock protecting this sleep lock
  
  // For debugging:
  char *name;        // Name of lock.
  int pid;           // Process holding lock
};

