# Spread of the threshold vs. `LDAC_LIN` measurement

This is a small tool for measuring pixel threshold spread as a function of `LDAC_LIN` using the `Ph2_ACF` framework.
The tool consists of a shell script (`LDACLINMeasurement.sh`) and a ROOT macro (`LDACLINCalibration.C`). The shell script performs all the needed measurements (*Threshold Equalization* and *S-curve* measurements for each point of `LDAC_LIN`) and needs to be ran first. The ROOT macro analyzes the measured data and finds the best `LDAC_LIN` value so it needs to be ran when all the measurements are finished.

Four versions of the measurement are available:
1. The "full" version: *Threshold Equalization* is followed by an *S-curve* measurement for each of 23 (or as many as you like) points with different `LDAC_LIN` values. The best value of `LDAC_LIN` is determined by the smallest spread of the threshold (which we get from S-curves).
2. The "short" version: only *Threshold Equalization* is ran for each value of `LDAC_LIN` (no *S-curve*). The "best" value is chosen by the smallest spread of pixel efficiencies at nominal threshold. This method only aproximately shows where the real best `LDAC_LIN` may be (from the tests made so far it seems that real best value is a bit higher than the one given by the "short" version).
3. The "normal" version: only *Threshold Equalization* is ran for each value of `LDAC_LIN` (no *S-curve*). 7 Best points are selected from the spread of pixel efficiencies at nominal threshold. *S-curve* measurements are lated done on these points only.
4. The "extra" version: does only the second part of the "normal" version. Do this if you have done the "short" version previously.

You might want to check [this short presentation](https://indico.cern.ch/event/1069283/contributions/4496542/attachments/2300018/3912301/IT_DAQ_MarijusAmbrozas_21-08-30.pdf) to see the idea and what to expect.

### Instructions on how to use the tool:
1. Make sure you have the `Ph2_ACF` software ready and compiled on your setup. Also make sure you have sourced `setup.sh`.
2. Simply run the shell script inside your working directory (it will run Threshold Equalization and S-curve 23 (or as many as you like) times with different `LDAC_LIN` values):
```
LDACLINMeasurement.sh
# Use flag "-s" to run the "short" version of "-l" for the "long" version
# LDACLINMeasurement.sh -s
```
3. There are many different flags available:<br>
   3.1. `-h` or `--help` – show brief help<br>
   3.2. `-s` or `--short` – do the "short" measurement version<br>
   3.3. `-l` or `--long` – do the "long" measurement version<br>
   3.4. `-e` or `--extra` – do the "extra" measurement version<br>
   3.5. `-o` or `--overwrite` – overwrite measurement root files if they already exist (default is to just skip the measurement if the file already exists)<br>
   3.6. `-a` or `--analyze` – automatically run the root analysis macro after the measurement is done (without the graphical outputs)<br>
   3.7. `-f XML.xml` or `--file=XML.xml` – specify the xml file (default is `CMSIT.xml`)<br>
   3.8. `-m measurementSet` or `--measurement=measurementSet` – specify the measurement set (in short, it is the output directory name, useful if you are doing many measurements to keep track of them)<br>
   3.9. `-v values.txt` or `--values=values.txt` – specify the text file containing all the LDAC_LIN values you want to try. If not specified, a default set of 23 points will be used.
   
4. Copy the ROOT macro to your working directory:
```
cp ../ProductionToolsIT/LDACLINCalibration/LDACLINCalibration.C .
```
5. Run the ROOT macro to analyze the results (you should specify the measurement set as a string argument):
```
root -l
.x LDACLINCalibration.C("myChip_20")
// If you ran the "short" measurement, you need to specify the second string argument as "SHORT" (or "S") to the ROOT macro
// Analogically, specify the argument "LONG" (or "L") for the "long" measurement
// Leave an empty string ("") for the "normal" measurement
// .x LDACLINCalibration.C("myChip_20", "SHORT")
```
6. You should see a graph showing the spread of the threshold vs. `LDAC_LIN` (spread of efficiencies vs. `LDAC_LIN` if running the "short" version). A clear minimum should be apparent.
7. You may repeat the measurement at different conditions (e.g., different temperatures) to check if the result is stable.

If you have any questions and/or suggestions, please contact Marijus Ambrozas (marijus.ambrozas@cern.ch).
