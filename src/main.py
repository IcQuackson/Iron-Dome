#!/bin/python3
#!/usr/bin/env python3

import threading
import time
import signal
import os
import utils
from logger_setup import setup_logger
import logging
from logging.handlers import RotatingFileHandler
from jobs.MemoryMonitor import handle_memory_monitor, MemoryMonitor
from utils import signal_handler


os.makedirs("/var/log/irondome", exist_ok=True)
shutdown_event = threading.Event()

def main():

	monitor = MemoryMonitor()
	monitor.set_memory_limit()

	try:

		setup_logger()
		logger = logging.getLogger()

		#t1 = threading.Thread(target=handle_memory_monitor, daemon=True)
		#t2 = threading.Thread(target=feature2, daemon=True)
		#t1.start()
		#t2.start()

		l = []
		while not shutdown_event.is_set(): # This tests memory usage
			l.append("*" * 1024)

		while not shutdown_event.is_set():
			time.sleep(1)
	except MemoryError:
		logger.info(f"Memory has exceed the {monitor.get_memory_limit()} MB limit. Shuting down...")



if __name__ == "__main__":
	signal.signal(signal.SIGINT, lambda sig, frame: signal_handler(sig, frame, shutdown_event))
	signal.signal(signal.SIGTERM, lambda sig, frame: signal_handler(sig, frame, shutdown_event))

	main()
