#ifndef DAEMON_H
#define DAEMON_H

// Declarations for daemon-related functions
void daemonize(void);
void signal_handler(int signum);

#endif // DAEMON_H
