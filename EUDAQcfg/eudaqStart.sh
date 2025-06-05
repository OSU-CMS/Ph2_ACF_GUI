#!/usr/bin/env sh
# Define port
#export RUNCONTROL=cmsdaq01.desy.de:44000
export RUNCONTROL=192.168.21.100:44000
echo $RUNCONTROL

# Start Run Control
xterm -T "Run Control" -e 'euRun' &
sleep 2 # 30 seems to stop it from core dumping .. but might be excessive 

# # Start Logger
xterm -T "Log Collector" -e 'euLog -r tcp://${RUNCONTROL}' &
sleep 2 # should this be longer too?

# # Start one DataCollector
# Line commented out by Edo
xterm -T "Data Collector" -e 'euCliCollector -n EventIDSyncDataCollector -t one_dc -r tcp://${RUNCONTROL}' &
sleep 2

# NI/Mimosa
# xterm -T "NI/Mimosa Producer" -e 'euCliProducer -n NiProducer -t ni_mimosa -r tcp://${RUNCONTROL}'  &
# sleep 2

#TLU producer 
#xterm -T "TluProducer" -e 'euCliProducer -n AidaTluProducer -t aida_tlu -r tcp://${RUNCONTROL}' &
# xterm -T "EUDET TLU Producer" -e 'euCliProducer -n EudetTluProducer -t eudet_tlu -r tcp://${RUNCONTROL}' &
# sleep 1

# eudaq producer from ph2acf
xterm -T "Ph2 ACF Producer" -e 'eudaqproducer -r tcp://${RUNCONTROL}' &
# # # for producer with raw data (test), cdz
# # # xterm -T "Ph2 ACF Producer" -e 'eudaqproducer -r tcp://${RUNCONTROL} -s ' &
sleep 1

# OnlineMonitor
# xterm -T "Online Monitor" -e 'StdEventMonitor -t StdEventMonitor -r tcp://${RUNCONTROL}' & 
# sleep 1


