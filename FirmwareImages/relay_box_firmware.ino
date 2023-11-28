const int sldo_pincount = 9;
const int pins[11] = {2,3,4,5,6,7,8,9,10,11,12}; //2-10 SLDO (10 GND), 11-12 NTC
int choice = -1;
int connection = -1;
char connection_char;

void setup()
{
  Serial.begin(9600);
  for (int i=0; i<=10; i++)
  {
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i],LOW);
  }
  connection = 23;
  connection_char = 'x';
}

void loop()
{
  if (Serial.available() > 0)
  {
    char pinnum = Serial.read();
    
    delay(1000);
  
    while (Serial.available() > 0) 
    {
      Serial.read();
    }
    
    choice = pinnum-'a';
    
    //Serial.println(choice);
    
    if (choice >= 0 && choice < sldo_pincount)
    {
      for (int i=0; i<=10; i++)
      {
        digitalWrite(pins[i],LOW);
      }
      delay(1000);
      digitalWrite(pins[8], HIGH);
      delay(500);
      digitalWrite(pins[choice], HIGH);
      connection = choice;
      Serial.println(connection);
    }
    else if (choice == 9) //NTC = char j
    {
      for (int i=0; i<=11; i++)
      {
        digitalWrite(pins[i], LOW);
      } 
      delay(1000);
      digitalWrite(pins[10], HIGH);
      delay(500);
      digitalWrite(pins[9], HIGH);
      connection = choice;
      Serial.println(connection);
    }
    else if (choice == 23)  //all off = char x
      {
        for (int i=0; i<=11; i++)
        {
          digitalWrite(pins[i],LOW);
        }
      connection = choice;
      Serial.println(connection);
      }
    else if (choice == -34) // query, char ?
      
      Serial.println((char)(connection + 'a'));
      
    else Serial.println("Unknown command \n");
  }
    
    
  
}




