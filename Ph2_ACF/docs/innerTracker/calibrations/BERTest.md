# Bit Error Rate test
<sub><sup>Last updated: 13.12.2024</sup></sub>

## Purpose

The Bit Error Rate (BER) Test is a basic calibration that measures the number of errors in the data stream. It is used to determine the quality of the data transmission.

Three different data transmission chains can be tested:
    1. Between the back-end (FC7) and the LpGBT
    2. Between the LpGBT and the front-end (module)
    3. Between the back-end (FC7) and the front-end (module)

Only option 3 is possible when using an electrical link.

## Method

The BER Test calibration sends pseudo-random binary sequences (PRBS) back and forth between the two components of the system and then compares the received data with the transmitted one to calculate the BER. The test is performed for a predetermined time duration or a predetermined number of frames, set using the `framesORtime` parameter. Depending on the parameter `byTime` value, `framesORtime` either sets the number of frames to be sent (`byTime=0`) or the measurement time in seconds (`byTime=1`). When the calibration finishes, the total number of PRBS frames and the number of read-back frames with error(s) are printed on the screen. If there are no frames with errors, the BER test is considered to be passed.

**Scan command:** `bertest`

## Configuration parameters

|Name            |Typical value|Description|
|----------------|-------------|-----------|
|`chain2Test`    |0            |Chain under test: 0$\rightarrow$BE-FE, 1$\rightarrow$BE-LpGBT, 2$\rightarrow$LpGBT-FE|
|`byTime`        |1            |Chooses whether the test duration is given in seconds or \#frames|
|`framesORtime`  |10           |Duration of the test: in seconds or \#frames (see `byTime`)|

## Typical output

Two examples of text output from the BER Test are shown below. The first one shows the text output when the BER Test has been passed (BER=0), whereas the second one shows the output example when the BER Test has been failed (BER>0) by plugging out the Display Port cable from the SCC for a brief moment during the test.

![BER Test Output Pass](images/bertest/BERtestOutputPass.png){width=380}![BER Test Output Fail](images/bertest/BERtestOutputFail.png){width=380}

The BER Test produces only a single histogram in the output root file that has a single bin whose content is the measured Bit Error Rate. The x-axis has no meaning in the plot. Two examples are shown below. The first one shows the histogram when the BER Test has been passed (BER=0), whereas the second one shows the histogram when the BER Test has been failed (BER>0) by plugging out the Display Port cable from the SCC for a brief moment during the test.

![BER Test Pass](images/bertest/BERtestPass.png){width=350}![BER Test Fail](images/bertest/BERtestFail.png){width=350}