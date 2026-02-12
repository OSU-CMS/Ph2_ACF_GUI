"""
10-23 
what happen if the serial number is not being register on the database?
the storing list will be empty
"""


import mysql.connector
import getpass




from Gui.python.logging_config import get_logger
logger = get_logger(__name__)

def GetTrims(password,serialNumber,debug = False):
    connection = mysql.connector.connect(
        host="cmsfpixdb.physics.purdue.edu",
        user="cmsfpix_phase2_user",
        password=password,

        database="cmsfpix_phase2"
    )

    #serialNumber = "RH0010"
    #serialNumber = "RH0006"
    cursor = connection.cursor()
    cursor.execute(f"select component.id from component where component.serial_number='{serialNumber}';")
    results = cursor.fetchall()
    if debug == True:
        logger.debug("raw ID:"+str(result))# it should look like [(778,)]
    parenetNum = results[0][0]

    cursor.execute(f"select component.description from component where component.serial_number='{serialNumber}';")
    results = cursor.fetchall() #[('TFPX CROC 1x2 HPK sensor module',)]
    if debug == True:
        logger.debug("raw description"+str(results))

    if "sensor" in str(results[0][0]):
        cursor.execute(f"select component.id from component where component.parent='{parenetNum}';")
        chipSensorResult=cursor.fetchall()
        secondParent=chipSensorResult[0][0]
        if debug == True:
            logger.debug("it is sensor module")
            logger.debug("secondParent" + str(secondParent))
        parenetNum = secondParent


    #get VDDA value
    VDDAList = []
    cursor.execute(f"SELECT component.serial_number,component.parent,measurement.name,type,component.site,ival from component,measurements,measurement where component.parent = {parenetNum} and measurements.name=measurement.name and component.id=measurement.part_id and measurement.name='TRIM_VDDA';")
    results = cursor.fetchall()
    for result in results:
        VDDA = result[-1]
        siteNum = result[-2]
        VDDAList.append([siteNum,VDDA])
    sorted_VDDAlist = sorted(VDDAList, key=lambda x: x[0])
    if debug == True:
        logger.debug("sorted_VDDAlist:"+str(sorted_VDDAlist))



    VDDDList = []
    cursor.execute(f"SELECT component.serial_number,component.parent,measurement.name,type,component.site,ival from component,measurements,measurement where component.parent = {parenetNum} and measurements.name=measurement.name and component.id=measurement.part_id and measurement.name='TRIM_VDDD';")
    results = cursor.fetchall()
    for result in results:
        VDDD = result[-1]
        siteNum = result[-2]
        VDDDList.append([siteNum,VDDD])

    sorted_VDDDlist = sorted(VDDDList, key=lambda x: x[0]) #make sure the we can get VDDD value base on the order of rising chip no
    if debug == True:
        logger.debug("sorted_VDDDlist:" + str(sorted_VDDDlist))
    connection.close()
    return sorted_VDDAlist,sorted_VDDDlist


if __name__ == "__main__":
    password = getpass.getpass("Enter your password:")
    serialNumber = "RH0001"
    sorted_VDDAlist,sorted_VDDDlist=GetTrims(password,serialNumber)
    logger.info("sorted_VDDAlist(in order site,trim value):" + str(sorted_VDDAlist))
    logger.info("VDDD:" + str(sorted_VDDDlist))


