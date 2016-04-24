/*  
 *   For a Desk lamp with an ESP-01 chip.
 *   Most code by Thomas Friberg
 *   Placeholder change
 */

// Import ESP8266 libraries
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

//Sensor details
const char* sensorID1 = "LED003"; //Name of sensor
const char* sensorID2 = "BUT003"; //Name of sensor
const char* deviceDescription = "Outside light";

// WiFi parameters
const char* ssid = ""; //Enter your WiFi network name here in the quotation marks
const char* password = ""; //Enter your WiFi pasword here in the quotation marks

//Server details
unsigned int localPort = 5007;  //UDP send port
const char* ipAdd = "192.168.0.100"; //Server address
byte packetBuffer[512]; //buffer for incoming packets

//Sensor variables
const int ledPin = 2; //LED pin number
const int butPin = 0; //Button pin
int ledPinState = 0; //Default boot state of LEDs and last setPoint of the pin between 0 and 100
int brightness = 100; //last 'on' setpoint for 0-100 scale brightness
static const unsigned int PWMTable[101] = {0,1,2,3,5,6,7,8,9,10,12,13,14,16,18,20,21,24,26,28,31,33,36,39,42,45,49,52,56,60,64,68,72,77,82,87,92,98,103,109,115,121,128,135,142,149,156,164,172,180,188,197,206,215,225,235,245,255,266,276,288,299,311,323,336,348,361,375,388,402,417,432,447,462,478,494,510,527,544,562,580,598,617,636,655,675,696,716,737,759,781,803,826,849,872,896,921,946,971,997,1023}; //0 to 100 values for brightnes
String data = "";
String message = "";
String devID = "";
const int butLoopTime = 100; //millis between loops
int lastButState = 0;
int butState = 0;
int butPushTime = 0; //millisecond timer for when the button is triggered
int ledSetPoint = 0;
int fadeSpeed = 18; //Time between fade intervals - 20ms between change in brightness
const int defaultFade = 18;
const int slowFade = 6000; //Takes 10 minutes to fade


WiFiUDP Udp; //Instance to send packets

void setup()
{
  //Set pins and turn off LED
  pinMode(ledPin, OUTPUT); //Set as output
  pinMode(butPin, INPUT); //For button input
  analogWrite(ledPin, 0); //Turn off LED while connecting
  
  // Start Serial port monitoring
  Serial.begin(115200);
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected with IP: ");
  // Print the IP address
  Serial.println(WiFi.localIP());

  Udp.begin(localPort);
  Serial.print("Udp server started at port: ");
  Serial.println(localPort);

  //Register on the network with the server after verifying connect
  fadeBrightness(0); //Fade down if the initil pin charge was high
  fadeBrightness(70);
  fadeBrightness(0);
  ledSetPoint=0; // input a setpoint for fading
  delay(2000); //Clearance to ensure registration
  sendUdpValue("REG",sensorID1,String(deviceDescription)); //Register LED on server
}

void loop()
{
  //Check button state
  if (millis() % butLoopTime == 0) {
    butState=digitalRead(butPin); //0 is depressed, 1 is released
    if ((lastButState==1 && butState==0) && (ledSetPoint != ledPinState)) {
      Serial.println("Fade interupted");
      butPushTime=millis();
      sendUdpValue("LOG",sensorID2,"hold");
    }
    else if (lastButState==1 && butState==0) {
      Serial.println("Button depressed");
      butPushTime=millis();
      sendUdpValue("LOG",sensorID2,"toggle");
    }
    else if (lastButState==0 && butState==1) {
      Serial.println("Button released");
      //sendUdpValue("LOG",sensorID2,String((millis()-butPushTime)/1000)); //Send button press to server
    }
    if (millis()-butPushTime == 5000 && ledPinState == 0 && butState==0) {
      sendUdpValue("LOG",sensorID2,"all off");
      fadeBrightness(10);
      fadeBrightness(0);
      butPushTime=butPushTime-1; //To prevent multiple re-runs of this step in the same millisecond
    }
    if (millis()-butPushTime == 1800 && ledPinState != 0 && butState==0) {
      sendUdpValue("LOG",sensorID2,"100");
      butPushTime=butPushTime-1; //To prevent multiple re-runs of this step in the same millisecond
    }
    lastButState=butState;
  }

  //Code for receiving UDP messages
  int noBytes = Udp.parsePacket();
  if ( noBytes ) {
    Serial.print("Packet of ");
    Serial.print(noBytes);
    Serial.print(" characters received from ");
    Serial.print(Udp.remoteIP());
    Serial.print(":");
    Serial.println(Udp.remotePort());
    // We've received a packet, read the data from it
    Udp.read(packetBuffer,noBytes); // read the packet into the buffer

    // display the packet contents in HEX
    for (int i=1;i<=noBytes;i++) {
      data = data + char(packetBuffer[i - 1]);
    } // end for
    Serial.println("Data reads: " + data);
    devID=data.substring(0,6);
    message=data.substring(7);
    data="";
    Serial.println("DevID reads after transform: " + devID);
    Serial.println("Message reads after transform: " + message);
  } // end if

  //Conditionals for switching based on LED signal
  if (devID==sensorID1) {
    devID=""; //Reset so only to trigger once    
    if (message.startsWith("slow ")) { //Enables slow fading
      fadeSpeed=slowFade;
      message=message.substring(5);
    }
    else {
      fadeSpeed=defaultFade;
    }
    if (((message=="instant toggle")||(message=="instant on")||(message=="instant 100")) && (ledPinState==0)) { //Only turn on if already off
      sendUdpValue("LOG",sensorID1,String(100));
      ledPinState=100;
      ledSetPoint=100;
      digitalWrite(ledPin, HIGH);
      Serial.println("---Instant on triggered");
    }
    else if (((message=="instant toggle") || (message=="instant 0") || (message=="instant off")) && (ledPinState>0)) { //Only turn off if already on
      sendUdpValue("LOG",sensorID1,String(0));
      ledPinState=0;
      ledSetPoint=0;
      digitalWrite(ledPin, LOW);
      Serial.println("---Instant off triggered");
    }
    if (((message=="toggle")||(message=="on")) && (ledPinState==0)) { //Only turn on if already off
      sendUdpValue("LOG",sensorID1,String(brightness));
      ledSetPoint=brightness; // input a setpoint for fading
      Serial.println("---On triggered");
      Serial.println("LED state is now has set point of " + String(ledSetPoint));
    }
    else if (((message=="toggle") || (message=="0") || (message=="off")) && (ledPinState>0)) { //Only turn off if already on
      sendUdpValue("LOG",sensorID1,String(0));
      ledSetPoint=0; // input a setpoint for fading
      Serial.println("---Off triggered");
      Serial.println("LED state is now has set point of " + String(ledSetPoint));
    }
    else if (message=="hold") { //For stopping the fade
      brightness=ledPinState;
      sendUdpValue("LOG",sensorID1,String(brightness));
      ledSetPoint=brightness;
      Serial.println("LED state is now has set point of " + String(ledSetPoint));
    }
    else if ((atoi(message.c_str())>0) && (atoi(message.c_str())<=1023) && (ledPinState!=atoi(message.c_str()))) { //Change brightness
      brightness=atoi(message.c_str());
      sendUdpValue("LOG",sensorID1,String(brightness));
      ledSetPoint=brightness; // input a setpoint for fading
      Serial.print("---PWM trigger: ");
      Serial.println("LED state is now has set point of " + String(ledSetPoint));
    }
    message="";
  }

  //Fading script
  if ((millis() % fadeSpeed == 0) && (ledPinState < ledSetPoint)) {
    ledPinState = ledPinState + 1;
    analogWrite(ledPin, PWMTable[ledPinState]);
    Serial.println("LED state is now set to " + String(ledPinState));
    delay(1);
  }
  else if ((millis() % fadeSpeed == 0) && (ledPinState > ledSetPoint)) {
    ledPinState = ledPinState - 1;
    analogWrite(ledPin, PWMTable[ledPinState]);
    Serial.println("LED state is now set to " + String(ledPinState));
    delay(1);
  }
}

void fadeBrightness(int setPoint) { //input integer between 0 and 100
  while (ledPinState < setPoint) {
    ledPinState = ledPinState + 1;
    analogWrite(ledPin, PWMTable[ledPinState]);
    delay(8);
  }
  while (ledPinState > setPoint) {
    ledPinState = ledPinState - 1;
    analogWrite(ledPin, PWMTable[ledPinState]);
    delay(8);
  }
}

void sendUdpValue(String type, String sensorID, String value) {
  //Print GPIO state in serial
  Serial.print("Value sent via UDP: ");
  Serial.println(type + "," + sensorID + "," + value);

  // send a message, to the IP address and port
  Udp.beginPacket(ipAdd,localPort);
  Udp.print(type);
  Udp.write(",");
  Udp.print(sensorID);
  Udp.write(",");
  Udp.print(value); //This is the value to be sent
  Udp.endPacket();
}

void returnSignalToSender() {  
  Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
  Udp.write("Answer from ");
  Udp.print(sensorID1);
  Udp.write(" with IP of ");
  Udp.print(WiFi.localIP());
  Udp.write(": The LED state is now set to ");
  Udp.println(ledPinState);
  Udp.endPacket();
}

