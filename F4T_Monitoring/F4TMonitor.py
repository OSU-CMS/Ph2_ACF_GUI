#---------------------------------------
# Arduino-Python Ethernet Communication
#---------------------------------------
from flask import Flask, render_template, jsonify
import socket
from dotenv import load_dotenv
import matplotlib.pyplot as plt
import matplotlib, threading, time, csv, yagmail, os
from datetime import datetime

load_dotenv()  # Load environment variables from .env file; required for App password

class F4TMonitor():
    def __init__(self):
        self.alertRecipients=["pratap.uppalapati@gmail.com","uppalapati.13@buckeyemail.osu.edu"] #emails
        self.alertSender="ph2osupixels@gmail.com"
        self.appPassword=os.getenv("APP_PASSWORD")
        self.timeBetweenEmails = 3600 # Send emails every 60 minutes
        self.lastEmailTime=-self.timeBetweenEmails

        raw_base = os.environ.get("PH2ACF_BASE_DIR")

        if raw_base:
            base_dir = os.path.join(raw_base, "F4T_Monitoring")
        else:
            print("PH2ACF_BASE_DIR is not set → Using current directory")
            # FIX: Fallback to current working directory so base_dir always exists
            base_dir = os.getcwd()

        self.logsPath = os.path.join(base_dir, "dht_logs")
        self.imagesPath = os.path.join(base_dir, "static/images/")

        os.makedirs(self.imagesPath, exist_ok=True)

        os.makedirs(self.logsPath, exist_ok=True)
        print("Base Dir:", base_dir)
        print("Images Path:", self.imagesPath)

        self.tempDangerLow = 0  #Originally -45, but changed to 0 for testing purposes
        self.tempDangerHigh = 27  #Originally 45, but changed to 27 for testing purposes 
        self.humidityDangerHigh = 60

        self.logFile = "dht_logs_" + datetime.now().strftime("%Y-%m-%d_%H-%M-%S") + ".csv"

        self.serverPort = 3000
        self.serverIP = '0.0.0.0'
        self.arduinoPort = 8081
        self.arduinoIP = '128.146.33.117' #GET FROM ARDUINO OUTPUT
        self.remoteIP = '128.146.32.122'

        matplotlib.use('agg')

        self.tempFig = plt.figure()
        plt.plot([],[],color='green',label='Temperature (°C)')
        plt.title("Temperature (°C)")
        plt.xlabel("Seconds since start")
        plt.ylabel("Temperature (°C)")
        plt.ylim(self.tempDangerLow-5, self.tempDangerHigh+10)
        plt.savefig(self.imagesPath+'temp.png')

        self.humidityFig = plt.figure()
        plt.plot([],[],color='blue',label='Humidity (%)')
        plt.title("Humidity (%)")
        plt.xlabel("Seconds since start")
        plt.ylabel("Humidity (%)")
        plt.ylim(0, self.humidityDangerHigh+10)
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
        elif (float(myDatalogs[-1][1])>=self.tempDangerHigh or float(myDatalogs[-1][1])<=self.tempDangerLow or float(myDatalogs[-1][2])>=self.humidityDangerHigh):
            return True
        else:
            return False

    def emailAlerts(self, subject):
        try:
            if time.time() - self.lastEmailTime < self.timeBetweenEmails:
                print(f"Email skipped: Only {int(time.time() - self.lastEmailTime)}s passed.")
                return 

            temp_img = os.path.join(self.imagesPath, 'temp.png')
            hum_img = os.path.join(self.imagesPath, 'humidity.png')

            email_contents = [
                "Ph2_ACF_GUI/F4T_Monitoring Alert",
                "\nRecent Data Points:",
                "\n".join(self.tailData),
                "\nInstructions:",
                "To view live data, join the lab VPN and run: ssh -L [local port]:localhost:3000 pixels@128.146.32.122",
                "Then view live data at http://localhost:[local port]"
            ]

            if os.path.exists(temp_img) and os.path.exists(hum_img):
                email_contents.insert(0, yagmail.inline(temp_img))
                email_contents.insert(1, yagmail.inline(hum_img))
            else:
                print("Warning: Graphs not found. Sending text-only alert.")

            yag = yagmail.SMTP(self.alertSender, password=self.appPassword)
            for receiver in self.alertRecipients:
                print(f"Sending to {receiver}...")
                yag.send(to=receiver, subject=subject, contents=email_contents)

            self.lastEmailTime = time.time()
            print("Email sent successfully.")

        except Exception as e:
            print(f"CRITICAL EMAIL ERROR: {e}")

    def updateData(self):
        while True:
            self.sock.sendto(bytes("h", 'utf-8'), self.address)

            try:
                data, _ = self.sock.recvfrom(2048)
                data = data.decode('utf-8').strip() 

                # --- RESET SIGNAL BLOCK ---
                if data[0] == "!":  
                    self.logFile = "dht_logs_" + datetime.now().strftime("%Y-%m-%d_%H-%M-%S") + ".csv"
                    self.datalogs = []
                    data = data[1:]
                    
                    print("Reset signal received: Clearing plots for new session") 

                    plt.figure(self.tempFig.number)
                    plt.clf()
                    plt.plot([], [], color='green', label='Temperature (°C)') 
                    plt.title("Temperature (°C)")
                    plt.xlabel("Seconds since start")
                    plt.ylabel("Temperature (°C)")
                    plt.savefig(os.path.join(self.imagesPath, 'temp.png'))

                    plt.figure(self.humidityFig.number)
                    plt.clf()
                    plt.plot([], [], color='blue', label='Humidity (%)') 
                    plt.title("Humidity (%)")
                    plt.xlabel("Seconds since start")
                    plt.ylabel("Humidity (%)")
                    plt.savefig(os.path.join(self.imagesPath, 'humidity.png'))

                data = data.split(',')
                self.datalogs.append(data)

                string = "Seconds since start: " + data[0] + ", Temperature (°C): " + data[1] + ", Humidity (%): " + data[2]
                
            except Exception as e:
                if str(e) == "timed out":
                    string = "Arduino connection timed out."
                else:
                    string = str(e)
                print(e)
                
            self.tailData.insert(0, string)
            if len(self.tailData) > 6: 
                self.tailData.pop() 
        
            # --- PLOTTING & SAVING BLOCK ---
            if len(self.datalogs) > 15 or (self.dangerZoneQ(self.datalogs) and time.time() - self.lastEmailTime > self.timeBetweenEmails) or (self.developmentMode and len(self.datalogs) > 0):
                
                log_file_path = os.path.join(self.logsPath, self.logFile)
                
                try:
                    with open(log_file_path, 'a', newline='') as file:
                        writer = csv.writer(file)
                        writer.writerows(self.datalogs)

                    with open(log_file_path, 'r') as file:
                        reader = csv.reader(file)
                        readdata = [row for row in reader]
                        times = [float(row[0]) for row in readdata]
                        temps = [float(row[1]) for row in readdata]
                        temps = [t if t > -270 else float('nan') for t in temps] # Replace any temp values below -270 with NaN to avoid plotting them
                        humidities = [float(row[2]) for row in readdata]
                        
                        tempMin = min(temps)
                        tempMax = max(temps)
                        humidityMax = max(humidities)

                        plt.figure(self.tempFig.number)
                        plt.clf()
                        plt.plot(times, temps, color='green', label='Temperature (°C)')
                        plt.title("Temperature (°C): " + str(self.tempDangerHigh) + "°C high, " + str(self.tempDangerLow) + "°C low")
                        plt.xlabel("Seconds since start")
                        plt.ylabel("Temperature (°C)")
                        plt.ylim(self.tempDangerLow-5, self.tempDangerHigh+10)
                        
                        
                        if tempMax > self.tempDangerHigh:
                            plt.fill_between(times, [tempMax]*len(times), self.tempDangerHigh, color='red', alpha=1)
                        if tempMin < self.tempDangerLow:
                            plt.fill_between(times, [tempMin]*len(times), self.tempDangerLow, color='red', alpha=1)

                        plt.savefig(os.path.join(self.imagesPath, 'temp.png'))


                        plt.figure(self.humidityFig.number)
                        plt.clf()
                        plt.plot(times, humidities, color='blue', label='Humidity (%)')
                        plt.title("Humidity (%): " + str(self.humidityDangerHigh) + "% high")
                        plt.xlabel("Seconds since start")
                        plt.ylabel("Humidity (%)")

                        plt.ylim(0, self.humidityDangerHigh+10) # Danger Zone + some padding

                        if humidityMax > self.humidityDangerHigh:
                            plt.fill_between(times, [humidityMax]*len(times), self.humidityDangerHigh, color='red', alpha=1,linestyle='--')

                        plt.savefig(os.path.join(self.imagesPath, 'humidity.png'))
                
                except Exception as e:
                    print(f"CRITICAL PLOTTING/SAVING ERROR: {e}")

                # Email Logic
                if float(self.datalogs[-1][1]) > self.tempDangerHigh and float(self.datalogs[-1][2]) > self.humidityDangerHigh:
                    self.emailAlerts("F4T temperature above " + str(self.tempDangerHigh) + "°C and humidity above " + str(self.humidityDangerHigh) + "°C")
                elif float(self.datalogs[-1][1]) < self.tempDangerLow and float(self.datalogs[-1][2]) > self.humidityDangerHigh:
                    self.emailAlerts("F4T temperature below " + str(self.tempDangerLow) + "°C and humidity above " + str(self.humidityDangerHigh) + "°C")
                else:
                    if float(self.datalogs[-1][1]) > self.tempDangerHigh:
                        self.emailAlerts("F4T temperature above " + str(self.tempDangerHigh) + "°C")
                    elif float(self.datalogs[-1][1]) < self.tempDangerLow:
                        self.emailAlerts("F4T temperature below " + str(self.tempDangerLow) + "°C")
                    elif float(self.datalogs[-1][2]) > self.humidityDangerHigh:
                        self.emailAlerts("F4T humidity above " + str(self.humidityDangerHigh) + "°C")
                
                self.datalogs = []  
            
            time.sleep(1)

if __name__ == "__main__":
    monitor = F4TMonitor()

    exact_static_dir = os.path.abspath(os.path.join(monitor.imagesPath, os.pardir))
    f4t_site = Flask(__name__, static_folder=exact_static_dir)

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
