import logging
from logging.handlers import RotatingFileHandler
import sys

logger = logging.getLogger()
	
def signal_handler(sig, frame, shutdown_event):
	logger.info("Received shutdown signal. Stopping...")
	shutdown_event.set()

def parse_args():
	if len(sys.argv) != 2:
		pass

