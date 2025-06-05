#!/bin/bash
# Shell script to perform multiple Threshold Equalization procedures and S-curve measurements with different LDAC_LIN values in Ph2_ACF
# Created by Marijus Ambrozas (marijus.ambrozas@cern.ch)
# Aug. 30 2021

short_flag=0
long_flag=0
extra_flag=0
overwrite_flag=0
analyze_flag=0
xml_file="CMSIT.xml"
measurement_set="ChipX_1"

# Getting the options
while test $# -gt 0; do
	case "$1" in
		-h|--help)
			echo "LDAC_LIN measurement - LDAC_LIN optimization for minimal threshold spread"
			echo " "
			echo "LDACLINMeasurement.sh [options]"
			echo " "
			echo "options:"
			echo "-h, --help                          Show brief help"
			echo "-s, --short       	          \"Short mode\" - perform only threqu (no scurve)"
			echo "-l, --long		          \"Long mode\" - perform threqu+scurve for every point"
			echo "-e, --extra		          \"Extra mode\" - perform threqu+scurve only on a subset of points (selected after the \"Short\" mode)"
			echo "-f XML.xml, --file=XML.xml          specify the xml file you are using (default is CMSIT.xml)"
			echo "-m ChipX_1, --measurement=ChipX_1	  specify the name for the measurement (output directory name)"
			echo "-v VALUES.txt --values=VALUES.txt   specify the text file with LDAC_LIN values you want to use (default list will be used if not specified)"
			echo "-o, --overwrite		          overwrite already existing root files (if you ran this measurement previously)"
			echo "-a, --analyze		          run measurement analysis (LDAC_LIN_calibration.C) automatically after the measurement is done"
			exit 0
			;;
		-s|--short)
			short_flag=1
      			shift
			;;
		-l|--long)
			long_flag=1
      			shift
			;;
		-e|--extra)
			extra_flag=1
      			shift
			;;
		-o|--overwrite)
			overwrite_flag=1
      			shift
			;;
		-a|--analyze)
			analyze_flag=1
      			shift
			;;
		-f)
			shift
			if test $# -gt 0; then
				xml_file=$1
			else
				echo "No xml file specified!"
				exit 1
			fi
			shift
			;;
		--file*)
			xml_file=`echo $1 | sed -e 's/^[^=]*=//g'`
			shift
			;;
		-m)
			shift
			if test $# -gt 0; then
				measurement_set=$1
			else
				echo "No measurement name specified!"
				exit 1
			fi
			shift
			;;
		--measurement*)
			measurement_set=`echo $1 | sed -e 's/^[^=]*=//g'`
			shift
			;;
		-v)
			shift
			if test $# -gt 0; then
				values_file=$1
			else
				echo "No file for LDAC_LIN values specified!"
				exit 1
			fi
			shift
			;;
		--values*)
			values_file=`echo $1 | sed -e 's/^[^=]*=//g'`
			shift
			;;
		*)
			break
			;;
	esac
done

# Exit if contradiction in the options
if ([ ${short_flag} -gt 0 ] && [ ${long_flag} -gt 0 ]) || ([ ${short_flag} -gt 0 ] && [ ${extra_flag} -gt 0 ]) || ([ ${long_flag} -gt 0 ] && [ ${extra_flag} -gt 0 ])
then
	echo "Error: please use no more than one option of [long, short, extra]."
	exit
fi
# Exit if XML file not found
if [ -z "$(ls ${xml_file} 2>/dev/null)" ]
then
	echo "Error: xml file ${xml_file} was not found!"
	exit
fi

# Setting the measurement values
if [ -z ${values_file} ] || [ -z "$(ls ${values_file} 2>/dev/null)" ]
then
	values=(1 10 50 60 70 80 90 100 110 120 130 140 150 160 170 180 190 200 400 800 1500 3000 8000)
else
	IFS=$'\n 	' read -d '' -r -a values < ${values_file}
fi
# Exit if less than 7 LDAC_LIN points are specified
if [ ${#values[@]} -lt 7 ]
then
	echo "Error: less than 7 LDAC_LIN values specified. Please specify 7 or more points."
	exit
fi

mkdir -p Results/${measurement_set}

# Converting flags to run instructions
run_part1=1 # threqu for all points
run_part2=0 # scurve for all points 
run_part3=1 # run ROOT to find a subset of best points
run_part4=1 # threqu+scurve for a subset of best points
if [ ${short_flag} -gt 0 ]
then
	run_part3=0
	run_part4=0
elif [ ${long_flag} -gt 0 ]
then
	run_part2=1
	run_part3=0
	run_part4=0
elif [ ${extra_flag} -gt 0 ]
then
	run_part1=0
	run_part3=0
fi

# RUN THE MEASUREMENT
if [ ${run_part1} -gt 0 ]
then
	echo "Running for all points"
	for value in "${values[@]}"
	do :
		# THRESHOLD EQUALIZATION
		# Checking if files from previous measurement exist
		if [ -z "$(ls Results/${measurement_set}/LDACLIN_${value}.root 2>/dev/null)" ] || [ ${overwrite_flag} -gt 0 ]
		then
			echo Setting LDAC_LIN=${value}
			sed -i -e 's/LDAC_LIN               =  .*/LDAC_LIN               =  "'${value}'"/g'  ${xml_file}
	
			echo "Running threshold equalization"
			CMSITminiDAQ -f ${xml_file} -c threqu
		
			filename=$(ls -ltr Results/Run*.root 2>/dev/null |tail -n 1| awk '{print $9}')
			if [ -z "${filename}" ]
			then
				echo "No threqu output file found!"
			else
				echo "Moving the file ${filename}"
				mv ${filename} Results/${measurement_set}/LDACLIN_${value}.root
			fi
		else
			echo "File Results/${measurement_set}/LDACLIN_${value}.root does already exist. Use the -o flag if you want to overwrite."
		fi


		# Skip if not running part 2
		if [ ${run_part2} -eq 0 ]
		then
			continue
		fi

		# S-CURVE
		# Checking if files from previous measurement exist
		if [ -z "$(ls Results/${measurement_set}/scurve_LDACLIN_${value}.root 2>/dev/null)" ] || [ ${overwrite_flag} -gt 0 ]
		then
			echo "Running s-curve"
			CMSITminiDAQ -f ${xml_file} -c scurve

			filename=$(ls -ltr Results/Run*.root |tail -n 1| awk '{print $9}')
			if [ -z "${filename}" ]
			then
				echo "No scurve output file found!"
			else
				echo "Moving the file ${filename}"
				mv ${filename} Results/${measurement_set}/scurve_LDACLIN_${value}.root
			fi
		else
			echo "File Results/${measurement_set}/scurve_LDACLIN_${value}.root does already exist. Use the -o flag if you want to overwrite."
		fi
	done
fi

# Run ROOT analysis macro to find best points and save them to file for default mode
if [ ${run_part3} -gt 0 ]
then
	root -l -q -b $PWD/LDACLINCalibration.C\(\"${measurement_set}\"\,\ \"SHORT\"\)
fi

# Make s-curve measurements on a subset of points with reading from the file
if [ ${run_part4} -gt 0 ]
then
	echo "Running for a subset of best points"
	file="Results/${measurement_set}/LDACLINCandidates.txt"
	while IFS= read -r value
	do
		echo Setting LDAC_LIN=${value}
		sed -i -e 's/LDAC_LIN               =  .*/LDAC_LIN               =  "'${value}'"/g'  ${xml_file}

		# THRESHOLD EQUALIZATION (again)
		echo "Running threshold equalization"
		CMSITminiDAQ -f ${xml_file} -c threqu

		# S-CURVE
		# Checking if files from previous measurement exist
		if [ -z "$(ls Results/${measurement_set}/scurve_LDACLIN_${value}.root 2>/dev/null)" ] || [ ${overwrite_flag} -gt 0 ]
		then
			echo "Running s-curve"
			CMSITminiDAQ -f ${xml_file} -c scurve

			filename=$(ls -ltr Results/Run*.root |tail -n 1| awk '{print $9}')
			if [ -z "${filename}" ]
			then
				echo "No scurve output file found!"
			else
				echo "Moving the file ${filename}"
				mv ${filename} Results/${measurement_set}/scurve_LDACLIN_${value}.root
			fi
		else
			echo "File Results/${measurement_set}/scurve_LDACLIN_${value}.root does already exist. Use the -o flag if you want to overwrite."
		fi
	
	done <"${file}"
fi

# Run ROOT to find the best LDAC_LIN value
if [ ${analyze_flag} -gt 0 ]
then
	if [ ${short_flag} -gt 0 ]
	then
		root -l -q -b $PWD/LDACLINCalibration.C\(\"${measurement_set}\"\,\ \"SHORT\"\)
	elif [ ${long_flag} -gt 0 ]
	then
		root -l -q -b $PWD/LDACLINCalibration.C\(\"${measurement_set}\"\,\ \"LONG\"\)
	else
		root -l -q -b $PWD/LDACLINCalibration.C\(\"${measurement_set}\"\,\ \"\"\)
	fi
fi
