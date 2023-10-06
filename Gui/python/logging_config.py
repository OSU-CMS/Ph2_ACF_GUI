#!/usr/bin/env python3
import logging

# Create a logger
logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)

# Create a handler for writing logs to the console
console_handler = logging.StreamHandler()
console_handler.setLevel(
    logging.INFO
)  # Set the minimum log level for the console handler

# Create a formatter to customize the log message format (optional)
formatter = logging.Formatter("%(asctime)s - %(name)s - %(levelname)s - %(message)s")
console_handler.setFormatter(formatter)

file_handler = logging.FileHandler('../Ph2_ACF_GUI.log')
file_handler.setLevel(logging.DEBUG)
file_handler.setFormatter(formatter)
# Add the console handler to the logger
logger.addHandler(console_handler)
logger.addHandler(file_handler)
