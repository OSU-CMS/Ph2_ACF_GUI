import os

pixelDir = '/home/bmanley/Ph2_ACF/'

os.system('source {0}setup.sh'.format(pixelDir))
os.system('rm -r {0}newTest'.format(pixelDir))
os.system('mkdir {0}newTest'.format(pixelDir))
os.system('cp {0}settings/RD53Files/CMSIT_RD53.txt {0}newTest/'.format(pixelDir))
os.system('cp {0}settings/CMSIT.xml {0}newTest/'.format(pixelDir))
os.chdir('{0}newTest'.format(pixelDir))
os.system('CMSITminiDAQ')