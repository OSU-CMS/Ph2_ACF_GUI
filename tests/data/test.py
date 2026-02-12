import re
from Gui.python.logging_config import get_logger
logger = get_logger(__name__)

line = "|06:25:13|I|>>> Progress:       0.3% <<<"
match = re.search(r"Progress:\s+([0-9]*\.?[0-9]+)%", line)

if match:
    logger.info(type(match.group(1)))  # Output: 0.3
