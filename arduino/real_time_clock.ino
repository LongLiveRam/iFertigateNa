#include <SoftwareSerial.h>
#include <TimeLib.h>
//Create software serial object to communicate with SIM900
SoftwareSerial mySerial(7, 8); //SIM900 Tx & Rx is connected to Arduino #7 & #8

String tme, tme2, date;
char c;
boolean sent;
long t = 0, h = 0, hw = 0;

void setup() {
Serial.begin(9600);
mySerial.begin(9600);
mySerial.write("AT+CMGF=1\r");
delay(1500);

mySerial.write("AT+CCLK=\"20/04/22,16:28:00+08\"\r"); // Set time and date here
while(1)
{
if(Serial.available())
{
Serial.write(Serial.read());
}
}
}
void loop() {
}
void updateSerial(){
delay(1000);
while (Serial.available())
{
mySerial.write(Serial.read());//Forward what Serial received to Software Serial Port
}
while(mySerial.available())
{
char c = mySerial.read();
Serial.write(c);//Forward what Software Serial received to Serial Port
if (c == '\n')
{
break;
}
else{
tme += c;
}
}
}
void checkTime(){
delay(1000);
Serial.println("Initializing...");
delay(1000);
mySerial.println("AT"); //Handshaking with SIM900
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
