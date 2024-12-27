import logging
from logging.handlers import RotatingFileHandler
import sys
import os

logger = logging.getLogger()
	
def signal_handler(sig, frame, shutdown_event):
	logger.info("Received shutdown signal. Stopping...")
	shutdown_event.set()

def parse_args():
	paths = []

	if len(sys.argv) == 1:
		from main import DEFAULT_PATH
		paths.append(DEFAULT_PATH)
	else:
		paths += sys.argv[1:]
	return {"paths" : paths}

def check_args(args):
	if not args["paths"] or len(args["paths"]) == 0:
		logger.info("An error occured")
		sys.exit(1)
	
	paths = args["paths"]
	
	for p in paths:
		if not os.path.exists(p):
			logger.info(f"Path '{p}' is not valid")
			sys.exit(1)
	
	if len(paths) != len(set(paths)):
		logger.info("Providing duplicate paths is not valid")
		sys.exit(1)

		
