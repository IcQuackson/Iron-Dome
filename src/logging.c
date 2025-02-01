/*
 * logging.c
 * - Handles creation/opening of the log file
 * - Provides log_message(...) to write messages
 */

#include "common.h"
#include "logging.h"
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>

#define LOG_DIR  "/var/log/irondome"
#define LOG_FILE "/var/log/irondome/irondome.log"

static FILE *g_log_fp = NULL;

// Initialize the logging subsystem
int init_logging(void)
{
    if (mkdir(LOG_DIR, 0755) < 0) {
        if (errno != EEXIST) {
            fprintf(stderr, "Failed to create directory %s: %s\n",
                    LOG_DIR, strerror(errno));
            return -1;
        }
    }

    g_log_fp = fopen(LOG_FILE, "a");
    if (!g_log_fp) {
        fprintf(stderr, "Failed to open log file %s: %s\n",
                LOG_FILE, strerror(errno));
        return -1;
    }

    // Set line buffering on the log file for real-time updates
    setvbuf(g_log_fp, NULL, _IOLBF, 0);

    return 0;
}

// Close the log file
void close_logging(void)
{
    if (g_log_fp) {
        fclose(g_log_fp);
        g_log_fp = NULL;
    }
}

// Write a formatted message to the log file
void log_message(const char *format, ...)
{
    if (!g_log_fp) {
        return;
    }

    va_list args;
    va_start(args, format);

    // Add a timestamp or prefix
    // Format -> [2025-02-01 12:34:56] Some message
    {
        char timebuf[64];
        time_t now = time(NULL);
        struct tm tm_info;
        localtime_r(&now, &tm_info);

        strftime(timebuf, sizeof(timebuf), "[%Y-%m-%d %H:%M:%S]", &tm_info);
        fprintf(g_log_fp, "%s ", timebuf);
    }

    vfprintf(g_log_fp, format, args);
    fprintf(g_log_fp, "\n");
    fflush(g_log_fp);

    va_end(args);
}
