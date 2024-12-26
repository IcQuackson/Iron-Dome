import time
import logging
import os
import psutil
import resource

logger = logging.getLogger()

class MemoryMonitor:
	def __init__(self, memory_limit_mb=100):
		self.memory_limit_mb = memory_limit_mb

	def set_memory_limit(self):
		max_mem_bytes = self.memory_limit_mb * 1024 * 1024
		resource.setrlimit(resource.RLIMIT_AS, (max_mem_bytes, max_mem_bytes))

	def get_memory_limit(self):
		return self.memory_limit_mb
	
	def get_memory_usage(self):
		process = psutil.Process(os.getpid())
		memory_usage_mb = process.memory_info().rss / (1024 ** 2) # Convert bytes to MB
		return memory_usage_mb
	
	def has_memory_exceeded_limit(self):
		return self.get_memory_usage() > self.memory_limit_mb
