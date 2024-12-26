import time
import logging
import os
import psutil

logger = logging.getLogger()

def handle_memory_monitor():
	monitor = MemoryMonitor()

	from main import shutdown_event
	while not shutdown_event.is_set():
		logger.info(f"Memory usage: {monitor.get_memory_usage()} MB")

		if monitor.has_memory_exceeded_limit():
			logger.info(f"Memory has exceed the {monitor.get_memory_limit()} MB limit. Shuting down...")
			os._exit(1)
			
		time.sleep(2)

class MemoryMonitor:
	def __init__(self, memory_limit_mb=100):
		self.memory_limit_mb = memory_limit_mb

	def get_memory_limit(self):
		return self.memory_limit_mb
	
	def get_memory_usage(self):
		process = psutil.Process(os.getpid())
		memory_usage_mb = process.memory_info().rss / (1024 ** 2) # Convert bytes to MB
		return memory_usage_mb
	
	def has_memory_exceeded_limit(self):
		return self.get_memory_usage() > self.memory_limit_mb
