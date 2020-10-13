import os
import argparse

from Gui.GUIutils.settings import *

parser = argparse.ArgumentParser()
parser.add_argument("-f", "--file", help="Hardware description file. Default value: CMSIT.xml")
parser.add_argument("-c", "--calib", help="Which calibration to run [Latency PixelAlive NoiseScan ScurveScan GainScan Threqu GainOpt Thrmin Thradj Injdelay ClockDelay Physics]. Default: pixelalive")

args = parser.parse_args()

os.system('rm -r {0}/test/Results/*'.format(os.environ.get("Ph2_ACF_AREA")))
os.system('cd {0}/test && CMSITminiDAQ -f {1} -c {2}'.format(os.environ.get("Ph2_ACF_AREA"), args.file, calibration[args.calib]))



