import re

ConvertForSpan = {
"<"          :    '&#60;',
">"          :    '&#62;',
}

ANSI_richtext = {
"[0m"        :    '</span>',
"[1m"        :    '<b>',
"[31m"       :    '</span><span style="color:#ff0000;">',
"[32m"       :    '</span><span style="color:#00ff00;">',
"[33m"       :    '</span><span style="color:#ffff00;">',
"[34m"       :    '</span><span style="color:#0000ff;">',
"[35m"       :    '</span><span style="color:#ff00ff;">',
"[36m"       :    '</span><span style="color:#00ffff;">',
"[37m"       :    '</span><span style="color:#000000;">',
}

def parseANSI(text):
    numBackLine = text.count('[A')
    textNoAnchor = text.lstrip('[A')
    for character in ConvertForSpan.keys():
        if character in textNoAnchor:
            textNoAnchor = textNoAnchor.replace(character,ConvertForSpan[character])
    for pattern in ANSI_richtext.keys():
        if pattern in textNoAnchor:
            textNoAnchor = textNoAnchor.replace(pattern,ANSI_richtext[pattern])
    textNoAnchor  = textNoAnchor.replace("</span>","",1)
    if '<b>' in  textNoAnchor:
        textNoAnchor = textNoAnchor+"</b>"
    return numBackLine, textNoAnchor

if  __name__ == "__main__":
    multilines = '''
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
    '''

    for line in multilines.split('\n'):
        num,  text = parseANSI(line)
        print(text)

