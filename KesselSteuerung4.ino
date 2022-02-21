/*

 
No dispplay:
Node MCU 1.0 (ESP 12E Module)

Wiring for this Code:
LCD:  NODEMCU:
GND   GND
VCC   VIN (5V)
SDA   D2
SCL   D1

18DS20:      NODEMCU:
YELLOW (SIG) D4
RED (VCC)    3,3V
BLACK (GND)  GND
(47kOhm Resitor between YELLOW and RED)

Relais:   NODEMCU:
ISOVCC    VIN
VCC       3.3V
GND       GND
IN1       D5
IN2       D6
 
 */
  // to read openweathermap webservice
//#include <ArduinoHttpClient.h>  // to read openweathermap webservice
#include "heltec.h"
#include <Wire.h>  // This library is already built in to the Arduino IDE

#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 0 //D3
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
#include <NTPClient.h>
#include <ESP8266WiFiMulti.h> // to read openweathermap webservice
#include <ESP8266WiFi.h>


#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <Time.h>
// NTP Init
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
#include <ESP8266HTTPClient.h>
//WLAN Access
const char *ssid = "xxx";  // You WIFI DATA
const char *password = "xxx";  // You WIFI DATA

float low_t[12]   = {-3.0,-2.0,0.0, 3.0, 7.0,10.0,12.0,12.0, 9.0, 5.0,1.0,-1.0};
float  high_t[12] = {2.0,  3.0,7.0,11.0,16.0,20.0,20.0,20.0,18.0,13.0,7.0, 3.0};


// General error text
String errorMessage="";

String IP = "";
String result;



float Temperature;


// WEb Sever Init
ESP8266WebServer server ( 80 );
// Global Variables
float waterTemp=99.9; //Temperatur des Brauchwassers gemessen
float minWaterTemp=99.9; //Temperatur des Brauchwassers gemessen
float maxWaterTemp=0.0; //Temperatur des Brauchwassers gemessen
float kessTemp=99.9; // Temperatur des Kesselwassers gemessen
float minKessTemp=99.9; // Temperatur des Kesselwassers gemessen
float maxKessTemp=0.0; // Temperatur des Kesselwassers gemessen
// Default Values
int beginDay = 9; // Start der Tagssteuerung
int endDay = 10;  // Ende der zeit für Warmwasser morgens 
int beginNight=17; // Start für Warwasser abends
int endNight=19; // Start der Nachtsteuerung  
float temperature=50.0;  // requested Watertemperature
int intervallDay=30;  // Minutes per hour the heating wil w´ork daytime
int intervallNight=30; // Minutes per hour the heating wil w´ork nighttime
int Minutes=0; //we count minutes to onyl read the openweathermap twice per hour, there is a max. count I wat to avoid
int OldMinutes=0;
int month;
int bias=0; //Anpassen der automatischen Heizfunkion 
boolean SteuerungAut = true;
boolean Wrelais = true; // warmwasserrelais
boolean Hrelais = true; // heizpumpenrelais
boolean Day[7]={true,true,true,true,true,true,true}; //0 0 sunday - workdays for warm water
  // init GPIO pins
//const int led = 13; //D7

// GPIO Belegung aus https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/blob/master/PinoutDiagram/WIFI%20Kit%208.pdf
#define Relais 12  // D6 
#define Relais2 13 // D7

/*Aproximation of  average temperature by month for Rothenbuch*/

float avgtemp (int hour, int day, int month)
{
  // Little sophisticated formuar to calculate the average time ta that date and time :))
  int pre = month==1 ? 12 : month -1;
  int succ = month==12 ? 0 : month +1;
  float min_t, max_t;
  
  if (day < 16)
  {
     min_t = (low_t[month] - low_t[pre])/30*(day+15)+low_t[pre];
     max_t = (high_t[month] - high_t[pre])/30*(day+15)+ high_t[pre];
  }
  else
  {
    min_t = (low_t[succ]-  low_t[month] )/30*(day-15)+low_t[month];
    max_t = (high_t[succ]-  high_t[month])/30*(day-15)+ high_t[month];
  }
  float avg_t=(max_t+min_t)/2.0;
  return sin((hour+12)*3.141/12)*(max_t - min_t)+ avg_t;

}



void getWeatherData(boolean force) //client function to send/receive GET request data.
{
if ((Minutes != OldMinutes && (Minutes == 1 || Minutes == 31)) || force) // once every 30 Minutes read OW = 48 reads per day => save
{  
  WiFiClient client;
  HTTPClient http;
  OldMinutes = Minutes;
  if(http.begin(client, "http://api.openweathermap.org/data/2.5/weather?q=Rothenbuch&APPID=e917f44259a1a562dfcd10a7848e8db5"))
  {
      //HTTPclient.get("/data/2.5/weather?q=Rothenbuch&APPID=e917f44259a1a562dfcd10a7848e8db5");
 
      int httpCode = http.GET();
      if (httpCode > 0)
      {
          result = http.getString();
    
          int sI=result.indexOf("temp");
          if (sI==-1) 
            { 
              int h;
             time_t rawtime = timeClient.getEpochTime();
             struct tm * ti;
             ti = localtime (&rawtime);
        
              month = ti->tm_mon;
              h=ti->tm_hour;
              
              Temperature= avgtemp(ti->tm_hour,ti->tm_mday,ti->tm_mon);
        
              errorMessage="Temp not found on weather server";}
          else
            Temperature = atof(result.substring(sI+6,sI+12).c_str()) -273.15; //Convert Kelvin to Celsius
        
          //Serial.println(Minutes);
          //Serial.println(Temperature);
     
      }
   }
 }
}

// Read and interpret Arguments of Webserver
void analyzeArgs(){
  if (server.args() > 1) { for (uint8_t j = 0; j < 7;j++) Day[j] = false;SteuerungAut=false;}
  for ( uint8_t index = 0; index < server.args(); index++ ) {
    String s = server.arg(index);

    // Switch on paramter name and process the paramter value
    switch(server.argName ( index )[0]+0) {  
      case 'b': beginDay=atoi( s.c_str() );break; // Begin Morning
      case 'e': endDay=atoi( s.c_str() ); break; // End Morning
      case 'B': beginNight=atoi( s.c_str() );break;// Begin Evening
      case 'E': endNight=atoi( s.c_str() );break;// End Evening
      case 't': temperature=atof( s.c_str() );break; // Desired Temperature
      case 'a': SteuerungAut=true;break; // Switch Controller Function on off
      case 'i': intervallDay=atoi( s.c_str() );break;// Intervall tagsueber    
      case 'I': intervallNight=atoi( s.c_str() );break;// Intervall nachts   
      case 'K': bias=   atoi( s.c_str() );break;// Bias as Korrekturfaktor
      case '0' : Day[0] = true;break;
      case '1' : Day[1] = true;break;
      case '2' : Day[2] = true;break;
      case '3' : Day[3] = true;break;    
      case '4' : Day[4] = true;break;
      case '5' : Day[5] = true;break;
      case '6' : Day[6] = true;break;  
      default: ;
    }
  }
  if (intervallDay<5) intervallDay= 0; if (intervallNight>55) intervallNight = 60; //Avoid to short switch times
  if (intervallDay>55) intervallDay= 60; if (intervallNight<5) intervallNight = 0; //Avoid to short switch times
}
void handleRoot() {

 char temp[2300];
 int sec = millis() / 1000;
 int min = sec / 60;
 int hr = min / 60;
 snprintf ( temp, 2300,
"<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>Kesselsteuerung</title>\
    <style>\
      body { background-color: #000000; font-family: Arial, Helvetica, Sans-Serif; Color: #FFC125; }\
    </style>\
  </head>\
  <body>\
    <table><tr><td><h1>Warmwasser und<br>Heizungssteuerung</h1>\
    <table>\
    <tr><td>Uhrzeit:</td><td>%s</td></tr>\
    <tr><td>Wassertemperatur:</td><td>%2.1f</td></tr>\
    <tr><td>Kesseltemperatur:</td><td>%2.1f</td></tr>\
    <tr><td>Aussentemperatur:</td><td>%2.1f</td></tr>\
    <tr><td>Heizpumpe:</td><td>%s</td></tr>\
    <tr><td>Wasserpumpe:</td><td>%s</td></tr>\
    </table></td><td><img src='https://rasica.files.wordpress.com/2013/02/burning_flame_by_mattthesamurai-d3hc6m5.gif'></td></tr></table><br>\
    <form>\
    <table border =1><th>Warmwasser</th>\<th>Stunde</th>\
    <tr><td>Startzeit Morgens</td><td><Input type='text' value='%02d' name=b></td></tr>\
    <tr><td>Stopzeit Morgens</td><td><Input type='text' value='%02d' name=e></td></tr>\
    <tr><td>Startzeit Abends</td><td><Input type='text' value='%02d' name=B></td></tr>\
    <tr><td>Stopzeit Abends</td><td><Input type='text' value='%02d' name=E></td></tr>\
    <tr><td>Soll Temperatur</td><td><Input type='text' value='%2.1f' name=t></td></tr>\
    </table><table><tr><td><label>So<Input type='checkbox' name=0 %s></label>\ 
    <label>Mo<Input type='checkbox' name=1 %s></label>\
    <label>Di<Input type='checkbox' name=2 %s></label>\
    <label>Mi<Input type='checkbox' name=3 %s></label>\
    <label>Do<Input type='checkbox' name=4 %s></label>\
    <label>Fr<Input type='checkbox' name=5 %s></label>\
    <label>Sa<Input type='checkbox' name=6 %s></label>\
    </td></tr>\
    </table><br><table border=1><th>Heizpumpe</th>\<th>min/h</th>\
    <tr><td>Tagintervall</td><td><Input type='text' value='%02d' name=i></td></tr>\
    <tr><td>Nachtintervall</td><td><Input type='text' value='%02d' name=I></td></tr>\
    <tr><td>Bias</td><td><Input type='text' value='%02d' name=K></td></tr>\
   </table>\
    <label>Automatik<Input type='checkbox' name=a %s></label>\
    <br><button>Setzen</button>\
  </form><BR>\
  <table>\
  <tr><td>Kessel</td><td>Min:%2.1f</td><td>Max:%2.1f</td></tr>\
  <tr><td>Wasser</td><td>Min:%2.1f</td><td>Max:%2.1f</td></tr>\
  </table><br>%s\
  </body>",
  timeClient.getFormattedTime().c_str(),waterTemp,kessTemp,Temperature,
      Hrelais ? "an" : "aus", Wrelais  ? "an" : "aus",
      beginDay,endDay,beginNight,endNight,temperature,    
      Day[0] ? "checked" : "" ,Day[1] ? "checked" : "" ,Day[2] ? "checked" : "" ,Day[3] ? "checked" : "" ,Day[4] ? "checked" : "" ,Day[5] ? "checked" : "" ,Day[6] ? "checked" : "" ,
      intervallDay,intervallNight, bias,  SteuerungAut ? "checked" : "" ,
      minKessTemp, maxKessTemp, minWaterTemp, maxWaterTemp,
      errorMessage.c_str()
 );
 server.send ( 200, "text/html", temp );

}
void handleNotFound() {
 
 String message = "File Not Found\n\n";
 message += "URI: ";
 message += server.uri();
 message += "\nMethod: ";
 message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
 message += "\nArguments: ";
 message += server.args();
 message += "\n";
 for ( uint8_t intervallDay = 0; intervallDay < server.args(); intervallDay++ ) {
  message += " " + server.argName ( intervallDay ) + ": " + server.arg ( intervallDay ) + "\n";
 }
 server.send ( 404, "text/plain", message );
 errorMessage=message;

}
void setup ( void ) {
  Heltec.begin(true /*DisplayEnable Enable*/, true /*Serial Enable*/);
  Heltec.display->drawString(0,10,"Kesselsteuerung startet ...");
  Heltec.display->display();
  // init GPIO pins
 // pinMode ( led, OUTPUT );
  pinMode ( Relais, OUTPUT );
  pinMode ( Relais2, OUTPUT );
  //pinMode ( Button, INPUT );
 

 Serial.begin ( 115200 );
 WiFi.begin ( ssid, password );
 WiFiClient client;
 HTTPClient http;
 
  sensors.begin();
  timeClient.begin();
  timeClient.setTimeOffset(3600);

 
 // Wait for connection
 while ( WiFi.status() != WL_CONNECTED ) {
  delay ( 500 );
  //Serial.print ( "." );
    Heltec.display->drawString(0,20,"Waiting for network ...");
  Heltec.display->display();
 }
 //Serial.println ( "" );
 //Serial.print ( "Connected to " );
 //Serial.println ( ssid );
 //Serial.print ( "IP address: " );
 //Serial.println ( WiFi.localIP() );
  Heltec.display->drawString(0,20,ssid);
  Heltec.display->display();
 if ( MDNS.begin ( "esp8266" ) ) {
  errorMessage= "MDNS responder started";
 }
 server.on ( "/", handleRoot );
 server.on ( "/inline", []() {
 server.send ( 200, "text/plain", "this works as well" );
 } );
 server.onNotFound ( handleNotFound );
 server.begin();
 IP = WiFi.localIP().toString();
 errorMessage= IP; 
 // once force read the outside temp initially
  getWeatherData(true);
}

void relaisSet(){


  // No chec if conditions for switch on are given
  int h = timeClient.getHours();

  Wrelais = (((beginDay <= h)&&(h < endDay)) || ((beginNight <= h)&&(h < endNight))) && (waterTemp < temperature) && Day[timeClient.getDay()];
  Wrelais = Wrelais && kessTemp > waterTemp ; // Macht wenig Sinn mit kaltem Kessel Warmwasser zu machen mit 5 Grad Verlust

    if (Wrelais) digitalWrite(Relais, LOW); else  digitalWrite(Relais, HIGH);

  // Start relais Depend on time Intervall
  // If hour 0 until morning start OR evening Start until 23 -> Night intervall ... otherwise day intervall
  int m = timeClient.getMinutes();
  Minutes = m; // for the counter openweathermap

  int intervall;

  if (SteuerungAut) {
    //bias = analogRead (A0)/50-10; // +/- 10 Minutes
    intervallNight = 26 - Temperature*2 + bias;
    if (intervallNight < 0) intervallNight=0;
    if (intervallNight > 60) intervallNight=60;
    intervallDay = 32 - Temperature*2 + bias;
    if (intervallDay < 0 )intervallDay = 0;
    if (intervallDay > 60 )intervallDay = 60;
  } 
  
  intervall  = ((h<beginDay)||(h>endNight)) ? intervallNight : intervallDay;
  Hrelais = (m < intervall) && (kessTemp > 30) && (9 < month < 5)  ;  //Macht wenig Sinn mit kaltem Kessel zu heizen, auch Mai bis September bleibt die Heizung aus
  

    if (Hrelais) digitalWrite(Relais2, LOW); else  digitalWrite(Relais2, HIGH);

}


void OledDisplay(){
  char ntemp[30];
  Heltec.display->clear();
  snprintf ( ntemp, 21,"%02d-%02dh %02d-%02dh %02d:%02dh",beginDay,endDay,beginNight,endNight,timeClient.getHours(), timeClient.getMinutes());
  Heltec.display->drawString(0,0,ntemp);
  Heltec.display->display();
  snprintf ( ntemp, 30,"%2.0f° %c%2.0f° %c%2.0f° T%2.0f° %02d/%02d ",Temperature,Wrelais ? 'W':'w',waterTemp,Hrelais?'H':'h',kessTemp,temperature,intervallDay,intervallNight);
  Heltec.display->drawString(0,10,ntemp);
  Heltec.display->display();
  if (timeClient.getSeconds() >20)
  {
    snprintf ( ntemp, 30,"K:%2.0f°-%2.0f° W:%2.0f°-%2.0f° ",minKessTemp,maxKessTemp,minWaterTemp,maxWaterTemp);
    Heltec.display->drawString(0,20,ntemp);
  }
  else if  (timeClient.getSeconds() >20)
     {

      Heltec.display->drawString(0,20,IP);
    }
  
    else
      Heltec.display->drawString(0,20,errorMessage);
  Heltec.display->display( );
}

float kessTempCalc(float kt)// Der Messfühler sitzt aussen, daher wird die temperatur zu niederig angezeigt, das kompensieren wir hier
 {
  if (kt < 20.0) 
   return kt;
  else
   return (kt-20)/kt*0.3; // ca. 10 Grad bei 50 Grad Kessetemperatur, 0 bei 20 Grad
 }

void readTemp() {
  sensors.requestTemperatures();
  waterTemp=sensors.getTempCByIndex(0);
  if (waterTemp < -120) { errorMessage = "Water sensor fail; set to 0"; waterTemp=0.0;}
  else {
    if (waterTemp < minWaterTemp) minWaterTemp=waterTemp;
    if (waterTemp > maxWaterTemp) maxWaterTemp=waterTemp;
  }
  kessTemp=sensors.getTempCByIndex(1);//+ kessTempCalc(kessTemp); //Korrektur der Messstelle
  if (kessTemp < -120) { errorMessage = "Kettle sensor fail; set to 99"; kessTemp=99.9;}
  else {
   if (kessTemp < minKessTemp) minKessTemp=kessTemp;
   if (kessTemp > maxKessTemp) maxKessTemp=kessTemp;
  }

}

void loop ( void ) {
  timeClient.update();
  server.handleClient();
  analyzeArgs();
  readTemp();
  delay(500);
  relaisSet();
  delay(500);
  getWeatherData(false);
  OledDisplay();
 
}
