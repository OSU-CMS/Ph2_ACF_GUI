import os

pixelDir = '/home/bmanley/Ph2_ACF/'

os.system('rm -r {0}newTest'.format(pixelDir))
os.system('mkdir {0}newTest'.format(pixelDir))
os.system('cp {0}settings/RD53Files/CMSIT_RD53.txt {0}newTest/'.format(pixelDir))
os.system('cp {0}settings/CMSIT.xml {0}newTest/'.format(pixelDir))

os.chdir('{}newTest'.format(pixelDir))
os.system('source {0}setup.sh && cd {0}newTest && CMSITminiDAQ'.format(pixelDir))