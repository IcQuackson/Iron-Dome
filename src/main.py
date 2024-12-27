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
from monitors.MemoryMonitor import MemoryMonitor
from monitors.DiskMonitor import DiskMonitor, handle_disk_monitor
from utils import signal_handler, parse_args, check_args

DEFAULT_PATH = "/home"

shutdown_event = threading.Event()

def main():

	monitor = MemoryMonitor()
	monitor.set_memory_limit()
	os.makedirs("/var/log/irondome", exist_ok=True)

	try:

		setup_logger()
		logger = logging.getLogger()

		args = parse_args()
		check_args(args)
		logger.info(f"Monitoring: {', '.join(path for path in args['paths'])}")

		t1 = threading.Thread(target=handle_disk_monitor, args=(args["paths"],), daemon=True)
		#t2 = threading.Thread(target=feature2, daemon=True)
		t1.start()
		#t2.start()

		""" 		l = []
		while not shutdown_event.is_set(): # This tests memory usage
			l.append("*" * 1024) """

		while not shutdown_event.is_set():
			#logger.info(f"Memory usage: {monitor.get_memory_usage()}")
			time.sleep(1)
	except MemoryError:
		logger.info(f"Memory has exceed the {monitor.get_memory_limit()} MB limit. Shuting down...")
		shutdown_event.set()



if __name__ == "__main__":
	signal.signal(signal.SIGINT, lambda sig, frame: signal_handler(sig, frame, shutdown_event))
	signal.signal(signal.SIGTERM, lambda sig, frame: signal_handler(sig, frame, shutdown_event))

	main()
