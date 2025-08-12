# logger_config.py
import logging
from datetime import datetime

current_time = datetime.now().strftime("%Y_%m_%d-%H:%M")
LOG_FILE = f"../data/PH2_ACF_GUI_{current_time}.log"

def get_logger(name=None):
    logger = logging.getLogger(name)
    logger.setLevel(logging.DEBUG)

    if not logger.handlers:
        # File handler (DEBUG and up)
        file_handler = logging.FileHandler(LOG_FILE)
        file_handler.setLevel(logging.DEBUG)
        file_format = logging.Formatter(
            '%(asctime)s - %(name)s - %(filename)s:%(lineno)d - %(funcName)s - %(levelname)s - %(message)s'
        )
        file_handler.setFormatter(file_format)

        # Console handler (INFO and up)
        console_handler = logging.StreamHandler()
        console_handler.setLevel(logging.INFO)
        console_format = logging.Formatter('%(levelname)s - %(message)s')
        console_handler.setFormatter(console_format)

        logger.addHandler(file_handler)
        logger.addHandler(console_handler)

        # Prevent logging from being propagated to the root logger again
        logger.propagate = False

    return logger
