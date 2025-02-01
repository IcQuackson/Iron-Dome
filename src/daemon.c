/*
 * daemon.c
 * - Contains daemon-related routines, such as daemonizing the process
 *   and handling signals.
 */

#include "common.h"
#include "daemon.h"
#include "logging.h"
#include <errno.h>

void daemonize(void)
{
    pid_t pid;

    // 1. Fork and let the parent exit
    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "fork() failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        // Parent exits
        exit(EXIT_SUCCESS);
    }

    // 2. Create new session
    if (setsid() < 0) {
        fprintf(stderr, "setsid() failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // 3. Second fork to ensure the daemon can't re-acquire a controlling terminal
    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "fork() failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        // Parent exits
        exit(EXIT_SUCCESS);
    }

    // 4. Clear file mode creation mask
    umask(0);

    // 5. Change working directory to root to avoid locking
    if (chdir("/") < 0) {
        fprintf(stderr, "chdir() failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }   

    // 6. Close standard file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Reopen them to /dev/null for safety
    open("/dev/null", O_RDONLY); // stdin
    open("/dev/null", O_WRONLY); // stdout
    open("/dev/null", O_WRONLY); // stderr
}

void signal_handler(int signum)
{
    // When SIGTERM or SIGINT is caught, set keep_running to 0
    if (signum == SIGTERM || signum == SIGINT) {
        log_message("Received signal %d, shutting down daemon...", signum);
        keep_running = 0;
    }
}
