import os
import logging
import time
import sys
from watchdog.events import FileSystemEvent, FileSystemEventHandler
from watchdog.observers import Observer
from watchdog.events import EVENT_TYPE_OPENED
import pwd

DISK_CHECK_INTERVAL = 1
DISK_READS_PER_SEC_LIMIT = 5

logger = logging.getLogger()

def handle_disk_monitor(paths):
	monitor = DiskMonitor(paths)

	logger.info("Running read monitor...")
	monitor.start()

	from main import shutdown_event
	while not shutdown_event.is_set():
		for path, count in monitor.get_reads().items():
			if count > DISK_READS_PER_SEC_LIMIT:
				logger.info(f"Possible disk read abuse detected at {path} with {count} reads in the last {DISK_CHECK_INTERVAL} second(s).")
		monitor.clear_reads()
		time.sleep(DISK_CHECK_INTERVAL)
	monitor.stop()

class DiskMonitor:
	def __init__(self, paths):
		self.paths = paths
		self.observer = Observer()
		self.read_counts = {path: 0 for path in paths} # Track reads for each resource

	def start(self):
		event_handler = self.create_event_handler()
		for path in self.paths:
			if os.path.isdir(path):
				self.observer.schedule(event_handler, path, recursive=True)
				logger.debug(f"Monitoring directory: {path}")
			elif os.path.isfile(path):
				self.observer.schedule(event_handler, os.path.dirname(path), recursive=False)
				logger.debug(f"Monitoring directory: {path}")
			else:
				logger.info(f"Path '{path}' is neither a file nor a directory. Skipping.")

		self.observer.start()
	
	def stop(self):
		self.observer.stop()
		self.observer.join()
	
	def create_event_handler(self):
		monitor = self

		class DiskClassHandler(FileSystemEventHandler):
			def on_any_event(self, event):
				for path in monitor.read_counts.keys():
					# Match event path to stored path and checks reads
					if (event.src_path == path \
						or event.src_path.startswith(path)) \
							and event.event_type == EVENT_TYPE_OPENED:
						# Get username of who accessed resource
						user = pwd.getpwuid(os.stat(event.src_path).st_uid).pw_name
						logger.debug(f"Read operation detected on {event.src_path} by {user}")
						monitor.read_counts[path] += 1

		return DiskClassHandler()

	def get_reads(self):
		return self.read_counts
	
	def clear_reads(self):
		self.read_counts = {path: 0 for path in self.paths}
