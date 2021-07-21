/*

*/
#include <Wire.h>  // This library is already built in to the Arduino IDE
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x3F, 20, 2);

#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 2 //D4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
// NTP Init
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
//WLAN Access
const char *ssid = "NationalparkSpessart";
const char *password = "62117742064635826514";
// WEb Sever Init
ESP8266WebServer server ( 80 );
// Global Variables
float waterTemp = 99.9;
float kessTemp = 99.9;
int b = 9;
int e = 10;
int B = 17;
int E = 19;
float t = 50.0;
int i = 30;
int I = 30;

boolean An = true;
boolean Wrelais = true; // warmwasser
boolean Hrelais = true; // heizpumper
boolean Day[7] = {true, true, true, true, true, true, true}; //0 0 sunday
// init GPIO pins
const int led = 13; //D7
const int Button = 0;
#define Relais 12 // D6
#define Relais2 14 // D5
//ONE_WIRE_BUS 5 //GPIO 14

// Read and interpret Arguments of Webserver
void analyzeArgs() {
  if (server.args() > 1) for (uint8_t j = 0; j < 7; j++) Day[j] = false;
  for ( uint8_t index = 0; index < server.args(); index++ ) {
    String s = server.arg(index);
    //Serial.println (server.argName ( index )+" "+server.argName ( index )[0] +"" + s);
    // Switch on paramter name and process the paramter value
    switch (server.argName ( index )[0] + 0) {
      case 'b': b = atoi( s.c_str() ); break; // Begin Morning
      case 'e': e = atoi( s.c_str() ); break; // End Morning
      case 'B': B = atoi( s.c_str() ); break; // Begin Evening
      case 'E': E = atoi( s.c_str() ); break; // End Evening
      case 't': t = atof( s.c_str() ); break; // Desired Temperature
      case 'a': An = 1 == atoi( s.c_str() ); break; // Switch Controller Function on off
      case 'i': i = atoi( s.c_str() ); break; // Intervall tagsueber
      case 'I': I = atoi( s.c_str() ); break; // Intervall nachts
      case '0' : Day[0] = true; break;
      case '1' : Day[1] = true; break;
      case '2' : Day[2] = true; break;
      case '3' : Day[3] = true; break;
      case '4' : Day[4] = true; break;
      case '5' : Day[5] = true; break;
      case '6' : Day[6] = true; break;
      default: ;
    }
  }
  if (i < 5) i = 5; if (I > 55) I = 60; //Avoid to short switch times
}
void handleRoot() {
  digitalWrite ( led, 1 );
  char temp[2000];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;
  snprintf ( temp, 2000,
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
    </td></tr></table><br><table border=1><th>Heizpumpe</th>\<th>min/h</th><tr><td>Tagintervall</td><td><Input type='text' value='%02d' name=i></td></tr>\
   <tr><td>Nachtintervall</td><td><Input type='text' value='%02d' name=I></td></tr></table>\
    <br><button>Setzen</button>\
  </form><BR>\
  </body>",
             timeClient.getFormattedTime().c_str(), waterTemp, kessTemp,
             Hrelais ? "an" : "aus", Wrelais  ? "an" : "aus",
             b, e, B, E, t,
             Day[0] ? "checked" : "" , Day[1] ? "checked" : "" , Day[2] ? "checked" : "" , Day[3] ? "checked" : "" , Day[4] ? "checked" : "" , Day[5] ? "checked" : "" , Day[6] ? "checked" : "" ,
             i, I
           );
  server.send ( 200, "text/html", temp );
  digitalWrite ( led, 0 );
}
void handleNotFound() {
  digitalWrite ( led, 1 );
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }
  server.send ( 404, "text/plain", message );
  digitalWrite ( led, 0 );
}
void setup ( void ) {
  lcd.init();   // initializing the LCD
  lcd.backlight(); // Enable or Turn On the backlight
  lcd.setCursor(0, 0);
  lcd.print("Starte ..."); // Start Print text to Line 1
  lcd.setCursor(0, 1);
  lcd.print("Kesselsteuerung"); // Start Print Test to Line 2
  // init GPIO pins
  pinMode ( led, OUTPUT );
  pinMode ( Relais, OUTPUT );
  pinMode ( Relais2, OUTPUT );
  pinMode ( Button, INPUT );

  digitalWrite ( led, 0 );
  Serial.begin ( 115200 );
  WiFi.begin ( ssid, password );
  Serial.println ( "" );
  sensors.begin();
  timeClient.begin();
  timeClient.setTimeOffset(3600);

  // Wait for connection
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }
  Serial.println ( "" );
  Serial.print ( "Connected to " );
  Serial.println ( ssid );
  Serial.print ( "IP address: " );
  Serial.println ( WiFi.localIP() );
  if ( MDNS.begin ( "esp8266" ) ) {
    Serial.println ( "MDNS responder started" );
  }
  server.on ( "/", handleRoot );
  //server.on ( "/test.svg", drawGraph );
  server.on ( "/inline", []() {
    server.send ( 200, "text/plain", "this works as well" );
  } );
  server.onNotFound ( handleNotFound );
  server.begin();
  Serial.println ( "HTTP server started" );
}
void relaisStart() {
  // First check if Button to toggle Controller Fuction on/off was pressed
  if (digitalRead(Button) == 0) An = !An;
  // No chec if conditions for switch on are given
  int h = timeClient.getHours();
  Wrelais = (((b <= h) && (h < e)) || ((B <= h) && (h < E))) && (waterTemp < t) && An && Day[timeClient.getDay()];
  if (Wrelais) digitalWrite(Relais, LOW); else  digitalWrite(Relais, HIGH);

 // digitalWrite ( led, Wrelais ); // With 3.3V Supply the Relais will not work, to test use LED

  // Start relais Depend on time Intervall
  // If hour 0 until morning start OR evening Start until 23 -> Night intervall ... otherwise day intervall
  int m = timeClient.getMinutes();

  int intervall;
  intervall  = ((h < b) || (h > E)) ? I : i;
  Hrelais = m <= intervall;
  if (Hrelais) digitalWrite(Relais2, LOW); else  digitalWrite(Relais2, HIGH);
}
void loop ( void ) {
  char ntemp[25];
  timeClient.update();
  server.handleClient();
  analyzeArgs();
  sensors.requestTemperatures();
  waterTemp = sensors.getTempCByIndex(0);
  kessTemp = sensors.getTempCByIndex(1);
  relaisStart();
  lcd.setCursor(0, 0);
  snprintf ( ntemp, 20, "%02d-%02dh %02d-%02dh", b, e, B, E);
  lcd.print(ntemp);
  lcd.setCursor(0, 1);
  snprintf ( ntemp, 20, "W%2.0f-H%2.0f %02d/%02d", waterTemp, kessTemp, i, I);
  lcd.print(ntemp);
}
