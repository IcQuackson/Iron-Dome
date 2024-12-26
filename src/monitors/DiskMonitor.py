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
		if monitor.get_reads() > DISK_READS_PER_SEC_LIMIT:
			logger.info(f"Disk read abuse detected with {monitor.get_reads()} reads in the last {DISK_CHECK_INTERVAL} second(s).")
		monitor.clear_reads()
		time.sleep(DISK_CHECK_INTERVAL)

class DiskMonitor:
	def __init__(self, paths):
		self.paths = paths
		self.observer = Observer()
		self.reads = 0

	def start(self):
		self.timestamp = time.time()
		for path in self.paths:
			event_handler = FileSystemEventHandler()
			event_handler.on_any_event =  self.handle_event
			self.observer.schedule(event_handler, path, recursive=True)
		self.observer.start()

	def handle_event(self, event):
		if event.event_type == EVENT_TYPE_OPENED:
			self.last_read = time.time()
			# get the user who accessed the file
			user = pwd.getpwuid(os.stat(event.src_path).st_uid).pw_name

			logger.debug(f"Read operation detected on {event.src_path} by {user}")
			self.reads += 1
	
	def get_reads(self):
		return self.reads
	
	def clear_reads(self):
		self.reads = 0
