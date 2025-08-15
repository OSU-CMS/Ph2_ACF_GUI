import re

line = "|06:25:13|I|>>> Progress:       0.3% <<<"
match = re.search(r"Progress:\s+([0-9]*\.?[0-9]+)%", line)

if match:
    print(type(match.group(1)))  # Output: 0.3
