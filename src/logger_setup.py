import logging
from logging.handlers import RotatingFileHandler
import sys

# Logger singleton
def setup_logger():
	logger = logging.getLogger()
	logger.setLevel(logging.INFO)

	if not logger.handlers:

		rotating_handler = RotatingFileHandler(
			"/var/log/irondome/irondome.log",
			maxBytes=5_000_000, 
			backupCount=3
		)
		rotating_handler.setLevel(logging.INFO)
		formatter = logging.Formatter(
			"%(asctime)s | %(name)s | %(levelname)s | %(message)s",
			"%Y-%m-%d %H:%M:%S"
		)
		rotating_handler.setFormatter(formatter)

		console_handler = logging.StreamHandler()
		console_handler.setLevel(logging.INFO)
		console_handler.setFormatter(formatter)

		logger.addHandler(console_handler)
		logger.addHandler(rotating_handler)
	return logger