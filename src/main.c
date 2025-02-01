/*
 * main.c
 * - Entry point for the daemon.
 * - Parses command-line arguments.
 * - Defines global data.
 * - Starts the daemon.
 */

#include "common.h"
#include "daemon.h"
#include "monitor.h"
#include "logging.h"

volatile sig_atomic_t keep_running = 1;
char monitor_targets[MAX_TARGETS][MAX_PATH_LEN];
int num_targets = 0;

int main(int argc, char *argv[])
{
    // 1. Argument parsing
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <path1> [<path2> ...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    num_targets = argc - 1;
    if (num_targets > MAX_TARGETS) {
        fprintf(stderr, "Too many targets specified (max = %d)\n", MAX_TARGETS);
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i <= num_targets; i++) {
        strncpy(monitor_targets[i - 1], argv[i], MAX_PATH_LEN - 1);
        monitor_targets[i - 1][MAX_PATH_LEN - 1] = '\0'; // ensure null-termination
    }

    if (init_logging() < 0) {
        fprintf(stderr, "Failed to initialize logging. Exiting.\n");
        exit(EXIT_FAILURE);
    }

    daemonize();

    log_message("IronDome Daemon started with %d targets.", num_targets);

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    monitor_loop();

    log_message("IronDome Daemon shutting down...");

    close_logging();

    return 0;
}
