#!/usr/bin/env python3
import logging

# Customize the logging configuration
logging.basicConfig(
    level=logging.INFO,  # Set the default log level as needed.
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s",
)

# Create a logger
logger = logging.getLogger(__name__)

# Create a handler for writing logs to the console
console_handler = logging.StreamHandler()
console_handler.setLevel(
    logging.DEBUG
)  # Set the minimum log level for the console handler

# Create a formatter to customize the log message format (optional)
formatter = logging.Formatter("%(asctime)s - %(name)s - %(levelname)s - %(message)s")
console_handler.setFormatter(formatter)

# Add the console handler to the logger
logger.addHandler(console_handler)
