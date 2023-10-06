import re
from Gui.python.logging_config import logger

ConvertForSpan = {
    b"<": b"&#60;",
    b">": b"&#62;",
}

ANSI_richtext = {
    b"[0m": b"</span>",
    b"[1m": b"<b>",
    b"[31m": b'</span><span style="color:#ff0000;">',
    b"[32m": b'</span><span style="color:#00ff00;">',
    b"[33m": b'</span><span style="color:#ffff00;">',
    b"[34m": b'</span><span style="color:#0000ff;">',
    b"[35m": b'</span><span style="color:#ff00ff;">',
    b"[36m": b'</span><span style="color:#00ffff;">',
    b"[37m": b'</span><span style="color:#000000;">',
}


def parseANSI(text):
    text = text.replace(b"\x1b", b"")
    numBackLine = text.count(b"[A")
    textNoAnchor = text.replace(b"[A", b"")
    for character in ConvertForSpan.keys():
        if character in textNoAnchor:
            textNoAnchor = textNoAnchor.replace(character, ConvertForSpan[character])
    for pattern in ANSI_richtext.keys():
        if pattern in textNoAnchor:
            textNoAnchor = textNoAnchor.replace(pattern, ANSI_richtext[pattern])
    textNoAnchor = textNoAnchor.replace(b"</span>", b"", 1)
    if b"<b>" in textNoAnchor:
        textNoAnchor = textNoAnchor + b"</b>"
    textNoAnchor = textNoAnchor + b"\n"
    return numBackLine, textNoAnchor


if __name__ == "__main__":
    multilines = """
||I| [32mCreating directory: [1m[33mResults[0m\n

||I| [36m---------------------------[0m\n
||I| [32m****** Reading  data ******[0m\n
||I| [32mn. 32 bit words :     21600[0m\n
||I| [1m[35m>>>> Progress :   0.5% <<<<[0m\n
||I| [36m---------------------------[0m\n

[A[A[A[A[A||I| [36m---------------------------[0m\n
||I| [32m****** Reading  data ******[0m\n
||I| [32mn. 32 bit words :     21600[0m\n
||I| [1m[35m>>>> Progress :   1.0% <<<<[0m\n
||I| [36m---------------------------[0m\n

[A[A[A[A[A||I| [36m---------------------------[0m\n
||I| [32m****** Reading  data ******[0m\n
||I| [32mn. 32 bit words :     21600[0m\n
||I| [1m[35m>>>> Progress :   1.6% <<<<[0m\n
||I| [36m---------------------------[0m\n

[A[A[A[A[A||I| [36m---------------------------[0m\n
||I| [32m****** Reading  data ******[0m\n
||I| [32mn. 32 bit words :     21600[0m\n
||I| [1m[35m>>>> Progress :   2.1% <<<<[0m\n
||I| [36m---------------------------[0m\n
    """

    for line in multilines.split("\n"):
        num, text = parseANSI(line)
        print(text)
