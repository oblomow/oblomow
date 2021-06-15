#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include <OneWire.h>


// Define starting and ending chars for recognizing commands
#define SOP '{'
#define EOP '}'
// maximum command length
#define MAX_COMMAND_LENGTH 33

// Buffer for receiving commands
#define BUFFER_SIZE 128
char buffer[BUFFER_SIZE];
// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// Initialize the Ethernet client library
// with the IP address and port of the server 
// that you want to connect to (port 80 is default for HTTP):
EthernetServer server(80);

// Setup OneWire System
OneWire  ds(9);  // on pin 9
#define MAX_DS1820_SENSORS 5 //maximum for 5 sensors
byte addr[MAX_DS1820_SENSORS][8];
int foundSensors = 0;
byte type_s;

void setup() {
  byte addrs[8];
  byte i;
  
  // start the serial library for debugging
  pinMode(5, OUTPUT); // LED Power
  pinMode(6, OUTPUT); // LED Sensors
  pinMode(7, OUTPUT); // LED Got IP
  pinMode(8, OUTPUT); // LED System running (on) and access (blink)
  testLEDs();
  digitalWrite(5, HIGH); // Power is on
  Serial.begin(9600);
  Serial.println("Initializing");


 if ( !ds.search(addrs)) {
    Serial.println("No more addresses.");
    Serial.println();
    ds.reset_search();
    delay(250);
    return;
  }

  Serial.print("ROM =");

  for( i = 0; i < 8; i++) {
    Serial.write(' ');
    Serial.print(addrs[i], HEX);
  }

  if (OneWire::crc8(addrs, 7) != addrs[7]) {
      Serial.println("CRC is not valid!");
      return;
  }
  Serial.println();

  // the first ROM byte indicates which chip

  switch (addrs[0]) {
    case 0x10:
      Serial.println("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
      return;
  }
  
  // Search for Sensors...
  ds.reset_search();
  Serial.println("Searching for Sensors...");
  for (int sensor = 0; sensor < MAX_DS1820_SENSORS; sensor++) {
    if (!ds.search(addr[sensor]))
    {
      ds.reset_search();
      delay(250);
      break;
    }
    Serial.print("- Sensor ");
    Serial.print(sensor+1);
    Serial.print(": ");
    for(int j = 0; j<8;j++){
      Serial.print("0x");
      if (addr[sensor][j] < 16) {
        Serial.print('0');
      }
      Serial.print(addr[sensor][j], HEX);
      if (j < 7) {
        Serial.print(", ");
      }
    }
    Serial.println();
    foundSensors++;
  }
  if (foundSensors == 0) {
    Serial.println("No sensors found, stopping!");
    // no point in carrying on, so do nothing forevermore:
    for(;;) {
      digitalWrite(6, HIGH);
      delay(200);

      digitalWrite(6, LOW);
      delay(200);
    }
  } 
  else {
    for(int i = 0; i < foundSensors; i++) {
      digitalWrite(6, HIGH);
      delay(750);

      digitalWrite(6, LOW);
      delay(750);
    } 
    delay(1000);
  }
  digitalWrite(6, HIGH);

  Serial.println("Getting IP...");

  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    for(;;) {
      digitalWrite(7, HIGH);
      delay(200);

      digitalWrite(7, LOW);
      delay(200);
    }
  }

  // print your local IP address:
  Serial.print("- My IP address: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    if(thisByte != 3) { 
      Serial.print("."); 
    }
  }
  Serial.println();
  digitalWrite(7, HIGH);

  Serial.println("Starting server...");
  server.begin();

  Serial.println("Naguino Temp is ready!");  
  Serial.println("Waiting for connections");
  Serial.println();
  digitalWrite(8, HIGH);
}

void loop() {
  String sLastCommand;
  String replyString = "";
  char t[10];
  int tempIndex = -1;
  byte present = 0;
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    
    // blink shortly
    digitalWrite(8, LOW);
    delay(200);
    digitalWrite(8, HIGH);

    boolean started = false;
    boolean ended = false;
    float celsius;

    char inData[MAX_COMMAND_LENGTH]; 
    byte index = 0;
    
    // As long as someone sends us something
    while(client.connected() && client.available())
    {
      char inChar = client.read();
      if(inChar == SOP) // if its the starting operator
      {
        started = true;
        index = 0;
        inData[index] = '\0';
      }
      else if(inChar == EOP) // if its the ending operator
      {
        ended = true;
        break;
      }
      else
      {
        if(index < MAX_COMMAND_LENGTH-1) // Array size
        {
          inData[index++] = inChar;
          inData[index] = '\0';
        }
      }
    }
    if(started && ended)
    {
      sLastCommand = String(inData);
      Serial.print("Got command: ");
      Serial.println(sLastCommand);

      // List Sensors
      if (sLastCommand == "listSensors;") {
        Serial.println("Should list sensors...");
        for (int i = 0; i < foundSensors; i++) {
          Serial.print("Sensor ");
          Serial.print(i);
          Serial.print(": ");
          for(int j = 0; j<8;j++){
            Serial.print("0x");
            if (addr[i][j] < 16) {
              Serial.print('0');
              client.print('0');
            }
            Serial.print(addr[i][j], HEX);
            client.print(addr[i][j], HEX);
            if (j < 7) {
              Serial.print(",");
              client.print(",");
            }
          }
          Serial.println();
          if(i <= (foundSensors - 1)) {
            client.println(";");
          }
        }
      } 
      else if (sLastCommand.substring(0,8) == "getTemp:") {
        Serial.print("Should get Temperature for Sensor 0 ");
        byte readAddr[] = { 
          addr[0][0], addr[0][1], addr[0][2], addr[0][3], addr[0][4], addr[0][5], addr[0][6], addr[0][7], addr[0][7]    };

        for(int i = 0; i<8; i++) {
          Serial.print("0x");
          if (readAddr[i] < 16) {
            Serial.print('0');
          }
          Serial.print(readAddr[i], HEX);
          if (i < 7) {
            Serial.print(", ");
          }

        }
        Serial.println();

        ds.reset(); // reset the bus
        ds.select(readAddr); // select the sensor
        ds.write(0x44,0);  // ask for temperature (0 = non-parasite mode, 1 = parasite mode)
        delay(1000); // let the sensor select the data
        ds.reset(); // reset the bus
        ds.select(readAddr);  // select the sensor
        ds.write(0xBE); // read Scratchpad
        int HighByte, LowByte, TReading, SignBit, Tc_100, Whole, Fract;
        char buf[20];
        byte data[12];
        Serial.print("  Data = ");
        Serial.print(present, HEX);
        Serial.print(" ");
        for (int i = 0; i < 9; i++) // we need 9 bytes
        { 
          data[i] = ds.read();
          Serial.print(data[i], HEX);
          Serial.print(" ");
        }
        Serial.print(" CRC=");
        Serial.print(OneWire::crc8(data, 8), HEX);
        Serial.println();

 // Convert the data to actual temperature
 // because the result is a 16 bit signed integer, it should
 // be stored to an "int16_t" type, which is always 16 bits
 // even when compiled on a 32 bit processor.

  int16_t raw = (data[1] << 8) | data[0];

  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {

    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them

    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 m

    //// default is 12 bit resolution, 750 ms conversion time
  }

  celsius = (float)raw / 16.0;


  Serial.print("  Temperature new = ");
  Serial.print(celsius);
  Serial.print(" Celsius, ");

  dtostrf( celsius,6,2,t);
        sprintf(buf, "%s",t);

        Serial.print("Temperature is: ");
        Serial.println(buf);
        client.println(buf);
      } 
      else { 
        Serial.println("Unknown command :(");
        client.println("sorry, didn't understand ya :(");
      }
      Serial.println("Closing connection");

    }
    client.stop();
  }
}

void testLEDs() {

  digitalWrite(5, HIGH);
  delay(100);
  digitalWrite(6, HIGH);
  delay(100);
  digitalWrite(7, HIGH);
  delay(100);
  digitalWrite(8, HIGH);
  delay(200);
  digitalWrite(5, LOW);
  delay(100);
  digitalWrite(6, LOW);
  delay(100);
  digitalWrite(7, LOW);
  delay(100);
  digitalWrite(8, LOW);
  delay(750); 
}

