#ifndef MONITOR_H
#define MONITOR_H

// Declarations for monitor-related functions
void monitor_loop(void);
void detect_disk_read_abuse(void);
void detect_crypto_abuse(void);
void detect_entropy_changes(void);

#endif // MONITOR_H
