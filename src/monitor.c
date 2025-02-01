/*
 * monitor.c
 * - Implements the main monitoring loop and specific checks:
 *   1) Disk read abuse
 *   2) Crypto usage
 *   3) Entropy changes
 */

#include "common.h"
#include "monitor.h"
#include "logging.h"

void monitor_loop(void)
{
    log_message("Monitor loop started.");

    while (keep_running) {
        detect_disk_read_abuse();
        detect_crypto_abuse();
        detect_entropy_changes();

        sleep(5);
    }

    log_message("Monitor loop exiting...");
}

void detect_disk_read_abuse(void)
{
    /*
      for each target in monitor_targets:
          get current read I/O counters (via /proc/diskstats or /proc/PID/io)
          compare with stored (previous) counters
          if difference > some threshold:
              log_message("Potential read abuse detected on %s", target);
    */
    log_message("Detecting disk read abuse...");
}

void detect_crypto_abuse(void)
{
    /*
      - Check CPU usage or hardware performance counters for crypto instructions
      if above threshold => log_message("High cryptographic activity");
    */
    log_message("Detecting crypto abuse...");
}

void detect_entropy_changes(void)
{
    /*
      for each file target in monitor_targets:
          if it's a file:
             compute partial/quick entropy measure
             if difference > threshold => log_message("Entropy changed on %s", target);
    */
    log_message("Detecting entropy changes...");
}
