/*
 * Soil NPK Sensor with Arduino for measuring Nitrogen, Phosphorus, and Potassium
 * Water Level Sensor for calculation of Evapotranspiration
 * Pump controls for Irrigation, Fertigation, Fertilizer application on soil and crops
 */
#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TimeLib.h>

// For the i2c supported Oled display module which is 128x64 
#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
 
#define RE 6
#define DE 5
#define FERTILIZE 11
#define FERTIGATE 12
#define IRRIGATE 13
// The following are the Inquiry frames which are send to the NPK sensor
//for reading the Nitrogen, Phosphorus, and Potassium values
// We defined three arrays with names nitro_inquiry_frame, phos_inquiry_frame, and pota_inquiry_frame 
// Each inquiry frame have 8 values
const byte nitro_inquiry_frame[] = {0x01,0x03, 0x00, 0x1e, 0x00, 0x01, 0xe4, 0x0c};
const byte phos_inquiry_frame[] = {0x01,0x03, 0x00, 0x1f, 0x00, 0x01, 0xb5, 0xcc};
const byte pota_inquiry_frame[] = {0x01,0x03, 0x00, 0x20, 0x00, 0x01, 0x85, 0xc0};
 
byte values[11];
SoftwareSerial modbus(2,3);
SoftwareSerial mySerial(7, 8); //SIM900 Tx & Rx is connected to Arduino #7 & #8
SoftwareSerial wifiSerial(9,10);      // RX, TX for ESP8266
String tme, tme2, date;
char c;
boolean sent;

long t = 0;
float h = 0;
float hw = 0;

int trig = 12;
int echo = 11;

bool DEBUG = true;   //show more logs
int responseTime = 10; //communication timeout
void setup() {
  Serial.begin(9600);
  modbus.begin(9600);
  pinMode (FERTILIZE, OUTPUT);
  pinMode (FERTIGATE, OUTPUT);
   pinMode (IRRIGATE, OUTPUT);
  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);
  pinMode(trig, OUTPUT);
  pinMode(echo, INPUT);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //initialize with the I2C addr 0x3C (128x64)
  delay(500);
  display.clearDisplay();
  display.setCursor(25, 15);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println(" NPK Sensor");
  display.setCursor(25, 35);
  display.setTextSize(1);
  display.print("Initializing");
  display.display();
  delay(2000);
  mySerial.begin(9600);
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect.
  }
  wifiSerial.begin(9600);
  while (!wifiSerial) {
    ; // wait for serial port to connect. 
  }
  sendToWifi("AT+CWMODE=2",responseTime,DEBUG); // configure as access point
  sendToWifi("AT+CIFSR",responseTime,DEBUG); // get ip address
  sendToWifi("AT+CIPMUX=1",responseTime,DEBUG); // configure for multiple connections
  sendToWifi("AT+CIPSERVER=1,80",responseTime,DEBUG); // turn on server on port 80
 
  sendToUno("Wifi connection is running!",responseTime,DEBUG);
  
}
 
void loop() {

if(Serial.available()>0){
     String message = readSerialMessage();
    if(find(message,"debugEsp8266:")){
      String result = sendToWifi(message.substring(13,message.length()),responseTime,DEBUG);
      if(find(result,"OK"))
        sendData("\nOK");
      else
        sendData("\nEr");
    }
}
 if(wifiSerial.available()>0){
    
    String message = readWifiSerialMessage();
    
    if(find(message,"esp8266:")){
       String result = sendToWifi(message.substring(8,message.length()),responseTime,DEBUG);
      if(find(result,"OK"))
        sendData("\n"+result);
      else
        sendData("\nErrRead");               //At command ERROR CODE for Failed Executing statement
    }
   
      else if(find(message,"Irrigate")){
      digitalWrite(11,HIGH);
      delay(5600);
    }
    else if(find(message,"Firtigate")){
      digitalWrite(12,HIGH);
      delay(6000);
    }
    else if(find(message,"Firtilize")){
      digitalWrite(13,HIGH);
      delay(6000);
    }
    else{
      sendData("\nErrRead");                 //Command ERROR CODE for UNABLE TO READ
    }
  }
  delay(responseTime);
  // we will need three variables of the type byte to store the values of 
  // Nitrogen, phosphorus, and Potassium. 
  byte nitrogen_val,phosphorus_val,potassium_val;

  nitrogen_val = nitrogen();
  delay(250);
  phosphorus_val = phosphorous();
  delay(250);
  potassium_val = potassium();
  delay(250);
  
  // The following code is used to send the data to the serial monitor
  
  Serial.print("Nitrogen_Val: ");
  Serial.print(nitrogen_val);
  Serial.println(" mg/kg");
  Serial.print("Phosphorous_Val: ");
  Serial.print(phosphorus_val);
  Serial.println(" mg/kg");
  Serial.print("Potassium_Val: ");
  Serial.print(potassium_val);
  Serial.println(" mg/kg");
  delay(2000);

 // The following code is used to display the data on the Oled display for the NPK
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 5);
  display.print("N: ");
  display.print(nitrogen_val);
  display.setTextSize(1);
  display.print(" mg/kg");
 
  display.setTextSize(2);
  display.setCursor(0, 25);
  display.print("P: ");
  display.print(phosphorus_val);
  display.setTextSize(1);
  display.print(" mg/kg");
 
  display.setTextSize(2);
  display.setCursor(0, 45);
  display.print("K: ");
  display.print(potassium_val);
  display.setTextSize(1);
  display.print(" mg/kg");
 
  display.display();
    // Transmitting pulse
digitalWrite(trig, LOW);
delayMicroseconds(2);
digitalWrite(trig, HIGH);
delayMicroseconds(10);
digitalWrite(trig, LOW);
delayMicroseconds(2);

//For the water level reading
// Waiting for pulse
t = pulseIn(echo, HIGH);

// Calculating distance

h = (t*0.34/2);
hw = 435 - h;

Serial.print(h);
Serial.print(hw);
updateSerial();
checkTime();
}
//For the NPK data
byte nitrogen(){
  digitalWrite(DE,HIGH);
  digitalWrite(RE,HIGH);
  delay(10);
  if(modbus.write(nitro_inquiry_frame,sizeof(nitro_inquiry_frame))==8){
    digitalWrite(DE,LOW);
    digitalWrite(RE,LOW);
    // When we send the inquiry frame to the NPK sensor, then it replies with the response frame
    // now we will read the response frame, and store the values in the values[] arrary, we will be using a for loop.
    for(byte i=0;i<7;i++){
    //Serial.print(modbus.read(),HEX);
    values[i] = modbus.read();
   // Serial.print(values[i],HEX);
    }
    Serial.println();
  }
  return values[4]; // returns the Nigtrogen value only, which is stored at location 4 in the array
}
 
byte phosphorous(){
  digitalWrite(DE,HIGH);
  digitalWrite(RE,HIGH);
  delay(10);
  if(modbus.write(phos_inquiry_frame,sizeof(phos_inquiry_frame))==8){
    digitalWrite(DE,LOW);
    digitalWrite(RE,LOW);
    for(byte i=0;i<7;i++){
    //Serial.print(modbus.read(),HEX);
    values[i] = modbus.read();
   // Serial.print(values[i],HEX);
    }
    Serial.println();
  }
  return values[4];
}
 
byte potassium(){
  digitalWrite(DE,HIGH);
  digitalWrite(RE,HIGH);
  delay(10);
  if(modbus.write(pota_inquiry_frame,sizeof(pota_inquiry_frame))==8){
    digitalWrite(DE,LOW);
    digitalWrite(RE,LOW);
    for(byte i=0;i<7;i++){
    //Serial.print(modbus.read(),HEX);
    values[i] = modbus.read();
    //Serial.print(values[i],HEX);
    }
    Serial.println();
  }
  return values[4];
}

//WIFI Signal and Data sending to operate pump
void sendData(String str){
  String len="";
  len+=str.length();
  sendToWifi("AT+CIPSEND=0,"+len,responseTime,DEBUG);
  delay(100);
  sendToWifi(str,responseTime,DEBUG);
  delay(100);
  sendToWifi("AT+CIPCLOSE=5",responseTime,DEBUG);
}
boolean find(String string, String value){
  if(string.indexOf(value)>=0)
    return true;
  return false;
}
String  readSerialMessage(){
  char value[100]; 
  int index_count =0;
  while(Serial.available()>0){
    value[index_count]=Serial.read();
    index_count++;
    value[index_count] = '\0'; // Null terminate the string
  }
  String str(value);
  str.trim();
  return str;
}
String  readWifiSerialMessage(){
  char value[100]; 
  int index_count =0;
  while(wifiSerial.available()>0){
    value[index_count]=wifiSerial.read();
    index_count++;
    value[index_count] = '\0'; // Null terminate the string
  }
  String str(value);
  str.trim();
  return str;
}
String sendToWifi(String command, const int timeout, boolean debug){
  String response = "";
  wifiSerial.println(command); // send the read character to the esp8266
  long int time = millis();
  while( (time+timeout) > millis())
  {
    while(wifiSerial.available())
    {
    // The esp has data so display its output to the serial window 
    char c = wifiSerial.read(); // read the next character.
    response+=c;
    }  
  }
  if(debug)
  {
    Serial.println(response);
  }
  return response;
}
String sendToUno(String command, const int timeout, boolean debug){
  String response = "";
  Serial.println(command); // send the read character to the esp8266
  long int time = millis();
  while( (time+timeout) > millis())
  {
    while(Serial.available())
    {
      // The esp has data so display its output to the serial window 
      char c = Serial.read(); // read the next character.
      response+=c;
    }  
  }
  if(debug)
  {
    Serial.println(response);
  }
  return response;
}

void updateSerial() {
delay(1000);
while (Serial.available())
{
mySerial.write(Serial.read()); //Forward what Serial received to Software Serial Port

}
while(mySerial.available())
{
char c = mySerial.read();
Serial.write(c);//Forward what Software Serial received to Serial Port
if (c == "\n"){
break;
}
else{
tme += c;
}
}
}
// If you want to send the Water level data on your phone number
void checkTime(){
delay(1000);
Serial.println("Initializing...");
delay(1000);

mySerial.println("AT");
updateSerial();

mySerial.println("AT+CCLK?");
updateSerial();
Serial.println("Result");
Serial.println(tme);
Serial.println("EndResult");
tme2 = tme.substring(39, 44);
date = tme.substring(30, 38);

Serial.println("Result2");
Serial.println(tme2);
Serial.println(date);
Serial.println("EndResult2");

if (tme2 == "08:00")
{
if (sent == false)
{
mySerial.println("AT+CMGF=1"); // Configuring TEXT mode
updateSerial();
mySerial.println("AT+CMGS=\"+639478288707\"");
//change ZZ with country code and xxxxxxxxxxx with phone number to sms
updateSerial();
String hw2 = String(hw);
String message = "ET GAGE NO. 2 \nWATER LEVEL: " + hw2 + " mm \nDate: " +
date + " Time: " + "8:00 AM";
mySerial.print(message);
updateSerial();
mySerial.write(26);
sent = true;
}
}
else if (tme2 == "08:01")
{
sent = false;
}
if (tme2 == "14:00")
{
if (sent == false)
{

mySerial.println("AT+CMGF=1");
// Configuring TEXT mode
updateSerial();
mySerial.println("AT+CMGS=\"+639478288707\"");
//change ZZ with country code and xxxxxxxxxxx with phone number to sms
updateSerial();
String hw2 = String(hw);


String message = "ET GAGE NO. 2 \nWATER LEVEL: " + hw2 + " mm \nDate: " +
date + " Time: " + "2:00 PM" ;
mySerial.print(message);
updateSerial();
mySerial.write(26);
sent = true;
}
}
else if (tme2 == "14:01")
{
sent = false;
}
tme = "";

}
