#!/bin/python3

#!/usr/bin/env python3

import threading
import time
import signal
import os
import logging
from logging.handlers import RotatingFileHandler

os.makedirs("/var/log/irondome", exist_ok=True)

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)
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
logger.addHandler(rotating_handler)

shutdown_event = threading.Event()

def feature1():
    while not shutdown_event.is_set():
        logger.info("Feature 1 is running...")
        time.sleep(2)

def feature2():
    while not shutdown_event.is_set():
        logger.info("Feature 2 is running...")
        time.sleep(3)

def main():
    t1 = threading.Thread(target=feature1, daemon=True)
    t2 = threading.Thread(target=feature2, daemon=True)

    t1.start()
    t2.start()

    # Keep the main thread alive
    while not shutdown_event.is_set():
        time.sleep(1)

def signal_handler(sig, frame):
    logger.info("Received shutdown signal. Stopping...")
    shutdown_event.set()

if __name__ == "__main__":
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    main()
