import mysql.connector
import getpass


password = getpass.getpass("Enter your password:")
#def GetTrims(password):
connection = mysql.connector.connect(
    host="cmsfpixdb.physics.purdue.edu",
    user="cmsfpix_phase2_user",
    password=password,
    database="cmsfpix_phase2"
)


serialNumber = "RH0006"
cursor = connection.cursor()
# Execute your SQL query
cursor.execute(f"select component.id from component where component.serial_number='{serialNumber}';")
results = cursor.fetchall()
ModuleParentID=results[0]
#result : [(778,)]
parenetNum = results[0][0]

cursor.execute(f"select component.description from component where component.serial_number='{serialNumber}';")
results = cursor.fetchall() #[('CROC 1x2 HPK sensor module',)]
#print(results[0][0])

if "sensor" in str(results[0][0]):
    #print("sensor module")
    cursor.execute(f"select component.id from component where component.parent='{parenetNum}';")
    chipSensorResult=cursor.fetchall()
    secondParent=chipSensorResult[0][0]
    print("secondParent" + str(secondParent))
    parenetNum = secondParent


#Q: how to find how many chips does a module have

#get VDDA value
VDDAList = []
cursor.execute(f"SELECT component.serial_number,component.parent,measurement.name,type,component.site,ival from component,measurements,measurement where component.parent = {parenetNum} and measurements.name=measurement.name and component.id=measurement.part_id and measurement.name='TRIM_VDDA';")
results = cursor.fetchall()
for result in results:
    print(result)
    VDDA = result[-1]
    siteNum = result[-2]
    VDDAList.append([siteNum,VDDA])
sorted_VDDAlist = sorted(VDDAList, key=lambda x: x[0])
print("sorted_VDDAlist:"+str(sorted_VDDAlist))



VDDDList = []
cursor.execute(f"SELECT component.serial_number,component.parent,measurement.name,type,component.site,ival from component,measurements,measurement where component.parent = {parenetNum} and measurements.name=measurement.name and component.id=measurement.part_id and measurement.name='TRIM_VDDD';")
results = cursor.fetchall()
for result in results:
    print(result)
    VDDD = result[-1]
    siteNum = result[-2]
    VDDDList.append([siteNum,VDDD])

sorted_VDDDlist = sorted(VDDDList, key=lambda x: x[0]) #make sure the we can get VDDD value base on the order of rising chip no
print("sorted_VDDDlist:" + str(sorted_VDDDlist))
connection.close()
#return sorted_VDDAlist,sorted_VDDDlist


#if __name__ == "__main":
#    password = getpass.getpass("Enter your password:")
#    sorted_VDDAlist,sorted_VDDDlist=GetTrims(password)
#    print("sorted_VDDAlist(in order site,trim value):" + str(sorted_VDDAlist))
#    print("VDDD:" + str(sorted_VDDDlist))



#test code
"""
#geting chip 1 VDDA value
cursor.execute(f"SELECT component.serial_number,component.parent,measurement.name,type,component.site,ival from component,measurements,measurement where component.parent = {parenetNum} and measurements.name=measurement.name and component.id=measurement.part_id and measurement.name='TRIM_VDDA' and component.site=0;")
results = cursor.fetchall()
#result : [('NY6250-04F3_9-C', 778, 'TRIM_VDDA', 'int', 0, 7)]  in the order or chip sertial number, parent number, trim value type,site(relate to chip No), trim value 
print(results[0][-1])

#getting chip 2 VDDD value
cursor.execute(f"SELECT component.serial_number,component.parent,measurement.name,type,component.site,ival from component,measurements,measurement where component.parent = {parenetNum} and measurements.name=measurement.name and component.id=measurement.part_id and measurement.name='TRIM_VDDD' and component.site=1;")
results = cursor.fetchall()
#result : [('NY6250-04F3_9-C', 778, 'TRIM_VDDA', 'int', 0, 7)]  in the order or chip sertial number, parent number, trim value type,site(relate to chip No), trim value 
print(results[0][-1])
"""




"""    
# Close the cursor only after processing all the results
    cursor.close()

except mysql.connector.Error as err:
    print("MySQL Error:", err)

finally:
    # Make sure to close the database connection when done
    connection.close()
    """