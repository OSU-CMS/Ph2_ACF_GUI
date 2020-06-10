import os

os.chdir('/home/bmanley/Ph2_ACF')
os.system('source setup.sh')
os.system('mkdir newTest')
os.system('cp settings/RD53Files/CMSIT_RD53.txt newTest/')
os.system('cp settings/CMSIT.xml newTest/')
os.chdir('/home/bmanley/Ph2_ACF/newTest')
os.system('CMSITminiDAQ')