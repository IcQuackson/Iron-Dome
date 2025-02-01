#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>

// Common defines
#define MAX_TARGETS 1024
#define MAX_PATH_LEN 1024

// We’ll maintain a global flag for the main loop (signal-based stop)
extern volatile sig_atomic_t keep_running;

// We also store the list of monitored paths as globals (for simplicity)
extern char monitor_targets[MAX_TARGETS][MAX_PATH_LEN];
extern int num_targets;

#endif // COMMON_H
