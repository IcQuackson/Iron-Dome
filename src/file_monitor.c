#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <linux/fanotify.h>
#include <sys/fanotify.h>
#include <sys/epoll.h>

// ----------------------------------
// Constants
// ----------------------------------

#define LOGBUF_SIZE      2048
#define EVENT_BUF_SIZE   8192

// ----------------------------------
// Data Structures
// ----------------------------------

typedef struct {
    // Directory we mark with fanotify (using FAN_EVENT_ON_CHILD).
    // If the user gave a file, this is that file's parent directory.
    // If the user gave a directory, it's that directory itself.
    char watch_path[2048];

    // If non-empty, we only track events for this specific file in watch_path.
    // If empty, we track everything in that directory.
    char target_path[2048];

    // Last known entropy for the target file (only used if target_path != "").
    double last_entropy;

    // 1 if the user originally asked for a directory, 0 if they asked for a file.
    int is_dir;
} file_info_t;

// ----------------------------------
// Globals
// ----------------------------------

static file_info_t *g_files = NULL;
static int g_num_files      = 0;
static volatile sig_atomic_t g_running = 1;

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------
static void log_message(const char *fmt, ...)
{
    FILE *fp = fopen("/var/log/irondome/irondome.log", "a");
    if (!fp) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    char buffer[LOGBUF_SIZE];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    time_t now = time(NULL);
    fprintf(fp, "[%ld] %s\n", now, buffer);
    fclose(fp);
}

// ---------------------------------------------------------------------------
// Daemonize
// ---------------------------------------------------------------------------
static void daemonize(void)
{
    pid_t pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        // Parent
        exit(EXIT_SUCCESS);
    }
    // Child
    if (setsid() < 0) {
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        // Parent
        exit(EXIT_SUCCESS);
    }

    umask(0);
    if (chdir("/") < 0) {
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    // Redirect standard fds to /dev/null
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_WRONLY);
    open("/dev/null", O_WRONLY);
}

// ---------------------------------------------------------------------------
// Signal Handler
// ---------------------------------------------------------------------------
static void handle_signal(int signo)
{
    (void)signo;
    g_running = 0;
}

// ---------------------------------------------------------------------------
// Entropy Calculation
// ---------------------------------------------------------------------------
static double compute_entropy(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        return -1.0;
    }

    if (fseek(f, 0, SEEK_END) < 0) {
        fclose(f);
        return -1.0;
    }
    long size = ftell(f);
    if (size <= 0) {
        fclose(f);
        return -1.0;
    }
    rewind(f);

    unsigned char *data = (unsigned char*)malloc(size);
    if (!data) {
        fclose(f);
        return -1.0;
    }
    if (fread(data, 1, size, f) != (size_t)size) {
        free(data);
        fclose(f);
        return -1.0;
    }
    fclose(f);

    unsigned long freq[256];
    memset(freq, 0, sizeof(freq));

    for (long i = 0; i < size; i++) {
        freq[data[i]]++;
    }
    free(data);

    double entropy = 0.0;
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            double p = (double)freq[i] / (double)size;
            entropy -= p * (log(p) / log(2.0));
        }
    }
    return entropy;
}

// ---------------------------------------------------------------------------
// Flexible Entropy Updater
// ---------------------------------------------------------------------------
static void apply_entropy_check(file_info_t *info, const char *path, const char *event_label)
{
    if (info->is_dir == 0 && strcmp(path, info->target_path) == 0) {
        double old_e = info->last_entropy;
        double new_e = compute_entropy(path);

        // If compute_entropy failed, skip
        if (new_e < 0) {
            return;
        }

        info->last_entropy = new_e;

        log_message("%s => %s (old=%.2f new=%.2f)", event_label, path, old_e, new_e);
        if (fabs(new_e - old_e) > 1.0) {
            log_message("ALERT: Big entropy change on %s (%.2f -> %.2f)",
                        path, old_e, new_e);
        }
        if (new_e > 7.0) {
            log_message("ALERT: High entropy on %s (%.2f) => possible crypto",
                        path, new_e);
        }
    }
    else if (info->is_dir == 1) {
        // Directory logic if you want it
    }
}


// ---------------------------------------------------------------------------
// Path Utilities
// ---------------------------------------------------------------------------
static void split_path(const char *full_path,
                       char *dir_out, size_t dir_sz,
                       char *file_out, size_t file_sz)
{
    const char *last_slash = strrchr(full_path, '/');
    if (!last_slash) {
        snprintf(dir_out,  dir_sz, ".");
        snprintf(file_out, file_sz, "%s", full_path);
        return;
    }
    size_t dir_len = last_slash - full_path;
    if (dir_len == 0) {
        snprintf(dir_out, dir_sz, "/");
    } else {
        if (dir_len >= dir_sz) {
            dir_len = dir_sz - 1;
        }
        strncpy(dir_out, full_path, dir_len);
        dir_out[dir_len] = '\0';
    }
    snprintf(file_out, file_sz, "%s", last_slash + 1);
}

static int get_path_from_fd(int fd, char *out_path, size_t max_len)
{
    char proc_path[64];
    snprintf(proc_path, sizeof(proc_path), "/proc/self/fd/%d", fd);

    ssize_t n = readlink(proc_path, out_path, max_len - 1);
    if (n < 0) {
        return -1;
    }
    out_path[n] = '\0';
    return 0;
}

// ---------------------------------------------------------------------------
// File/Directory Setup
// ---------------------------------------------------------------------------
static void init_file_info(int argc, char **argv)
{
    g_num_files = argc - 1;
    g_files = (file_info_t *)calloc(g_num_files, sizeof(file_info_t));
    if (!g_files) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < g_num_files; i++) {
        const char *input_path = argv[i + 1];
        struct stat st;
        if (stat(input_path, &st) < 0) {
            fprintf(stderr, "Warning: stat('%s') failed: %s\n",
                    input_path, strerror(errno));
        }

        if (S_ISDIR(st.st_mode)) {
            // It's a directory => watch it directly
            snprintf(g_files[i].watch_path, sizeof(g_files[i].watch_path), "%s", input_path);
            g_files[i].target_path[0] = '\0';
            g_files[i].last_entropy = -1.0;
            g_files[i].is_dir       = 1;
        } else {
            // It's a file => watch the parent directory
            char dirbuf[1024], filebuf[1024];
            split_path(input_path, dirbuf, sizeof(dirbuf), filebuf, sizeof(filebuf));
            snprintf(g_files[i].watch_path, sizeof(g_files[i].watch_path), "%s", dirbuf);
            snprintf(g_files[i].target_path, sizeof(g_files[i].target_path),
                     "%s/%s", dirbuf, filebuf);
            g_files[i].last_entropy = compute_entropy(input_path);
            g_files[i].is_dir       = 0;
        }
    }
}

// ---------------------------------------------------------------------------
// Fanotify & Epoll Setup
// ---------------------------------------------------------------------------
static int setup_fanotify(void)
{
    // We request FAN_CLASS_CONTENT so we can open file descriptors in events
    int fan_fd = fanotify_init(FAN_CLOEXEC | FAN_CLASS_CONTENT, O_RDONLY | O_CLOEXEC);
    if (fan_fd < 0) {
        log_message("fanotify_init failed: %s", strerror(errno));
        free(g_files);
        exit(EXIT_FAILURE);
    }

    // For each watch item, mark the directory with FAN_EVENT_ON_CHILD
    // so that events on children are visible. We also watch for FAN_OPEN,
    // FAN_MODIFY, etc. (You can add FAN_CLOSE_WRITE if desired.)
    for (int i = 0; i < g_num_files; i++) {
        uint64_t mask = FAN_OPEN | FAN_MODIFY | FAN_EVENT_ON_CHILD;
        unsigned int flags = FAN_MARK_ADD;

        if (fanotify_mark(fan_fd, flags, mask, AT_FDCWD, g_files[i].watch_path) < 0) {
            log_message("Failed to fanotify_mark on %s: %s",
                        g_files[i].watch_path, strerror(errno));
        } else {
            if (g_files[i].is_dir) {
                log_message("Monitoring directory %s (FAN_EVENT_ON_CHILD)",
                            g_files[i].watch_path);
            } else {
                log_message("Monitoring directory %s for file %s",
                            g_files[i].watch_path,
                            g_files[i].target_path);
            }
        }
    }

    return fan_fd;
}

static int setup_epoll(int fan_fd)
{
    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        log_message("epoll_create1 failed: %s", strerror(errno));
        close(fan_fd);
        free(g_files);
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events  = EPOLLIN;
    ev.data.fd = fan_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fan_fd, &ev) < 0) {
        log_message("epoll_ctl failed: %s", strerror(errno));
        close(epoll_fd);
        close(fan_fd);
        free(g_files);
        exit(EXIT_FAILURE);
    }

    return epoll_fd;
}

// ---------------------------------------------------------------------------
// Process a single fanotify event
// ---------------------------------------------------------------------------
static void process_fanotify_event(struct fanotify_event_metadata *metadata)
{
    // If this event was caused by our own daemon, skip it
    if (metadata->pid == getpid()) {
        if (metadata->fd >= 0) {
            close(metadata->fd);
        }
        return;
    }

    // We have a valid FD from fanotify
    char path[1024];
    if (get_path_from_fd(metadata->fd, path, sizeof(path)) == 0) {
        // Now see which watch item this path belongs to
        // We do a naive check if path starts with g_files[i].watch_path
        uint64_t mask = metadata->mask;

        for (int i = 0; i < g_num_files; i++) {
            size_t watch_len = strlen(g_files[i].watch_path);
            if (strncmp(path, g_files[i].watch_path, watch_len) == 0) {

                // If it's a directory watch, we handle all child files.
                // If there's a target_path, we only handle it if path == target_path.

                if (g_files[i].is_dir) {
                    if (mask & FAN_OPEN) {
                        log_message("OPEN (dir watch) => %s", path);
                    }
                    if (mask & FAN_MODIFY) {
                        log_message("MODIFY (dir watch) => %s", path);
                    }
                } else {
                    // Single file watch
                    // We only do checks if path == target_path
                    if (strcmp(path, g_files[i].target_path) == 0) {
                        if (mask & FAN_OPEN) {
                            apply_entropy_check(&g_files[i], path, "OPEN");
                        }
                        if (mask & FAN_MODIFY) {
                            apply_entropy_check(&g_files[i], path, "MODIFY");
                        }
                    }
                }
            }
        }
    }

    close(metadata->fd);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file_or_directory> [<file_or_directory> ...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    init_file_info(argc, argv);

    daemonize();

    signal(SIGTERM, handle_signal);
    signal(SIGINT,  handle_signal);

    int fan_fd = setup_fanotify();

    int epoll_fd = setup_epoll(fan_fd);

    log_message("Daemon started. Monitoring...");

    while (g_running) {
        struct epoll_event events[1];
        int nfds = epoll_wait(epoll_fd, events, 1, 500);
        if (nfds < 0) {
            if (errno == EINTR) {
                continue; // check g_running
            }
            log_message("epoll_wait error: %s", strerror(errno));
            break;
        }
        if (nfds == 0) {
            continue; // no events
        }

        if (events[0].data.fd == fan_fd) {
            char buffer[EVENT_BUF_SIZE];
            ssize_t len = read(fan_fd, buffer, sizeof(buffer));
            if (len < 0) {
                if (errno != EAGAIN && errno != EINTR) {
                    log_message("fanotify read error: %s", strerror(errno));
                    break;
                }
                continue;
            }

            // Process each fanotify event in this buffer
            struct fanotify_event_metadata *metadata;
            for (char *ptr = buffer; ptr < buffer + len; ) {
                metadata = (struct fanotify_event_metadata *)ptr;
                if (metadata->vers != FANOTIFY_METADATA_VERSION) {
                    log_message("Fanotify metadata version mismatch (got %u, expected %u).",
                                metadata->vers, FANOTIFY_METADATA_VERSION);
                    break;
                }

                if (metadata->fd >= 0) {
                    // Handle the event
                    process_fanotify_event(metadata);
                }

                if (metadata->event_len == 0) {
                    // No more events
                    break;
                }
                ptr += metadata->event_len;
            }
        }
    }

    close(epoll_fd);
    close(fan_fd);
    free(g_files);

    log_message("Daemon stopping.");
    return 0;
}
