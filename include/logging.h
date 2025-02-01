#ifndef LOGGING_H
#define LOGGING_H

// Initialize the logging system, creating dirs/files if needed.
int init_logging(void);

// Close the logging file (cleanup).
void close_logging(void);

// Log a formatted message to /var/log/irondome/irondome.log
void log_message(const char *format, ...);

#endif // LOGGING_H
