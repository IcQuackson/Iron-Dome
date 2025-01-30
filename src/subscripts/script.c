#include <stdio.h>
#include <stdlib.h>
#include <sys/fanotify.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

#define MAX_EVENTS 1024

void usage(const char *progname) {
	printf("Usage: %s <file_or_directory> [file_or_directory] ...\n", progname);
	fflush(stdout);
	fflush(stderr);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		usage(argv[0]);
	}

	printf("Monitoring file read access for: ");
	for (int i = 1; i < argc; i++) {
		printf("%s ", argv[i]);
	}

	fflush(stdout);
	fflush(stderr);

	// Step 1: Initialize fanotify
	int fanotify_fd = fanotify_init(FAN_CLASS_CONTENT | FAN_CLOEXEC | FAN_NONBLOCK, O_RDONLY);
	if (fanotify_fd == -1) {
		perror("fanotify_init");
		exit(EXIT_FAILURE);
	}

	// Step 2: Mark directories for monitoring
	for (int i = 1; i < argc; i++) {
		struct stat path_stat;
		if (stat(argv[i], &path_stat) == -1) {
			perror("stat");
			fprintf(stderr, "Skipping: %s\n", argv[i]);
			fflush(stdout);
			fflush(stderr);
			continue;
		}

		if (S_ISDIR(path_stat.st_mode)) {
			// Mark the directory (tracks file reads inside it)
			if (fanotify_mark(fanotify_fd, FAN_MARK_ADD | FAN_MARK_ONLYDIR,
							  FAN_ACCESS | FAN_ONDIR, AT_FDCWD, argv[i]) == -1) {
				perror("fanotify_mark (directory)");
				fprintf(stderr, "Failed to mark directory: %s\n", argv[i]);
			} else {
				printf("Monitoring directory (all files inside): %s\n", argv[i]);
			}
		} else {
			// Mark individual file
			if (fanotify_mark(fanotify_fd, FAN_MARK_ADD, FAN_ACCESS, AT_FDCWD, argv[i]) == -1) {
				perror("fanotify_mark (file)");
				fprintf(stderr, "Failed to mark file: %s\n", argv[i]);
			} else {
				printf("Monitoring file: %s\n", argv[i]);
			}
		}
		fflush(stdout);
		fflush(stderr);
	}

	struct pollfd fds;
	fds.fd = fanotify_fd;
	fds.events = POLLIN;

	// Step 3: Event loop to listen for file read access
	while (1) {
		if (poll(&fds, 1, -1) > 0) {
			struct fanotify_event_metadata buf[MAX_EVENTS];
			ssize_t len = read(fanotify_fd, buf, sizeof(buf));

			if (len > 0) {
				struct fanotify_event_metadata *event;
				for (event = buf; FAN_EVENT_OK(event, len); event = FAN_EVENT_NEXT(event, len)) {
					if (event->mask & FAN_ACCESS) {
						char path[PATH_MAX];
						snprintf(path, sizeof(path), "/proc/self/fd/%d", event->fd);
						ssize_t path_len = readlink(path, path, sizeof(path) - 1);
						if (path_len > 0) {
							path[path_len] = '\0';
							printf("File read detected: %s (PID: %d)\n", path, event->pid);
						}
						close(event->fd);
					}
					fflush(stdout);
					fflush(stderr);
				}
			}
		}
	}

	close(fanotify_fd);
	return 0;
}
