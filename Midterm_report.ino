//Thermostat and Hygromter using M5stack -CPE term project NYUAD, Spring 2023
#include <M5Core2.h>
#include "DHT.h"
#include <WiFi.h>
#include <cmath>
#include <WiFiMulti.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

WiFiMulti wifiMulti;
int temperature,humidity;
int target_temperature=35;       //set temperature 
int calibrated_temperature=0;
int calibrated_humidity=0;
int temperature_hysterisis=1;   //hysterisis for temperature
int target_humidity=50;       //set humidity
int humidity_hysterisis=10;//hysterisis for humidity 
EMailSender emailSend("esp322376@gmail.com", "nejyqiqnkalzdtas");//email sending ID and Password
unsigned long currentMillis;
int menu = 1;
void executeAction();
void action4(),action3(),action2(),action1(),updateMenu(),mainmenu();
void setting_menu();
void calibration_menu();
int maxtemperature=-100;
int mintemperature=1000;

void setup() {
  wifiMulti.addAP("NETGEAR34", "quickcartoon150"); //Add SSID and Password for WiFi
  wifiMulti.addAP("NETGEAR84", "dailypineapple324");
  wifiMulti.addAP("NETGEAR71", "classyearth413");
  while (wifiMulti.run() != WL_CONNECTED) { //connect to available wifi
    delay(1000);
  }
  M5.begin();
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(100, 80);
  M5.Lcd.print("Loading....");
  delay(2000);
  M5.Lcd.fillScreen(BLUE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 80);
  M5.Lcd.setTextColor(RED);
  M5.Lcd.print("Thermostat and Hygrometer");
  server.begin();
   M5.Lcd.setTextColor(WHITE,BLACK);
  timeClient.begin();
  timeClient.setTimeOffset(14400);
  delay(3000);
}
void loop() {
  while (wifiMulti.run() != WL_CONNECTED) { //reconnect if wifi connection is lost
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
if ((currentMillis-lastsensorread)>1000){   //read temperature and humidity every 1 second
    temperature = random(25,40)+calibrated_temperature; //Random number generator for temperature 
    humidity = random(0,101)+calibrated_humidity;     //Random number generator for humidity
    lastsensorread=millis();
    if (temperature>maxtemperature){ //check for maximum temperature
      maxtemperature=temperature;
    }
    if(temperature<mintemperature){ //check for minimum temperature
      mintemperature=temperature;
    }
    displaying(temperature,humidity);
}
void setTargetTemperature() { //menu for setting Temperature
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
      break;
    }
    delay(50);
  }
}

void setTargetHumidity() { //menu for setting humidity
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
      break;
    
    }
    delay(50);
  }
}
void displaying(int temperature,int humidity){ //temperature displaying function
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
void updateMenu(){ //Menu update after a button is pressed
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
void executeAction() { //execute action when middle button is pressed
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
void setCalibration(){ //Temperature calibration menu
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
void setting_menu(int target,char what){ //Target temperature setting display menu
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
void calibration_menu(int calibrated){ //Calibration setting display menu
    M5.Lcd.setCursor(0, 40);
    M5.Lcd.print("Current Calibration: ");
    M5.Lcd.print(calibrated);
    M5.Lcd.print(" C");
    M5.Lcd.setCursor(30, 60);
       M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(0, 200);
      M5.Lcd.print("    -       Back      +");
}
void detailmenu(){
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
