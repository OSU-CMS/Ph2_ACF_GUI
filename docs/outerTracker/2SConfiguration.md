# Configuration and Calibration of a 2S Module

[TOC maxLevel=3]

## Introduction

To configure a 2S Module using Ph2_ACF and an FC7 board, the following command is run: `runCalibration -f <filename> -c alignment`. This command initiates a sequence of interactions with the FC7 firmware, printed in this  [long file](https://cernbox.cern.ch/index.php/s/XDIfEF4fALdHYTL), and summarized below. 


NOTE: Ph2_ACF includes backup strategies to handle many exceptions, while the description below only includes the simple case of everything going according to plan^[As a simple example, if a fast-command resync is not cleared by the CIC in SystemController::InitializeOT, the fast command sampling edge is changed and the resync is sent again.].

NOTE: This documentation is based on [this Dev branch commit](https://gitlab.cern.ch/cms_tk_ph2/Ph2_ACF/-/tree/42c2fd0491f8b1fed8cf79d530f3b4b2ac4a2607) from April 16, 2024

NOTE: version 1, based on `ot_module_test`, is [here](https://codimd.web.cern.ch/aZInmo7-RAOvIdat8Z2ohQ?both)


References:
- LpGBT: https://cds.cern.ch/record/2809058/files/lpGBT_manual.pdf
- CIC: https://edms.cern.ch/ui/file/2797497/1/CIC2_specs_v1p4.pdf
- CBC: https://edms.cern.ch/ui/file/2355257/1/CBC3p1_User_Manual_V1p5p2.pdf
- FW: https://gitlab.cern.ch/cms_tk_ph2/d19c-firmware

___ 

## Switch on the module and configure it


### Configure CDCE clock 

Communication from the software to the FC7 board begins by configuring the CDCE clock generator ^[`D19cFWInterface::configureCDCE`: system.spi.tx_data, system.spi.command, sysreg.ctrl.cdce_sync, system.spi.rx_data]. This can be done a limited number of times, it should only be done for a newly connected FC7 board, or if the clock in input to the FW has recently changed (i.e. the FC7 was used for other things like hybrid testing).

### Activate FC7 and Module: 

The FC7 is reset ^[`D19cFWInterface::ConfigureBoard`: command_processor_block.global.reset=1, sysreg.fmc_pwr.pg_c2m=1] the DIO5 is powered ON ^[`D19cFWInterface::PowerOnDIO5`: sysreg.fmc_pwr.l8_pwr_en, sysreg.i2c_command, sysreg.i2c_settings], and the entire FC7 board (~500 registers) is configured. After a CDCE sync command ^[`D19cFWInterface::syncCDCE`: sysreg.ctrl.cdce_sync], the lpGBT tx/rx polarity and version ^[`D19cFWInterface::configureTxRxPolarities and configureLpGbtVersions`:  fc7_daq_cnfg.optical_block.tx_polarity.l12 and l8 = 0 and fc7_daq_cnfg.optical_block.lpgbt_version=0] are set in the FC7. The slow control interface ^[`D19clpGBTSlowControlWorkerInterface::Reset`: fc7_daq_ctrl.command_processor_block.i2c.control.reset_fifos=1 then =0, cpb_ctrl_reg.command_fifo_reset=1 then=0,cpb_ctrl_reg.reply_fifo_reset=1 then =0]  and the optical block ^[`D19cLinkInterface::ResetLinks`: fc7_daq_ctrl.optical_block.general=1 then 0 then 4194304] of the FC7 are reset, and hard_reset commands are sent to the FrontEnds and the CIC ^[`D19cFWInterface::ReadoutChipReset`: physical_interface_block.control.chip_hard_reset=1 and `D19cFWInterface::ChipReset`: physical_interface_block.control.cic_hard_reset=1]. Stub logic and sparsification are disabled ^[`D19cFWInterface::ConfigureBoard`: fc7_daq_cnfg.ddr3_debug.stub_enable=0 and fc7_daq_cnfg.physical_interface_block.cic.2s_sparsified_enable=0] before enabling the hybrid and chips ^[`D19cFWInterface::EnableFrontEnds`: fc7_daq_cnfg.global.chips_enable_hyb_00=255 and hyb_00=255, fc7_daq_cnfg.global.hybrid_enable=3], and sending a re-synch signal to align CBC L1A counters ^[`D19cFWInterface::ChipReSync`: fc7_daq_ctrl.fast_command_block.control=589824]. The Trigger is stopped ^[`D19cTriggerInterface::Stop`: fc7_daq_ctrl.stub_counter_block.general.shutter_close=1 and =0, fast_command_block.control.stop_trigger=1] and the Trigger FSM is reset ^[`D19cTriggerInterface::ResetTriggerFSM`: fc7_daq_ctrl.fast_command_block.control.reset=1, fast_command_block.control.load_config=1], then the readout is reset twice^[`D19cL1ReadoutInterface::ResetReadout`: fc7_daq_ctrl.readout_block.control.readout_reset=1 then =0, twice] , and finally the FC7 is reset again^[`D19cFWInterface::ConfigureBoard`: fc7_daq_ctrl.fast_command_block.control.reset=1].


### Configure lpGBT:

The first communication of Ph2_ACF with any chip registers is with the lpGBT: after checking the `ConfigPins` register to determine the chip info ^[`D19clpGBTInterface::ConfigureChip`: for example `Tx Data Rate = 5 Gbit/s; TxEncoding = FEC5; LpGBT Mode = Transceiver`], the Power-Up State Machine is checked using the `PUSMStatus` register, and the entire chip is configured (~300 registers), then the State Machine status is updated ^[`lpGBTInterface::SetPUSMDone`: lpGBT POWERUP2=0x6, meaning dllConfigDone and pllConfigDone] and checked once more. Once the `PUSMStatus` register reaches ready state, the I2C Masters are reset^[`lpGBTInterface::ResetI2C`: lpGBT RST0 is set to 0, then 0x7, then 0] and the CICs are reset using the programmable I/O (PIO) pins of the lpGBT^[`D19clpGBTInterface::cicReset and cbcReset`: lpGBT PIOOutH is set to 0x81, while PIOOutL is set first to 0x41 and then to 0x1, 0x9, 0x8, 0x48, 0x49]. 

### Configure CIC:

The entire CICs are configured (~100 registers), and then fully enabled using CIC registers^[`SystemController::CicStartUp`: set CIC FE_ENABLE to 0xff, then check lpGBT ConfigPins and CIC FE_CONFIG]. A soft reset is executed ^[`SystemController::CicStartUp`: CIC MISC_CTRL set to 0x12 then 0x2] after checking whether it is required ^[`CicInterface::CheckSoftReset`:  SOFT_RESET_REQUEST bit of CIC timingStatusBits register (0xA6 on page 5)]. The DLL in the CIC is reset and checked ^[`CicInterface::ResetDLL`: set CIC scDllResetReq0/1 to 0xff then to 0, then check with scDllLocked0/1], and the fast command detector lock is checked ^[`CicInterface::CheckFastCommandLock`:  FC_DECODER_LOCKED bit of CIC timingStatusBits]. A resync fast command is sent and checked ^[`SystemController::InitializeOT`: fc7_daq_ctrl.fast_command_block.control=0x90000 to send the resync and RESYNC_REQUEST bit of CIC timingStatusRegister to check if it has been cleared].

### Configure CBC:

The CBCs are reset^[`D19clpGBTInterface::cbcReset`: lpGBT PIOOutH and PIOOutL set to 0x81 and 0x49, then 0x81 and 0x49], then both pages of registers are configured^[Using CBC register `FeCtrl&TrgLat2` to select the page].

___

## Alignment Procedures


A short description of the `alignment` sequence is provided by `runCalibration -h` : 
- OTalignLpGBTinputs: Optimize LpGBT Rx phases to properly decode the inputs from the CICs
- OTalignBoardDataWord: Find bitslips in the FPGA to decode triggered data on to decode words
- OTverifyBoardDataWord: Use features of the CIC and FW to verify correct alignment of L1 and stub words in the FW
- OTalignStubPackage: Find stub package delay to properly decode stubs in the FC7
- OTCICphaseAlignment: Run the CIC automatic procedure to find the sampling phase for CBC/MPA stub and L1 lines
- OTCICwordAlignment: Run the CIC automatic word alignment procedure to properly decode CBC/MPA stub lines
- OTverifyCICdataWord: Inject L1 and stubs for each CBC/MPA and verify that CIC output corresponds to the expected pattern

The procedure for each step is described below, with images from [F.  Ravera's slides](https://indico.cern.ch/event/1392905/contributions/5864544/attachments/2824591/4933766/FRavera_2024_03_21.pdf)

___
### Align LpGBT inputs (OTalignLpGBTinputs)

In order to configure the CIC to output an alignment pattern on stub lines, the FC7 trigger source is set to internal and stopped^[fc7_daq_cnfg.fast_command_block.trigger_source=3
fc7_daq_ctrl.stub_counter_block.general.shutter_close=1
fc7_daq_ctrl.stub_counter_block.general.shutter_close=0
fc7_daq_ctrl.fast_command_block.control.stop_trigger=1], the output pattern is enabled^[`CicInterface::SelectOutput`: CIC MISC_CTRL from 0x2 to 0x6 to activate bit 2: OUTPUT_PATTERN_ENABLE], and the front-end block is disabled^[`CicInterface::EnableFEs`:  CIC FE_ENABLE=0]. 
After checking the LpGBT chip rate^[`D19clpGBTInterface::PhaseAlignRx`: LpGBT ConfigPins], the rx phase shifters are configured^[`lpGBTInterface::ConfigurePhShifter`: LpGBT PS0Config=0x1c, PS0Delay=0, same for PS2]. The Rx groups are enabled in phase tracking mode^[`lpGBTInterface::ConfigureRxGroup`: LpGBT EPRX%Control from 0x58 to 0x59] and the RxDll is reset^[`lpGBTInterface::ResetRxDll`: LpGBT RST1 to 1 and then 0]. The phase alignment training is then run^[`D19clpGBTInterface::PhaseAlignRx`: using registers EPRXTrain%, EPRX%Locked, EPRX%CurrentPhase%] and the resulting phase is set in the `EPRX%PhaseSelect` bits of the LpGBT `EPRX%ChnCntr` registers^[`lpGBTInterface::ConfigureRxPhase`: LpGBT EPRX%0ChnCntr]. Once alignment is complete, the training is disabled^[`lpGBTInterface::ConfigureRxGroup`: LpGBT EPRX%Control first bit set to 0], the CIC alignment pattern is switched off and the front-ends are re-enabled^[`LinkAlignmentOT::AlignLpGBTInputs`: CIC MISC_CTRL=0x2 and FE_ENABLE=0xff].


![](https://codimd.web.cern.ch/uploads/upload_d020b78567c566901e0a52f2da793ae0.png)

___
### Align board data word (OTalignBoardDataWord)

Here the CIC is configured, as above, to output an alignment pattern, with the front-end block disabled and triggers stopped. The word alignment is performed by the `phase_tuning_ctrl` of the physical_interface_block in the FW^[`OTalignBoardDataWord::stubAndL1WordAlignment`: send commands (built by `D19cBackendAlignmentFWInterface::SendCommand`) through fc7_daq_ctrl.physical_interface_block.phase_tuning_ctrl, and get replies in phase_tuning_reply]. The `phase_tuning_ctrl` commands sent to the FC7 for stub and L1 word alignment are `Configure`, `AlignLine`, `ReturnResult`^[For example, a sequence of `phase_tuning_ctrl` commands is 1179648, 1376258, 1114112]. 


Once the stub alignment is complete, the FC7 triggers are configured, the handshake for the readout block is enabled^[`OTalignBoardDataWord::stubAndL1WordAlignment`: 
fc7_daq_cnfg.fast_command_block.triggers_to_accept=0
fc7_daq_cnfg.fast_command_block.misc.backpressure_enable=0
fc7_daq_cnfg.fast_command_block.user_trigger_frequency=100
fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity=0
fc7_daq_ctrl.fast_command_block.control.load_config=1
fc7_daq_cnfg.tlu_block.tlu_enabled=0
fc7_daq_cnfg.readout_block.global.data_handshake_enable=1], chips are resync'd^[fc7_daq_ctrl.fast_command_block.control=589824] and triggers are switched on ^[fc7_daq_ctrl.stub_counter_block.general.shutter_close=0x1 then 0, fc7_daq_ctrl.fast_command_block.control.stop_trigger=0x1,
fc7_daq_ctrl.fast_command_block.control.reset=0x1
fc7_daq_ctrl.fast_command_block.control.load_config=0x1
fc7_daq_ctrl.stub_counter_block.general.shutter_open=0x1 then 0
fc7_daq_ctrl.fast_command_block.control.start_trigger=0x1]. The L1 word alignment is again performed by the `phase_tuning_ctrl` of the physical_interface_block in the FW. Ten events are then read out^[`SystemController::ReadNEvents`], even though the front-ends are disabled. At this point, the stub alignment pattern of the CIC is disabled^[CIC MISC_CTRL = 0x2], and the front-ends are re-enabled^[CIC FE_ENABLE=0xff].

Since this procedure takes place inside the FW, it does not affect any of the chip registers.

![](https://codimd.web.cern.ch/uploads/upload_5825adb8ea872240b02885644bd08e2e.png)

___
### Verify board data word (OTverifyBoardDataWord)

The CIC pattern is enabled, the front-ends are disabled^[CIC MISC_CTRL from 0x2 to 0x6, FE_ENABLE from 0xff to 0], and the stub debug outout (i.e. a snapshot of the incoming pattern) is printed^[`OTverifyBoardDataWord::runStubIntegrityTest`: select the hybrid with fc7_daq_cnfg.physical_interface_block.slvs_debug.hybrid_select, set fc7_daq_cnfg.physical_interface_block.slvs_debug.chip_select=0, then write fc7_daq_ctrl.fast_command_block.control=131072 and read fc7_daq_stat.physical_interface_block.stub_debug] and analyzed^[OTverifyBoardDataWord::isStubPatternMatched]

Once complete, the triggers are stopped and the L1A debug output is produced in the FW, similarly to the stub word alignment above^[`OTverifyBoardDataWord::runL1IntegrityTest`: select the hybrid with fc7_daq_cnfg.physical_interface_block.slvs_debug.hybrid_select, set fc7_daq_cnfg.physical_interface_block.slvs_debug.chip_select=0, then fc7_daq_cnfg.fast_command_block.misc.initial_fast_reset_enable=0, misc.backpressure_enable=0, control.stop_trigger=0x1, control.reset=0x1, control.load_config=0x1, control.start_trigger=0x1, then count triggers with fc7_daq_stat.fast_command_block.trigger_in_counter, then stop triggers with fc7_daq_ctrl.fast_command_block.control.stop_trigger=0x1, then read fc7_daq_stat.physical_interface_block.l1a_debug].  The L1A output is printed and the header is analyzed. 


The front-end registers are restored, the front-ends are enabled^[CIC FE_ENABLE=0xff] and the stub alignment output of the CIC is disabled^[CIC MISC_CTRL = 0x2].

![](https://codimd.web.cern.ch/uploads/upload_05e6ff7db0063c2a23dd0a87ad794d01.png)

___
### Align stub package on board (OTalignStubPackage)

__[Updated in merge request [!570](https://gitlab.cern.ch/cms_tk_ph2/Ph2_ACF/-/merge_requests/570), see [here](https://indico.cern.ch/event/1391274/contributions/5848662/attachments/2867622/5019684/FRavera_2024_05_30.pdf)]__

The purpose of this step is to determine the `stub_package_delay` parameter of the FC7^[fc7_daq_cnfg.physical_interface_block.stubs.stub_package_delay]. The stub package alignment is setup by disabling the front-ends^[CIC FE_ENABLE=0x0], setting up the test pulse trigger, the test pulse delay and stub readout delay^[`LinkAlignmentOT::AlignStubPackage`:
fc7_daq_cnfg.fast_command_block.trigger_source=6
fc7_daq_ctrl.fast_command_block.control.load_config=1
fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity=0
fc7_daq_cnfg.fast_command_block.test_pulse.delay_after_test_pulse=300
fc7_daq_cnfg.readout_block.global.common_stubdata_delay=200
fc7_daq_cnfg.fast_command_block.test_pulse.en_fast_reset=1
fc7_daq_cnfg.tlu_block.tlu_enabled=0]. For each value of the `stub_package_delay`, the FC7 decoder is reset^[`D19cFWInterface::Bx0Alignment`: fc7_daq_ctrl.physical_interface_block.control.decoder_reset=1 then 0, then send a global resynch (fc7_daq_ctrl.fast_command_block.control=65536) and repeat until fc7_daq_stat.physical_interface_block.cic_decoder.bx0_alignment_state=8],  10 events are read out^[SystemController::ReadNEvents], the BxIDs are compared between two hybrids of a link and between multiple links, until a `stub_package_delay` that results in all hybrids matching is found.

Once this is done, 10 more events are read out using the internal trigger^[fc7_daq_cnfg.fast_command_block.trigger_source=3], after resetting the original values for test pulse delay, and their BxIDs are printed and __[TO DO]__ added to a histogram. The FC7 are then set back to original values, except for the `stub_package_delay` register measured by this procedure, and the front-ends are enabled^[CIC FE_ENABLE=0xff].

___
### CIC phase tuning (OTCICphaseAlignment)

The CIC is configured to use the automatic phase aligner^[`CicInterface::SetAutomaticPhaseAlignment`: CIC PHY_PORT_CONFIG=0xb, which sends a 01 on the scTrackMode bits, activating "EportRxTrackingModeAutoStartup: mode used to determine the phase alignment parameters"], and the phase aligner is reset^[CIC scResetChannels0 and scResetChannels1 to 0xff and then to 0]. The FC7 trigger source is set to internal^[fc7_daq_cnfg.fast_command_block.trigger_source=3
fc7_daq_cnfg.fast_command_block.triggers_to_accept=500
fc7_daq_ctrl.fast_command_block.control.load_config=1] and to align the L1 lines, the PtCut is set to maximum and the readout is set to Sampled^[Pipe&StubInpSel&Ptwidth=0xe], the cluster cut is set to ≤4, ^[LayerSwap&CluWidth=0x4], and the channel masks are set to enable every other channel to create the 10101010 pattern^[CBC MaskChannel-%-to-% = 0x55, except for 0x15 for the last one]. The CBC OR254 flag (set to 1 if any channels are hit) is enabled in the stub output^[`CbcInterface::producePhaseAlignmentPattern`: CBC 40MhzClk&Or254=0xca]A channel reset is applied to begin the phase alignment^[`CicInterface::ResetPhaseAligner`: CIC scResetChannels% = 0xff, then 0], and the triggers are sent^[D19cTriggerInterface::SendNTriggers]. The CIC ChannelLocked register vectors are read out^[CIC scChannelLocked%] and checked, and the optimal phases^[CIC scPhaseSelectB%] are read out.

Next, to aling the Stub 0 lines, the CBC stub bend code and window offests are adapted^[CBC Bend7=0xa, CoincWind&Offset12=0, CoincWind&Offset34=0], the VCth is set to 1023^[CBC VCth1=0xff and VCth2=0x3], and the desired channels are masked^[MaskChannel-088-to-081=0x3c, MaskChannel-176-to-169=0x3, the rest at 0]. As above, a channel reset is applied to begin the phase alignment, the triggers are sent, the CIC ChannelLocked register vectors and optimal phases are read out and checked. The procedure is repeated again for Stub line 1 through 4 (all in one step), just changing the channel masks to create the pattern^[MaskChannel-048-to-041=0x3, 088-to-081=0x3c, 176-to-169=0x3, the rest at 0].

Automated phase alignment is then disabled^[`CicInterface::SetStaticPhaseAlignment`: CIC PHY_PORT_CONFIG=0x3], and the CIC phase registers `scPhaseSelectB%` are set with the phase giving the best efficiency based on all the information collected above.

![](https://codimd.web.cern.ch/uploads/upload_0c02727aa7ed3bde5652c1431be9d984.png)
___
### CIC word alignment (OTCICwordAlignment)

The CBC word alignment patterns are setup: the HitOr (OR254) flag is switched OFF, the PtCut is set to maximum, the cluster cut is set to ≤4, the readout is set to Sampled^[`CbcInterface::produceWordAlignmentPattern`: CBC 40MhzClk&Or254=0x8a, Pipe&StubInpSel&Ptwidth=0xe, LayerSwap&CluWidth=0x4, Pipe&StubInpSel&Ptwidth=0xe], then the patterns are configured using the bend look-up table and setting the stub window offsets to 0^[`CbcInterface::produceWordAlignmentPattern`: CBC Bend7=0x1, Bend8=0x3, Bend9=0x1, CoincWind&Offset12=0, CoincWind&Offset12=0], the dedicated channel masks are configured^[MaskChannel-128-to-121=0x3, 192-to-185=0x24, 216-to-209=0x84 and the rest at 0], and VCth is set to 1023.


The alignment is then performed through the auto word alignment of the CIC: the patterns are set up^[`CicInterface::ConfigureAlignmentPatterns`: CIC CALIB_PATTERN0=0x7a, PATTERN1=0xbc, PATTERN2=0xd4, PATTERN3=0x31, PATTERN4=0x81] and the word alignment is enabled^[`CicInterface::ConfigureAlignmentPatterns`: CIC MISC_CTRL=0 to set USE_EXT_WA_DELAY=0 to use internal word delay, then MISC_CTRL=1 to set AUTO_WA_REQUEST=1] until it is completed^[`CicInterface::ConfigureAlignmentPatterns`: require 1 in the first timingStatusBits, AUTO_WA_DONE, then set MISC_CTRL=1 to set AUTO_WA_REQUEST=0]. The word alignment values resulting from the automated procedure are then read out^[`CicInterface::retrieveExternalWordAlignmentValues`: CIC WA_DELAY%] and written to the dedicated `EXT_WA_DELAY%` CIC registers. At the end of the procedure, the automatic word alignment is disabled^[CIC MISC_CTRL=0x2].

![](https://codimd.web.cern.ch/uploads/upload_a3f05fffca08875d447d76fe54d6f770.png)

___
### Verify CIC data word (OTverifyCICdataWord)

For stubs, after checking the lpGBT rate^[CIC ConfigPins], the CIC automatic pattern is disabled^[CIC MISC_CTRL=0x2 so that bit 2 (OUTPUT_PATTERN_ENABLE) is deactivated], the stub debug outout for the CIC is enabled^[fc7_daq_cnfg.physical_interface_block.slvs_debug.hybrid_select and .chip_select=0], specifying that the output frame should include the stub bend^[CIC BEND_SEL set to 1, i.e. FE_CONFIG=0x4], and a single CBC at a time is enabled^[starting with CIC FE_ENABLE=0x1]. The pattern is set up similarly to OTCICwordAlignment^[Pipe&StubInpSel&Ptwidth=0xe, LayerSwap&CluWidth=0x4, Bend7=0x9, Bend9=0xb, Bend0=0xf, CoincWind&Offset12=0, CoincWind&Offset34=0] with Vcth at 1023^[VCth1=0xff,VCth2=0x3] and most channels masked^[Except MaskChannel-016-to-009=0x3, 160-to-153=0x40, 168-to-161=0x2, 176-to-169=0x21]. Each trigger is then sent^[Fast command, fc7_daq_ctrl.fast_command_block.control=131072] and the corresponding data^[80 28-bit words from fc7_daq_stat.physical_interface_block.stub_debug] is read out and compared to the expected pattern. Readout requires concatenating the data^[concatenatedStubPackage], since the stubs are sent on 5 lines. Then the next CBC is selected^[Using FE_ENABLE] and the process is repeated until all CBCs have been tested.

For L1, the FC7 trigger is set up for L1 alignment^[
fc7_daq_cnfg.fast_command_block.triggers_to_accept=0
fc7_daq_cnfg.fast_command_block.misc.backpressure_enable=0
fc7_daq_cnfg.fast_command_block.user_trigger_frequency=100
fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity=0
fc7_daq_ctrl.fast_command_block.control.load_config=1
fc7_daq_cnfg.tlu_block.tlu_enabled=0
fc7_daq_cnfg.readout_block.global.data_handshake_enable=1
fc7_daq_cnfg.readout_block.global.data_handshake_enable=1
], CIC sparsification is enabled^[FE_CONFIG=0x14 to activate bit 4, CBC_SPARSIFICATION_SEL] and the debug output is enabled^[fc7_daq_cnfg.physical_interface_block.slvs_debug.hybrid_select and .chip_select=0]. As above, all CBCs except 1 are disabled^[CIC FE_ENABLE=1, etc] and the CIC automatic pattern is disabled^[CIC MISC_CTRL=0x2 so that bit 2 (OUTPUT_PATTERN_ENABLE) is deactivated]. The CBC is set up with the HitOr (OR254) flag enabled, Sampled logic, Hits+Stubs^[CBC 40MhzClk&Or254=0xca, Pipe&StubInpSel&Ptwidth=0xe], and the clusters are created by setting Vcth=1023 and enabling a few channels^[CBC MaskChannel-176-to-169=0x14 and all others at 0]. Events are read out^[ The trigger is activated with:
fc7_daq_cnfg.fast_command_block.misc.initial_fast_reset_enable=1
fc7_daq_cnfg.fast_command_block.misc.backpressure_enable=0
fc7_daq_ctrl.fast_command_block.control.stop_trigger=1
fc7_daq_ctrl.fast_command_block.control.reset=1
fc7_daq_ctrl.fast_command_block.control.load_config=1
fc7_daq_ctrl.fast_command_block.control.start_trigger=1
and stopped with: 
fc7_daq_ctrl.fast_command_block.control.stop_trigger=1
once fc7_daq_stat.fast_command_block.trigger_in_counter is larger than 10. The data is read out as 50 28-bit words from fc7_daq_stat.physical_interface_block.l1a_debug. This is done N times to accumulate statistics.] and the data is compared with the expected pattern.


![](https://codimd.web.cern.ch/uploads/upload_e5da86f0bd509776347c7b03a05700c7.png)

___
### [To Do, Ph2_ACF development in progress] BX0 delay

Set the CIC register EXT_BX0_DELAY

![](https://codimd.web.cern.ch/uploads/upload_a8df8afa740630f757a978ffd15dcbe9.png)

___

## Calibration Procedures

After alignment, configuration of a 2S module continues with the following steps, which require the high voltage to be ON.
- `runCalibration -f <xml_file> -c calibrationandpedenoise`
    - This module runs the [alignment](#Alignment-Procedures) sequence above, and adds the following two steps
    - PedestalEqualization: Equalize the pedestal/threshold for all channels
    - PedeNoise: Measure noise and Pedestal/pulse peak, set threshold at 5 sigma from the pedestal and run occupancy measurement
- `runCalibration -r <calibrationandpedenoise_runNumber> -c injectionDelayOptimization`
    - OTalignBoardDataWord: already part of [alignment](#Alignment-Procedures)
    - OTinjectionDelayOptimization: Optimize delay for injecting calibration pulses
- `runCalibration -r <injectionDelayOptimization_ruNumber> -c measureOccupancy`
    - OTalignBoardDataWord: already part of [alignment](#Alignment-Procedures)
    - OTMeasureOccupancy: Measure channel occupancy

At the end of this sequence, the CBC is set to the correct injection delay and with a threshold 5 times the noise away from the pedestal^[Configurable in xml using OTinjectionDelayOptimization_CBCnumberOfSigmaNoiseAwayFromPedestal].

The procedures are introduced here [F.  Ravera's slides](https://indico.cern.ch/event/1391271/contributions/5848650/attachments/2840534/4964997/FRavera_2024_04_12_PS_L1_issue.pdf)

___
### Equalize the pedestal/threshold (PedestalEqualization)


The procedure takes as input the desired occupancy at pedestal (default: 56%) and the target channel offset (default: 127), and determines the VCth (per module) and offset (per channel) that achieves this occupancy.

The stub output of the CBCs is disabled^[CBC Pipe&StubInpSel&Ptwidth=0x2e, i.e. HIP suppression  for stubs, and HIP&TestMode=0, i.e. HIP suppression enabled after 0 clocks suppressing all stubs]. The CBC test pulse is also disabled^[MiscTestPulseCtrl&AnalogMux=0, where Bit 6 is Enable Test Pulse], all channel offsets are set to 127^[CBC Channel%=0x7f] and VCth is set to 0^[CBC VCth1=0 and VCth2=0]. A bit-wise scan of VCth is performed, measuring channel occupancy at each step, until the optimal threshold is found for each chip. 

The  channel offsets are then set to 255 (max) and the VCth value for each CBC is printed out (for example, in the 590-610 range). At this point, the VCth of all CBCs is set to the average of the individual CBC values (for example 603).

Next, a bit-wise scan of the individual channel offsets is performed, keeping the VCth fixed, until the target occupancy is obtained in each channel. The channel offsets are expected to be around the target value (for example 112-155, while the target was 127), and they are saved in a file to be used for subsequent tests.

___
### Measure noise pedestal (PedeNoise) 

Noise is measured using s-curves: as the threshold changes, the occupancy goes from 0% to 100% tracing an Erf function (i.e. an s-curve), and the sigma of this s-curve is defined as the noise. If noise is low, the s-curve is sharp. In this procedure the s-curves are generated without a test pulse, so the amplitude of the CBC test pulse is set to zero^[CBC TestPulsePotNodeSel=0 and MiscTestPulseCtrl&AnalogMux=0]. The CBC trigger latency is set to 198 bunch crossings^[CBC TriggerLatency1=0xc6 and FeCtrl&TrgLat2=0x40].

Before measuring noise, the pedestal for each CBC is found using a bit-wise scan of VCth, with 1000 events at 100 Hz at each step of the search, targeting a 56% occupancy, as in the procedure above. The stub output of the CBC is disabled as above^[CBC Pipe&StubInpSel&Ptwidth=0x2e and HIP&TestMode=0]. To generate the s-curve, VCth is set to the average threshold of all CBCs (for example 602) and slowly moved, while average occupancy is measured over 1000 events. The VCth scan is performed in steps of 1, and ranges from 10 steps below occupancy of 90% to 10 steps above occupancy of 10% (for example from 573 to 626).

The noise measurement is validated by taking 100,000 events with VCth of all CBCs set to 5 sigma away from the pedestal (for example 541 with a measured noise of 11.9). For this test, noisy channels are masked (__A version of this test with unmasked noisy channels might be added to the QA__).

---
### Set injection delay and readout latency (OTinjectionDelayOptimization)

The procedure looks for the best delay from charge injection to buffer readout. This is the delay resulting in the highest signal for a fixed amplitude of the calibration pulse, i.e. the delay allowing to reach 50% occupancy with the highest value of VCth. The following CBC registers are set by the procedure: TestPulseDel&ChanGroup (injection delay), TriggerLatency1/FeCtrl&TrgLat2 (Trigger latency split in two registers). 

To prepare for pulsing, the CBC test pulse is enabled^[CBC MiscTestPulseCtrl&AnalogMux=0x40 to activate bit 6 (i.e. Enable Test Pulse)], while sparsification is disabled on the CIC^[CIC FE_CONFIG to 0x4 to disactivate bit 4 (CBC_SPARSIFICATION_SEL)] and on the FC7^[fc7_daq_cnfg.physical_interface_block.cic.2s_sparsified_enable=0]. On the FC7, the "delay after test pulse"  is set to 199 bunch crossings (i.e. 25 ns clock cycles)^[fc7_daq_cnfg.fast_command_block.test_pulse.delay_after_test_pulse=199]. The CBC HitOr (OR254) flag is enabled^[CBC 40MhzClk&Or254=0xca to enable bit 6 (ENOR254)], and the Test Pulse Amplitude is set to 218^[CBC TestPulsePotNodeSel=0xda]. 

A loop begins, with increasing delays from 0 to 150 ns, which are converted into coarse TriggerLatency values up to 6 bunch crossings  away from the default of 200, and fine TestPulseDelays from 0 to 25ns^[The TriggerLatency is set to 200 - (delay / 25) clock cycles (exception: 201 if delay is 0), so from 201 to 195. The TestPulseDelay  is set to 25 - (delay % 25) (exception: 0 if delay is 0), so ranging between 0 and 25. The TriggerLatency is entered into the CBC registers TriggerLatency1/FeCtrl&TrgLat2, while the TestPulseDelays go into TestPulseDel&ChanGroup]. For each delay, a VCth bitwise scan is performed to find the VCth resulting in 50% occupancy. 

The best TriggerLatency and TestPulseDelay (for example best latency = 198 clock cycles, - best delay = 13 ns) is then determined for each CBC, based on the settings that resulted in the highest value of VCth for a 50% efficiency. 

Note: Since the last three bits of TestPulseDel&ChanGroup are always set to 0, the same channel group (000) is always pulsed, ignoring the other channels. 
___
### Measure channel occupancy (OTMeasureOccupancy)


Most of this paragraph is the same as the start of the previous procedure: To prepare for pulsing, the CBC test pulse is enabled^[CBC MiscTestPulseCtrl&AnalogMux=0x40 to activate bit 6 (i.e. Enable Test Pulse)], while sparsification is disabled on the CIC^[CIC FE_CONFIG to 0x4 to disactivate bit 4 (CBC_SPARSIFICATION_SEL)] and on the FC7^[fc7_daq_cnfg.physical_interface_block.cic.2s_sparsified_enable=0]. On the FC7, the "delay after test pulse"  is left at the 199 value of the previous test ^[fc7_daq_cnfg.fast_command_block.test_pulse.delay_after_test_pulse=199]. The Test Pulse Amplitude is set to 218, corresponding to a MIP^[CBC TestPulsePotNodeSel=0xda]. 

As opposed to the previous procedure, the channel group to receive the test pulse is varied^[Last 3 bits of CBC TestPulseDel&ChanGroup] until all channels have been pulsed. For each channel group, 100,000 events are read out. The un-pulsed channels are not masked (but Ph2_ACF ignores them when analyzing the results).

___

## Additional characterizations

Other interesting methods

---

### [To Do] Measure Common Mode noise (OTCMNoise)

---

### [To Do] Measure Temperature (OTTemperature)

--- 

### [To Do] Tune Vref value for LpGBT ADC (TuneLpGBTVref)

---

### General Methods

Reading events, for example while performing scans, is done by the D19cL1ReadoutInterface::ReadEvents function, with the following interaction with the FW.
^[##FW Value in register ID fc7_daq_cnfg.fast_command_block.user_trigger_frequency : 100
##FW: RegManager::WriteReg: fc7_daq_cnfg.readout_block.global.data_handshake_enable=1
##FW Value in register ID fc7_daq_cnfg.fast_command_block.misc.trigger_multiplicity : 0
##FW: RegManager::WriteReg: fc7_daq_ctrl.fast_command_block.control.reset=1
##FW: RegManager::WriteStackReg: BEGIN
##FW: RegManager::WriteStackReg: fc7_daq_cnfg.fast_command_block.triggers_to_accept=1000
##FW: RegManager::WriteReg: fc7_daq_ctrl.fast_command_block.control.load_config=1
##FW: RegManager::WriteReg: fc7_daq_cnfg.readout_block.packet_nbr=999
##FW: RegManager::WriteReg: fc7_daq_ctrl.readout_block.control.readout_reset=1
##FW: RegManager::WriteReg: fc7_daq_ctrl.readout_block.control.readout_reset=0
##FW Value in register ID fc7_daq_stat.ddr3_block.init_calib_done : 1
##FW: RegManager::WriteReg: fc7_daq_ctrl.stub_counter_block.general.shutter_close=1
##FW: RegManager::WriteReg: fc7_daq_ctrl.stub_counter_block.general.shutter_close=0
##FW Value in register ID fc7_daq_stat.fast_command_block.general.fsm_state : 0
##FW: RegManager::WriteReg: fc7_daq_ctrl.fast_command_block.control.stop_trigger=1
##FW Value in register ID fc7_daq_stat.fast_command_block.general.fsm_state : 0
##FW: RegManager::WriteReg: fc7_daq_ctrl.fast_command_block.control.reset=1
##FW: RegManager::WriteReg: fc7_daq_ctrl.fast_command_block.control.load_config=1
##FW Value in register ID fc7_daq_stat.fast_command_block.general.source : 3
##FW Value in register ID fc7_daq_cnfg.fast_command_block.trigger_source : 3
##FW Value in register ID fc7_daq_cnfg.fast_command_block.user_trigger_frequency : 100
##FW Value in register ID fc7_daq_cnfg.fast_command_block.triggers_to_accept : 1000
##FW: RegManager::WriteReg: fc7_daq_ctrl.stub_counter_block.general.shutter_open=1
##FW: RegManager::WriteReg: fc7_daq_ctrl.stub_counter_block.general.shutter_open=0
##FW: RegManager::WriteReg: fc7_daq_ctrl.fast_command_block.control.start_trigger=1
##FW Value in register ID fc7_daq_stat.fast_command_block.running_time : 13378
##FW Value in register ID fc7_daq_cnfg.fast_command_block.triggers_to_accept : 1000
##FW Value in register ID fc7_daq_stat.fast_command_block.trigger_in_counter : 72
...
...
##FW Value in register ID fc7_daq_stat.fast_command_block.trigger_in_counter : 986
##FW Value in register ID fc7_daq_stat.fast_command_block.trigger_in_counter : 1000
##FW Value in register ID fc7_daq_stat.fast_command_block.general.fsm_state : 0
##FW Value in register ID fc7_daq_stat.fast_command_block.trigger_in_counter : 1000
##FW: RegManager::WriteReg: fc7_daq_ctrl.stub_counter_block.general.shutter_close=1
##FW: RegManager::WriteReg: fc7_daq_ctrl.stub_counter_block.general.shutter_close=0
##FW Value in register ID fc7_daq_stat.fast_command_block.general.fsm_state : 0
##FW: RegManager::WriteReg: fc7_daq_ctrl.fast_command_block.control.stop_trigger=1
##FW Value in register ID fc7_daq_stat.fast_command_block.general.fsm_state : 0
##FW Value in register ID fc7_daq_stat.readout_block.general.words_cnt : 80000
##FW Value in register ID fc7_daq_stat.readout_block.general.readout_req : 1
##FW Value in register ID fc7_daq_cnfg.readout_block.global.data_handshake_enable : 1]