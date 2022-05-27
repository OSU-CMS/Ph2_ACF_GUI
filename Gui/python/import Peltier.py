import Peltier

pelt = Peltier.PeltierController("com4", 9600)
message = ['*','0','0','0','0','0','0','f','a','e','7','^']
a = pelt.readTemperature(message)
print(a)