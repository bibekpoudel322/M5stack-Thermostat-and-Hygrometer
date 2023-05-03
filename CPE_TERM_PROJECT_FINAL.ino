/* Thermostat and Hygrometer */
//CPE Term Project-Spring 2023
#include <M5Core2.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <EMailSender.h>
#include <ThingSpeak.h>
#include <cmath>
#include <WiFiMulti.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"
Adafruit_SHT31 sht31 = Adafruit_SHT31();
#include <M5_4Relay.h>
#define EEPROM_SIZE 8

M5_4Relay Relay4;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

WiFiMulti wifiMulti;

#define API_KEY "DTTVTAX3PU4MG8Y4"
WiFiClient client;
WiFiServer server(80);
const char* ssid = "NETGEAR34";//SSID for wifi connection
const char* password = "quickcartoon150";//password for wifi connection
int temperature,humidity;
const int heating_relay=0;  //I2C address correspondance of relay1 for M5stack
const int cooling_relay=1;  //I2C address correspondance of relay2 for M5stack
const int humidifying_relay=2;  //I2C address correspondance of relay3 for M5stack
const int dehumidifying_relay=3;  //I2C address correspondance of relay4 for M5stack
int calibrated_temperature=0;
int calibrated_humidity=0;
int target_temperature=35;
int temperature_hysterisis=1;   //hysterisis for temperature
int target_humidity=50;       //set humidity
int humidity_hysterisis=10;//hysterisis for humidity 
EMailSender emailSend("esp322376@gmail.com", "nejyqiqnkalzdtas");
unsigned long lastemailsent=0,lastsensorread=0;
unsigned long currentMillis;
int menu = 1;
void beep();
void vibrate();
void send_email();
void executeAction();
void action4(),action3(),action2(),action1(),updateMenu(),mainmenu();
void setting_menu();
void calibration_menu();
bool email_sent=false;
bool optimal_temperaturereached=false;
bool optimal_humidityreached=false;
int lastoptimalreached=0;
int maxtemperature=-100;
int mintemperature=1000;
bool heatingstatus,coolingstatus,humidifyingstatus,dehumidifyingstatus;
int failsafecounter=0;
bool failure=false;
/*
int secondaryheaterpin=
int secondaryhumidifierpin=
int secondarycoolerpin=
int secondarydehumidifierpin=
*/
bool getfailurestatus(){
  return failure;
}
void setup() {
  EEPROM.begin(EEPROM_SIZE);
  target_temperature=EEPROM.read(0);
  target_humidity=EEPROM.read(1);
  wifiMulti.addAP("NETGEAR34", "quickcartoon150");
  wifiMulti.addAP("NETGEAR84", "dailypineapple324");
  wifiMulti.addAP("NETGEAR71", "classyearth413");
  WiFi.begin(ssid, password);
   while (wifiMulti.run() != WL_CONNECTED) {
    delay(1000);
  }
  M5.begin();
  Wire.begin();
  sht31.begin(0x44);
  Relay4.begin(Wire1,21,22);
  Relay4.AllOff();
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(100, 80);
  M5.Lcd.print("Loading....");
  delay(2000);
  M5.Lcd.fillScreen(BLUE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 80);
  M5.Lcd.setTextColor(RED);
  M5.Lcd.print("Thermostat and Hygrometer");
  ThingSpeak.begin(client);
  server.begin();
   M5.Lcd.setTextColor(WHITE,BLACK);
  timeClient.begin();
  timeClient.setTimeOffset(14400);
  delay(5000);
}

void loop() {
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(1000);
  }
  M5.update();
  if (M5.BtnB.wasReleased()){
    updateMenu();
    M5.update();
  while(true){
    M5.update();
  if(M5.BtnA.wasReleased()){
    menu++;
    updateMenu();
  }  
  if(M5.BtnC.wasReleased()){
    menu--;
    updateMenu();
  }  
  if(M5.BtnB.wasReleased()){
    if (menu==5){
      menu=0;
      break;
    }
    executeAction();
    updateMenu();
  }  
  }
    }
  currentMillis=millis();
  if((currentMillis-lastemailsent)>600000 && (currentMillis-lastoptimalreached)>600000){//send email if the optimal temperature or optimal humidity is not reached even after 10 minutes of system running  
    if ((optimal_temperaturereached==false || optimal_humidityreached==false)){
      send_email();
      beep();
      failsafecounter++;
      if(failsafecounter>=3){
      failure=true;                
      }
      else{
        failure=false;
      }      
      lastemailsent=millis();
    }
  }
  if ((currentMillis-lastsensorread)>1000){  //set delay of 1sec
    temperature = round(sht31.readTemperature())+calibrated_temperature; //Read temperature from sensor
    humidity = round(sht31.readHumidity())+calibrated_humidity;     //Read humidity from sensor 
    lastsensorread=millis();
    if (temperature>maxtemperature){
      maxtemperature=temperature;
    }
    if(temperature<mintemperature){
      mintemperature=temperature;
    }     
    displaying(temperature,humidity);//display recent temperature and humidity
    ThingSpeak.setField(1, temperature); //Field 1 corresponding to Temperature value
    ThingSpeak.setField(2, humidity); //Field 2 corresponding to Humidity Value
    int x=ThingSpeak.writeFields(1,API_KEY);
  }
  //main control statements
  if (temperature < target_temperature - temperature_hysterisis && heatingstatus==false) {
    heating_on();
    optimal_temperaturereached==false;
    /*
    if(getfailurestatus==true){
      secondaryheateron();
          }
     */
    } else if (temperature >= target_temperature && heatingstatus==true) {
    heating_off();
    optimal_temperaturereached=true;
    lastoptimalreached=millis();
    failsafecounter=0;
    /*
    if(digitalRead(secondaryheaterpin)==HIGH){
        secondaryheateroff();      
    }
     */
  }
  else if (temperature>target_temperature + temperature_hysterisis && heatingstatus==false)
  {
    cooling_on();
    optimal_temperaturereached=false;
  }
  else if (temperature<=target_temperature && coolingstatus==true)
  {
    cooling_off();
    lastoptimalreached=millis();
  }
  if (humidity < target_humidity - humidity_hysterisis && humidifyingstatus==false) {
    humidifying_on();
  } 
  else if (humidity >= target_humidity && humidifyingstatus==true) {
    humidifying_off();
  }
    else if (humidity > target_humidity+humidity_hysterisis && humidifyingstatus==false) {
    dehumidifying_on();
  }
    else if (humidity <= target_humidity && dehumidifyingstatus==true) {
    dehumidifying_off();
  }
}
//relay functions 
void heating_off(){
  Relay4.Write4Relay(heating_relay,false);
  heatingstatus=false;
}
void cooling_off(){
  Relay4.Write4Relay(cooling_relay,false);
  coolingstatus=false;
}
void humidifying_off(){
  Relay4.Write4Relay(humidifying_relay,false);
  humidifyingstatus=false;
}
void dehumidifying_off(){
  Relay4.Write4Relay(dehumidifying_relay,false);
  dehumidifyingstatus=false;
}
void heating_on(){
  Relay4.Write4Relay(heating_relay,true);
  heatingstatus=true;
}
void cooling_on(){
  Relay4.Write4Relay(cooling_relay,true);
  coolingstatus=true;
}
void humidifying_on(){
  Relay4.Write4Relay(humidifying_relay,true);
  humidifyingstatus=true;
}
void dehumidifying_on(){
  Relay4.Write4Relay(dehumidifying_relay,true);
  dehumidifyingstatus=true;
}
void setTargetTemperature() {//function to allow user set temperature
  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.print("Enter target temperature:");
  setting_menu(target_temperature,'C');

  while (true) {
    M5.update();
    if (M5.BtnA.wasReleased()) {
      target_temperature -= 1;
      M5.update();
      M5.Lcd.clear();
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.print("Enter target temperature:");
      setting_menu(target_temperature,'C');
    }
    if (M5.BtnC.wasReleased()) {
      target_temperature += 1;
      M5.update();
      M5.Lcd.clear();
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.print("Enter target temperature:");
      setting_menu(target_temperature,'C');
    }
    if (M5.BtnB.wasReleased()) {
      M5.lcd.clear();
      M5.update();
      EEPROM.write(0,target_temperature);
      EEPROM.commit();

      break;
    }
    delay(50);
  }
}

void setTargetHumidity() {//function to allow user set humidity
  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.print("Enter target humidity:");
  setting_menu(target_humidity,'%');
  while (true) {
    M5.update();
    if (M5.BtnA.wasReleased()) {
      target_humidity -= 5;
      if (target_humidity<=0){
        target_humidity=0;
      }
      M5.Lcd.clear();
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.print("Enter target humidity:");
      setting_menu(target_humidity,'%');
    }
    if (M5.BtnC.wasReleased()) {
      target_humidity += 5;
      if (target_humidity>=100){
        target_humidity=100;
      }
      M5.Lcd.clear();
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.print("Enter target humidity:");
      setting_menu(target_humidity,'%');
      
    }
    if (M5.BtnB.wasReleased()) {
            M5.lcd.clear();
      M5.update();
      EEPROM.write(1,target_humidity);
      EEPROM.commit();      
      break;
    
    }
    delay(50);
  }
}
void beep(){//Alarm 
  //produce sound notification
  for (int i = 0; i < 3; ++i){
    M5.Spk.DingDong();//produce system sound 3 times continuously
  }
}
void send_email(){//Email Sending Function
 
  EMailSender::EMailMessage message;
  //email contents
  message.subject = "PROBLEM in HVAC!";
  message.message = "Dear User,\n There is a problem in your Heating or Cooling system please check the system as soon as possible!!!!";
  
  //sending email to receiver email
  EMailSender::Response resp = emailSend.send("bkpoudel44@gmail.com", message);
  delay(4000);//delay 4s.
}
void displaying(int temperature,int humidity){//Function to display current temperature and humidity in screen
  M5.Lcd.clear();
  M5.Lcd.setCursor(0, 20);
  M5.Lcd.setTextSize(3);
  M5.Lcd.print("Temperature: ");
  M5.Lcd.print(temperature);
  M5.Lcd.print(" C\n\n\n");
  M5.Lcd.setTextSize(3);
  M5.Lcd.print("Humidity   : ");
  M5.Lcd.print(humidity);
  M5.Lcd.print(" %\n\n\n");
  M5.Lcd.setTextSize(3);
  M5.Lcd.print("     ");
  displaytime();
  M5.Lcd.setTextSize(2);
  M5.Lcd.print("\n\n");
  M5.Lcd.print("           Menu");

}
void updateMenu(){ //function to update the menu when up/down button is pressed.
  M5.Lcd.setTextSize(3);
  switch (menu) {
    case 0:
      menu = 1;
      break;
    case 1:
        M5.Lcd.clear();
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.print(">Set Temperature \n");
        M5.Lcd.print(" Set Humidity \n");
        M5.Lcd.print(" Calibration \n");
        M5.Lcd.print(" Details \n");
        M5.Lcd.print(" Exit Settings \n");
        M5.Lcd.setTextSize(2);
        M5.Lcd.setCursor(0, 200);
       M5.Lcd.print("    v     Select      ^");
      break;
    case 2:
        M5.Lcd.clear();
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.print(" Set Temperature \n");
        M5.Lcd.print(">Set Humidity \n");
        M5.Lcd.print(" Calibration \n");
        M5.Lcd.print(" Details \n");
        M5.Lcd.print(" Exit Settings \n");
        M5.Lcd.setTextSize(2);
        M5.Lcd.setCursor(0, 200);
        M5.Lcd.print("    v     Select      ^");
      break;
    case 3:
        M5.Lcd.clear();
        M5.Lcd.setCursor(0, 0);
        M5.Lcd.print(" Set Temperature \n");
        M5.Lcd.print(" Set Humidity \n");
        M5.Lcd.print(">Calibration \n");
        M5.Lcd.print(" Details \n");
        M5.Lcd.print(" Exit Settings \n");
        M5.Lcd.setTextSize(2);
        M5.Lcd.setCursor(0, 200);
        M5.Lcd.print("    v     Select      ^");
      break;
    case 4:
      M5.Lcd.clear();
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.print(" Set Temperature \n");
      M5.Lcd.print(" Set Humidity \n");
      M5.Lcd.print(" Calibration \n");
      M5.Lcd.print(">Details \n");
      M5.Lcd.print(" Exit Settings \n");
      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(0, 200);
      M5.Lcd.print("    v     Select      ^");
      break;
    case 5:
      M5.Lcd.clear();
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.print(" Set Temperature \n");
      M5.Lcd.print(" Set Humidity \n");
      M5.Lcd.print(" Calibration \n");
      M5.Lcd.print(" Details \n");
      M5.Lcd.print(">Exit Settings \n");
      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(0, 200);
      M5.Lcd.print("    v     Select      ^");
      break;
    case 6:
      menu=5;    
      }
}
void executeAction() {//function to execute the command when middle button is pressed
  switch (menu) {
    case 1:
      action1();
      break;
    case 2:
      action2();
      break;
    case 3:
      action3();
      break;
    case 4:
      action4();
      break;
  }
}
void setCalibration(){//function menu for calibration of the sensor 
M5.Lcd.clear();
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.print("Temperature Calibration:");
  calibration_menu(calibrated_temperature);
  while (true) {
    M5.update();
    if (M5.BtnA.wasReleased()) {
      calibrated_temperature -= 1;
      M5.Lcd.clear();
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.print("Temperature Calibration:");
      M5.Lcd.setCursor(0, 40);
      calibration_menu(calibrated_temperature);
    }
    if (M5.BtnC.wasReleased()) {
      calibrated_temperature += 1;
      M5.Lcd.clear();
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.print("Temperature Calibration:");
      calibration_menu(calibrated_temperature);
    }
    if (M5.BtnB.wasReleased()) {
            M5.lcd.clear();
      M5.update();
      break;
    
    }
    delay(50);
  }
}
//execute ations accordingly
void action1(){
setTargetTemperature();
}
void action2(){
setTargetHumidity();
}
void action3(){

setCalibration();
}
void action4(){
  detailmenu();
}
void setting_menu(int target,char what){//Menu for setting target parameter
      M5.Lcd.setCursor(0, 40);
      M5.Lcd.print("Current target: ");
      M5.Lcd.print(target);
      M5.Lcd.print(" ");
      M5.Lcd.print(what);
      M5.Lcd.setCursor(30, 60);
       M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(0, 200);
      M5.Lcd.print("    -       Back      +");
}
void calibration_menu(int calibrated){//General Calibration function
    M5.Lcd.setCursor(0, 40);
    M5.Lcd.print("Current Calibration: ");
    M5.Lcd.print(calibrated);
    M5.Lcd.print(" C");
    M5.Lcd.setCursor(30, 60);
       M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(0, 200);
      M5.Lcd.print("    -       Back      +");
}
void displaytime(){//function to display time
  timeClient.update();
  String formattedTime = timeClient.getFormattedTime();
  M5.Lcd.println(formattedTime);
}
void detailmenu(){//function to display details menu
  M5.Lcd.clear();
  while (true) {
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextSize(2);
  M5.Lcd.print("Target Temperature:");
  M5.Lcd.print(target_temperature);
  M5.Lcd.print(" C\n\n\n");
  M5.Lcd.print("Target Humidity   :");
  M5.Lcd.print(target_humidity);
  M5.Lcd.print(" %\n\n\n");
  M5.Lcd.print("Max Temperature   :");
  M5.Lcd.print(maxtemperature);
  M5.Lcd.print(" C\n\n\n");  
  M5.Lcd.print("Min Temperature   :");
  M5.Lcd.print(mintemperature);
  M5.Lcd.print(" C\n");
  M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(0, 200);
  M5.Lcd.print("           Back       ");
  M5.update();
    if (M5.BtnB.wasReleased()) {
      M5.lcd.clear();
      M5.update();
      break;
    }
    delay(50);
  }
 
}
//Secondary heater function
 /*void secondaryheateron(){
  digitalWrite(secondaryheaterpin,HIGH);
}
 void secondaryheateroff(){
  digitalWrite(secondaryheaterpin,LOW);
}
 void secondarycooleron(){
  digitalWrite(secondarycoolerpin,HIGH);
}
 void secondarycooleroff(){
  digitalWrite(secondarycoolerpin,LOW);
}
 void secondaryhumidifieron(){
  digitalWrite(secondaryhumidifierpin,HIGH);
}
 void secondarydehumidifieron(){
  digitalWrite(secondarydehumidifierpin,HIGH);
}
 void secondarydehumidifieroff(){
  digitalWrite(secondarydehumidifierpin,LOW);
}
 void secondaryhumidifieroff(){
  digitalWrite(secondaryhumidifierpin,LOW);
}*/

