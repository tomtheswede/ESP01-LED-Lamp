/*  
 *   For a Desk lamp with an ESP-01 chip and a button.
 *   Code by Thomas Friberg (https://github.com/tomtheswede)
 *   Updated 25/04/2016
 */

// Import ESP8266 libraries
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

//Sensor details
const char* sensorID1 = "LED002"; //Name of sensor
const char* sensorID2 = "BUT002"; //Name of sensor
const char* deviceDescription = "Study Lamp";
const int defaultFade = 15;
const int ledPin = 2; //LED pin number
const int butPin = 0; //Button pin
const int butLoopTime = 100; //millis between loops

// WiFi parameters
const char* ssid = ""; //Enter your WiFi network name here in the quotation marks
const char* password = ""; //Enter your WiFi pasword here in the quotation marks

//Server details
unsigned int localPort = 5007;  //UDP send port
const char* ipAdd = "192.168.0.100"; //Server address
byte packetBuffer[512]; //buffer for incoming packets

//Sensor variables
int ledPinState = 0; //Default boot state of LEDs and last setPoint of the pin between 0 and 100
int ledSetPoint = 0;
int brightness = 100; //last 'on' setpoint for 0-100 scale brightness
static const unsigned int PWMTable[101] = {0,1,2,3,5,6,7,8,9,10,12,13,14,16,18,20,21,24,26,28,31,33,36,39,42,45,49,52,56,60,64,68,72,77,82,87,92,98,103,109,115,121,128,135,142,149,156,164,172,180,188,197,206,215,225,235,245,255,266,276,288,299,311,323,336,348,361,375,388,402,417,432,447,462,478,494,510,527,544,562,580,598,617,636,655,675,696,716,737,759,781,803,826,849,872,896,921,946,971,997,1023}; //0 to 100 values for brightnes
int lastButState = 0;
int butState = 0;
int butPushTime = 0; //millisecond timer for when the button is triggered
int fadeSpeed = defaultFade; //Time between fade intervals - 20ms between change in brightness
String data = "";

WiFiUDP Udp; //Instance to send packets


//--------------------------------------------------------------

void setup()
{
  SetupLines();
}

//--------------------------------------------------------------


void loop()
{
  //Check button state
  CheckButton();

  data=ParseUdpPacket(); //Code for receiving UDP messages

  if (data!="") {
    ProcessLedMessage(data);//Conditionals for switching based on LED signal
  }
  
  FadeLEDs(); //Fading script
}

//--------------------------------------------------------------





void SetupLines() {
  //Set pins and turn off LED
  pinMode(ledPin, OUTPUT); //Set as output
  digitalWrite(ledPin, 0); //Turn off LED while connecting
  pinMode(butPin, INPUT); //For button input
  
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

  //Open the UDP monitoring port
  Udp.begin(localPort);
  Serial.print("Udp server started at port: ");
  Serial.println(localPort);

  //Register on the network with the server after verifying connect
  delay(2000); //Time clearance to ensure registration
  SendUdpValue("REG",sensorID1,String(deviceDescription)); //Register LED on server
  digitalWrite(ledPin, HIGH); //Turn off LED while connecting
  delay(50); //A flash of light to confirm that the lamp is ready to take commands
  digitalWrite(ledPin, LOW); //Turn off LED while connecting
  ledSetPoint=0; // input a setpoint for fading as we enter the loop
}


void CheckButton() {
  if (millis() % butLoopTime == 0) {
    butState=digitalRead(butPin); //0 is depressed, 1 is released
    if ((lastButState==1 && butState==0) && (ledSetPoint != ledPinState)) {
      Serial.println("Fade interupted");
      butPushTime=millis();
      SendUdpValue("LOG",sensorID2,"hold");
    }
    else if (lastButState==1 && butState==0) {
      Serial.println("Button depressed");
      butPushTime=millis();
      SendUdpValue("LOG",sensorID2,"toggle");
    }
    else if (lastButState==0 && butState==1) {
      Serial.println("Button released");
      //SendUdpValue("LOG",sensorID2,String((millis()-butPushTime)/1000)); //Send button press to server
    }
    if (millis()-butPushTime == 5000 && ledPinState == 0 && butState==0) {
      SendUdpValue("LOG",sensorID2,"all off");
      digitalWrite(ledPin,1);
      delay(50);
      digitalWrite(ledPin,0);
      butPushTime=butPushTime-1; //To prevent multiple re-runs of this step in the same millisecond
    }
    if (millis()-butPushTime == 1800 && ledPinState != 0 && butState==0) {
      SendUdpValue("LOG",sensorID2,"100");
      butPushTime=butPushTime-1; //To prevent multiple re-runs of this step in the same millisecond
    }
    lastButState=butState;
  }
}

String ParseUdpPacket() {
  int noBytes = Udp.parsePacket();
  String udpData = "";
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
      udpData = udpData + char(packetBuffer[i - 1]);
    } // end for
    Serial.println("Data reads: " + udpData);
  } // end if
  return udpData;
}

void ProcessLedMessage(String dataIn) {
  String devID = "";
  String message = "";

  devID=dataIn.substring(0,6); //Break down the message in to it's parts
  Serial.println("DevID reads after processing: " + devID);
  message=dataIn.substring(7);
  Serial.println("Message reads after processing: " + message);
  
  if (devID.substring(0,3)=="LED") { //Only do this set of commands if there is a message for an LED device
    
    //Enables slow fading
    if (message.startsWith("fade")) { 
      int messagePos=message.indexOf(" ");
      //Serial.println("Position of space is " + messagePos);
      String fadeVal=message.substring(4,messagePos);
      //Serial.println("Fade value is " + fadeVal);
      fadeSpeed=atoi(fadeVal.c_str());
      //Serial.println("Fade speed set to " + fadeSpeed);
      message=message.substring(messagePos+1); //Cutting 'fade' from the message
      Serial.println("Custom fade increment speed of " + fadeVal + " miliseconds trigged");
      Serial.println("Message trimmed to : " + message);
    }
    else {
      fadeSpeed=defaultFade;
    }

    //Enables instant toggling
    if (((message=="instant toggle")||(message=="instant on")||(message=="instant 100")) && (ledPinState==0)) { //Only turn on if already off
      SendUdpValue("LOG",sensorID1,String(100));
      ledPinState=100;
      ledSetPoint=100;
      digitalWrite(ledPin, HIGH);
      Serial.println("---Instant on triggered");
    }
    else if (((message=="instant toggle")  || (message=="instant off") || (message=="instant 0")) && (ledPinState>0)) { //Only turn off if already on
      SendUdpValue("LOG",sensorID1,String(0));
      ledPinState=0;
      ledSetPoint=0;
      digitalWrite(ledPin, LOW);
      Serial.println("---Instant off triggered");
    }

    //Enables regular dimming
    if (((message=="toggle")||(message=="on")) && (ledPinState==0)) { //Only turn on if already off
      SendUdpValue("LOG",sensorID1,String(brightness));
      ledSetPoint=brightness; // input a setpoint for fading
      Serial.println("---On triggered");
      Serial.println("LED state is now has set point of " + String(ledSetPoint));
    }
    else if (((message=="toggle") || (message=="off") || (message=="0")) && (ledPinState>0)) { //Only turn off if already on
      SendUdpValue("LOG",sensorID1,String(0));
      ledSetPoint=0; // input a setpoint for fading
      Serial.println("---Off triggered");
      Serial.println("LED state is now has set point of " + String(ledSetPoint));
    }
    else if (message=="hold") { //For stopping the fade
      brightness=ledPinState;
      SendUdpValue("LOG",sensorID1,String(brightness));
      ledSetPoint=brightness;
      Serial.println("LED state is now has set point of " + String(ledSetPoint));
    }
    else if ((atoi(message.c_str())>0) && (atoi(message.c_str())<=1023) && (ledPinState!=atoi(message.c_str()))) { //Change brightness
      brightness=atoi(message.c_str());
      SendUdpValue("LOG",sensorID1,String(brightness));
      ledSetPoint=brightness; // input a setpoint for fading
      Serial.print("---PWM trigger: ");
      Serial.println("LED state is now has set point of " + String(ledSetPoint));
    }
  }
}

void FadeLEDs() {
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

void SendUdpValue(String type, String sensorID, String value) {
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
