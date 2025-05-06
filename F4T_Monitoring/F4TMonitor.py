#---------------------------------------
# Arduino-Python Ethernet Communication
#---------------------------------------
from flask import Flask, render_template, jsonify
import socket
import matplotlib.pyplot as plt
import matplotlib, threading, time, csv, yagmail, os
from datetime import datetime

class F4TMonitor():
    def __init__(self):
        self.alertRecipients=["speck.57@buckeyemail.osu.edu"]#,"delossantos.22@buckeyemail.osu.edu","joyce.279@osu.edu"]
        self.alertSender="osupixels@gmail.com"
        self.appPassword="jkwb jeez fvmb gdns"
        self.timeBetweenEmails = 3600
        self.lastEmailTime=-self.timeBetweenEmails

        base_dir = '/home/pixels/Workspaces/Steve/march5Ph2_ACF_GUI/Ph2_ACF_GUI/F4T_Monitoring' #os.environ.get("PH2ACF_BASE_DIR")+'/F4T_Monitoring'
        self.imagesPath = base_dir+'/static/images/'
        self.logsPath = base_dir+'/dht_logs/'

        self.tempDangerLow = -45
        self.tempDangerHigh = 45
        self.humidityDangerHigh = 30

        self.files = [os.path.join(self.logsPath, f) for f in os.listdir(self.logsPath) if os.path.isfile(os.path.join(self.logsPath, f))]
        self.logFile = os.path.basename(max(self.files, key=os.path.getctime)) if len(self.files)!=0 else "dht_logs_"+datetime.now().strftime("%Y-%m-%d %H:%M:%S")+".csv"
        
        self.serverPort = 3000
        self.serverIP = '0.0.0.0'
        self.arduinoPort = 8081
        self.arduinoIP = 'xxx.xxx.xx.xx' #GET FROM ARDUINO OUTPUT
        #tailscaleIP = '100.122.12.41'
        #tailscaleMachineLink = "https://login.tailscale.com/admin/invite/AfKCYUaFR2c"
        self.remoteIP = '128.146.32.122'

        matplotlib.use('agg')

        self.tempFig = plt.figure()
        plt.plot([],[],color='green',label='Temperature (°C)')
        plt.title("Temperature (°C)")
        plt.xlabel("Seconds since start")
        plt.ylabel("Temperature (°C)")
        plt.savefig(self.imagesPath+'temp.png')

        self.humidityFig = plt.figure()
        plt.plot([],[],color='blue',label='Humidity (%)')
        plt.title("Humidity (%)")
        plt.xlabel("Seconds since start")
        plt.ylabel("Humidity (%)")
        plt.savefig(self.imagesPath+'humidity.png')

        self.datalogs = []
        self.developmentMode=False

        self.address = (self.arduinoIP, self.arduinoPort)
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.settimeout(5)

        self.tailData=[]

    def dangerZoneQ(self, myDatalogs):
        if len(myDatalogs)==0:
            return False
        elif (float(myDatalogs[-1][1])>=self.tempDangerHigh or float(myDatalogs[-1][1])<=self.tempDangerHigh or float(myDatalogs[-1][2])>=self.humidityDangerHigh):
            return True
        else:
            return False

    def emailAlerts(self, subject):
        if time.time() - self.lastEmailTime > self.timeBetweenEmails:
            for receiver in self.alertRecipients:
                yag=yagmail.SMTP(self.alertSender,password=self.appPassword)
                yag.send(
                    to=receiver,
                    subject=subject,
                    contents=[
                        yagmail.inline(self.imagesPath+'temp.png'),
                        yagmail.inline(self.imagesPath+'humidity.png'),
                        "\n".join(self.tailData),
                        "\n",
                        "To view live data, join the lab VPN and run: ssh -L [local port]:localhost:3000 pixels@128.146.32.122 \n\
                        Then view live data on your local browser at http://localhost:[local port]"
                        #Use this link to access the host machine: "+tailscaleMachineLink
                    ]
                )
            self.lastEmailTime = time.time()

    def updateData(self):
        while True:
            self.sock.sendto(bytes("h", 'utf-8'), self.address)
            try:
                data, _ = self.sock.recvfrom(2048)
                data = data.decode('utf-8')
                print(data)

                if data[0]=="!":
                    self.logFile="dht_logs_"+datetime.now().strftime("%Y-%m-%d %H:%M:%S")+".csv"
                    self.datalogs=[]
                    data=data[1:]

                    plt.figure(self.tempFig.number)
                    plt.clf()
                    plt.plot([],[],color='green',label='Temperature (°C)')
                    plt.title("Temperature (°C)")
                    plt.xlabel("Seconds since start")
                    plt.ylabel("Temperature (°C)")
                    plt.savefig(self.imagesPath+'temp.png')

                    plt.figure(self.humidityFig.number)
                    plt.clf()
                    plt.plot([],[],color='blue',label='Humidity (%)')
                    plt.title("Humidity (%)")
                    plt.xlabel("Seconds since start")
                    plt.ylabel("Humidity (%)")
                    plt.savefig(self.imagesPath+'humidity.png')

                data=data.split(',')
                self.datalogs.append(data)

                string="Seconds since start: "+data[0]+", Temperature (°C): "+data[1]+", Humidity (%): "+data[2]
            except Exception as e:
                if str(e)=="timed out":
                    string = "Arduino connection timed out."
                else:
                    string = str(e)
                print(e)
            self.tailData.insert(0,string)
            if len(self.tailData)>6: self.tailData.pop() 
        
            if len(self.datalogs)>15 or (self.dangerZoneQ(self.datalogs) and time.time() - self.lastEmailTime > self.timeBetweenEmails) or (self.developmentMode and len(self.datalogs)>0):
                with open(self.logsPath+self.logFile,'a',newline='') as file:
                    writer = csv.writer(file)
                    writer.writerows(self.datalogs)

                with open(self.logsPath+self.logFile,'r') as file:
                    reader = csv.reader(file)
                    readdata = [row for row in reader]
                    times = [float(row[0]) for row in readdata]
                    temps = [float(row[1]) for row in readdata]
                    humidities = [float(row[2]) for row in readdata]
                    
                    tempMin = min(temps)
                    tempMax = max(temps)
                    humidityMax = max(humidities)

                    plt.figure(self.tempFig.number)
                    plt.plot(times,temps,color='green',label='Temperature (°C)')
                    if tempMax > self.tempDangerHigh:
                        plt.fill_between(times, [tempMax]*len(times), self.tempDangerHigh, color='red', alpha=1)
                    if tempMin < self.tempDangerLow:
                        plt.fill_between(times, [tempMin]*len(times), self.tempDangerLow, color='red', alpha=1)

                    plt.savefig(self.imagesPath+'temp.png')

                    plt.figure(self.humidityFig.number)
                    plt.plot(times,humidities,color='blue',label='Humidity (%)')
                    if humidityMax > self.humidityDangerHigh:
                        plt.fill_between(times, [humidityMax]*len(times), self.humidityDangerHigh, color='red', alpha=1)

                    plt.savefig(self.imagesPath+'humidity.png')

                    if float(self.datalogs[-1][1])>self.tempDangerHigh and float(self.datalogs[-1][2])>self.humidityDangerHigh:
                        self.emailAlerts("F4T temperature above "+str(self.tempDangerHigh)+"°C and humidity above "+str(self.humidityDangerHigh)+"°C")
                    elif float(self.datalogs[-1][1])<self.tempDangerLow and float(self.datalogs[-1][2])>self.humidityDangerHigh:
                        self.emailAlerts("F4T temperature below "+str(self.tempDangerLow)+"°C and humidity above "+str(self.humidityDangerHigh)+"°C")
                    else:
                        if float(self.datalogs[-1][1])>self.tempDangerHigh:
                            self.emailAlerts("F4T temperature above "+str(self.tempDangerHigh)+"°C")
                        elif float(self.datalogs[-1][1])<self.tempDangerLow:
                            self.emailAlerts("F4T temperature below "+str(self.tempDangerLow)+"°C")
                        elif float(self.datalogs[-1][2])>self.humidityDangerHigh:
                            self.emailAlerts("F4T humidity above "+str(self.humidityDangerHigh)+"°C")
                self.datalogs=[]  
            
            time.sleep(1)

if __name__ == "__main__":
    monitor = F4TMonitor()
    f4t_site = Flask(__name__)

    @f4t_site.route('/')
    def home():
        return render_template("index.html")

    @f4t_site.route('/data')
    def get_data():
        return jsonify({"value": "\n".join(monitor.tailData)})
    
    thread = threading.Thread(target=monitor.updateData)
    thread.daemon = True
    thread.start()

    f4t_site.run(host=monitor.serverIP,port=monitor.serverPort,debug=False)