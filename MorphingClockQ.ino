
/*
Thanks to all who made this project possible!

remix from HarryFun's great Morphing Digital Clock idea https://github.com/hwiguna/HariFun_166_Morphing_Clock
awesome build tutorial here:https://www.instructables.com/id/Morphing-Digital-Clock/

openweathermap compatibility added thanks to Imirel's remix https://github.com/lmirel/MorphingClockRemix
some bits and pieces from kolle86's MorphingClockRemixRemix https://github.com/kolle86/MorphingClockRemixRemix
small seconds by Trenck

this remix by timz3818 adds am/pm, working night mode (change brightness and colors), auto dissapearing first digit, and lower brightness icons

This remix by Gary Quiring 3/2022
Changed NTP calls, would not compile
USA Daylight savings time won't work with NTPClientlib, it's default is EU DST, change the line below for USA default
library/NTPCLientlib/src/NTClientlib.h
#define DEFAULT_DST_ZONE        DST_ZONE_USA

Removed day/night mode
Changed some RGB colors and made default colors for each area (Wind, Weather Text, Clock, Date)
Changed Tiny font for number 1.  It looked weird with straight line always right justified
Removed leading zero from date display
Changed wind = 0 to display "CALM"
Add Weather text toggle for text or animation.  I found the animation hard to understand.  Text displays words like Cloudy
Fixed inconsistenty with web control variables.  The code was not always referencing the changed variable but the hardcoded
variables in params.h.  Another words changing the settings on the web had no effect on the clock/weather display.  
Fixed the config file writing.  location and apiKey were declared as Strings which was corrupting the file write.  No clue why
it was easier to declare them as char arrays and it worked.
Added Reset Config File to web options.  Most likely for developer use only.  It's handy when the file is corrupt.    
Added urldecode function to remove %20's from the web entries
Added day of week to the date line
Wind and humidity will be alternately displayed every 10 seconds
Created Wifi Connection function.  It will try config file first and then params.h for SSID and Password
Web logic was broken for changing SSID and Password, it never checked to see if it could connect before saving the settings

Required to compile:
AdaFruit v1.6.1
PxMatrix by Dom B v1.3.0
Wifi Manager by TZ .15.0
From Board Manager select ESP8266 2.7.4 anything 3.0 or greater won't compile


provided 'AS IS', use at your own risk
 */


// we need these:
#include <TimeLib.h>
#include <NtpClientLib.h>
#include <ESP8266WiFi.h>
#include <PxMatrix.h>
#include "FS.h"
#include <ArduinoJson.h>
#include "TinyFont.h"
#include "Digit.h"
#include "Digitsec.h"
#include "params.h"

//ESP8266 setup
#ifdef ESP8266
#include <Ticker.h>
Ticker display_ticker;
#define P_LAT 16
#define P_A 5
#define P_B 4
#define P_C 15
#define P_D 12
#define P_E 0
#define P_OE 2
#endif

// Pins for LED MATRIX
PxMATRIX display(64, 32, P_LAT, P_OE, P_A, P_B, P_C, P_D, P_E);


void getWeather ();
// needed variables
byte hh;
byte mm;
byte ss;
byte ntpsync = 1;
//  const char ntpsvr[]   = "pool.ntp.org";
const char ntpsvr[]   = "time.google.com";


/////////==========NOTES==========//////////
// by default, this is setup for displays using FM6126A drivers and 1/16 scan
// if not using a display with FM6126A, remove/modify display.setDriverChip(FM6126A); and display.begin(16); as needed
// See PxMAtrix documentation for more info regarding getting your LED matrix working
//
// double reset will not work, set WIFI info in params.h instead
// 
//once connected to WIFI, you can change time zone by going to:
// http://clock.ip.address/timezone/desired_offset 
// ex. http://clock.ip.address/timezone/-8 sets GMT -8
//
// the default icons are dim. you can use the commented out ones instead if you want brighter icons
//
// Known issues: the display will freeze for 5-10sec when weather info is updated




/////////========== CONFIGURATION ==========//////////
/* Set your wifi info, api key, date format, unit type, and location in params.h
 * If you want to adjust color/brightness/position of screen objects you can do that in the following sections.
 */



  byte day_bright = 70; //sets daytime brightness; 70 is default. values higher than this may not work.
  byte dim_bright = 25; // sets brightness for partly dimmed mode
  byte nm_bright = 25; // sets brightness for night mode

//  byte dim_start = 19; // sets time (24h) for display to dim partly. Ends when night mode starts
//  byte nm_start = 23; // start time (24h) for night mode; must be either 22 or 23
//  byte nm_end = 1; // end time for night mode; must be between 1 and 9
  
//=== SEGMENTS ===
// This section determines the position of the HH:MM ss digits onscreen with format digit#(&display, 0, x_offset, y_offset, irrelevant_color)

  byte digit_offset_amount;
 
  byte Digitsec_x = 56 + digit_offset_amount;
  byte Digit_x = 62 + digit_offset_amount;

  Digitsec digit0(&display, 0, Digitsec_x - 7*1, 14, display.color565(255, 255, 255));
  Digitsec digit1(&display, 0, Digitsec_x - 7*2, 14, display.color565(255, 255, 255));
  Digit digit2(&display, 0, Digit_x - 4 - 9*3, 8, display.color565(255, 255, 255));
  Digit digit3(&display, 0, Digit_x - 4 - 9*4, 8, display.color565(255, 255, 255));
  Digit digit4(&display, 0, Digit_x - 7 - 9*5, 8, display.color565(255, 255, 255));
  Digit digit5(&display, 0, Digit_x - 7 - 9*6, 8, display.color565(255, 255, 255));

 
//=== COLORS ===
// this defines the colors used for the time, date, and wind info
// format:  display.color565 (R,G,B), with RGB values from 0 to 255
// default is set really dim (~10% of max), increase for more brightness if needed

  int color_disp = display.color565 (40, 40, 50);    // primary color


// some other colors
// R G B
  int cc_blk = display.color565 (0, 0, 0);     // black
  int cc_wht = display.color565 (25, 25, 25);  // white
  int cc_red = display.color565 (50, 0, 0);    // red
//  int cc_org = display.color565 (25, 16, 0);   // orange
  int cc_org = display.color565 (25, 10, 0);   // orange
  int cc_grn = display.color565 (0, 50, 0);    // green
//gq  int cc_blu = display.color565 (0, 13, 25);   // blue
  int cc_blu = display.color565 (0, 0, 150);   // blue
  int cc_bblu = display.color565 (0, 128, 255);    // bright blue
  int cc_ylw = display.color565 (45, 45, 0);   // yellow
  int cc_gry = display.color565 (10, 10, 10);  // gray
  int cc_dgr = display.color565 (3, 3, 3);     // dark grey
  int cc_cyan = display.color565 (0, 25, 25);  // cyan
  int cc_ppl = display.color565 (25, 0, 25);   // purple

  // alternate, brighter text colors. Best used with the alternate brighter icons
  /*
  int cc_blk = display.color565 (0, 0, 0);        // black
  int cc_wht = display.color565 (255, 255, 255);  // white
  int cc_red = display.color565 (255, 0, 0);      // red
  int cc_org = display.color565 (255, 165, 0);    // orange
  int cc_grn = display.color565 (0, 255, 0);      // green
  int cc_blu = display.color565 (0, 128, 255);    // blue
  int cc_ylw = display.color565 (255, 255, 0);    // yellow
  int cc_gry = display.color565 (128, 128, 128);  // gray
  int cc_dgr = display.color565 (30, 30, 30);     // dark gray
  int cc_lblu = display.color565 (0, 255, 255);   // light blue
  int cc_ppl = display.color565 (255, 0, 255);    // purple
   */


// Default colors
// Temperature color is set based on temperature for example red is for hot
int cc_time = cc_cyan;  //default color for time
int cc_wind = cc_ylw;   //default color for wind
int cc_date = cc_grn;   //default color for date
int cc_wtext = cc_wht;  //default color for weather description

//===OTHER SETTINGS===
int ani_speed = 500; // sets animation speed
int weather_refresh = 15; // sets weather refresh interval in minutes; must be between 1 and 59

//=== POSITION ===
// to stop seeing data use "nosee" to move its position outside of the display range
byte nosee = 100;

// Set weather icon position; TF_cols = 4 by default
  byte img_x = 7*TF_COLS+1;
  byte img_y = 2;

// Date position
  byte date_x = 2;
  byte date_y = 26;

// Temperature position
  byte tmp_x = 12*TF_COLS;
  byte tmp_y = 2;

// Wind position and label
// I'm not clear on why the first position is shifted by TF_COLS wasting space
//  byte wind_x = 1*TF_COLS;
  byte wind_x = 0;
  byte wind_y = 2;

// Pressure position and label
  byte press_x = nosee;
  byte press_y = nosee;
  char press_label [] = "";
  
// Humidity postion and label
  byte humi_x = nosee;
  byte humi_y = nosee;
  char humi_label [] = "%";

// Text weather condition
  byte wtext_x = 5*TF_COLS-3;
  byte wtext_y = 2;
  String weather_text = "        ";
 
String wind_lstr = "   ";
String humi_lstr = "   ";
int wind_humi;

WiFiServer httpsvr (80); //Initialize the server on Port 80
unsigned char dbri = 255;


//#ifdef ESP8266
// ISR for display refresh
void display_updater ()
{
   display.display (day_bright); //set brightness for day
}

//#endif

//settings
#define NVARS 11
#define LVARS 40
char c_vars [NVARS][LVARS];
typedef enum e_vars {
  EV_SSID,
  EV_PASS,
  EV_TZ,
  EV_24H,
  EV_METRIC,
  EV_DATEFMT,
  EV_OWMK,
  EV_GEOLOC,
  EV_DST,
  EV_OTA,
  EV_WANI
};

bool toBool (String s)
{
  return s.equals ("true");
}

int vars_read()
{
    //Remove file for testing when it has corrupt data
    //SPIFFS.remove("/mvars.cfg");
    //File varf = SPIFFS.open ("/mvars.cfg", "w");
    //  varf.close ();
    //return 1;

  File varf = SPIFFS.open ("/mvars.cfg", "r");
  if (!varf)
  {
    Serial.println ("Failed to open config file");
    return 0;
  }
  //read vars
  for (int i = 0; i < NVARS; i++)
    for (int j = 0; j < LVARS; j++)
      c_vars[i][j] = (char)varf.read ();

  varf.close ();
  show_config_vars ();
  return 1;
}

int vars_write ()
{
  File varf = SPIFFS.open ("/mvars.cfg", "w");
  if (!varf)
  {
    Serial.println ("Failed to open config file");
    return 0;
  }
  Serial.println ("Writing config file ......");
  for (int i = 0; i < NVARS; i++) {
    for (int j = 0; j < LVARS; j++) {
      if (varf.write (c_vars[i][j]) != 1)
        Serial.println ("error writing var");
    }
  //  varf.write ("\n");
  //  Serial.println (" ");
  }
  //
  varf.close ();
  return 1;
}

unsigned char h2int(char c)
{
    if (c >= '0' && c <='9'){
        return((unsigned char)c - '0');
    }
    if (c >= 'a' && c <='f'){
        return((unsigned char)c - 'a' + 10);
    }
    if (c >= 'A' && c <='F'){
        return((unsigned char)c - 'A' + 10);
    }
    return(0);
}

//Strip out URL encoding
String urldecode(String str)
{
    
    String encodedString="";
    char c;
    char code0;
    char code1;
    for (int i =0; i < str.length(); i++){
        c=str.charAt(i);
      if (c == '+'){
        encodedString+=' ';  
      }else if (c == '%') {
        i++;
        code0=str.charAt(i);
        i++;
        code1=str.charAt(i);
        c = (h2int(code0) << 4) | h2int(code1);
        encodedString+=c;
      } else{
        
        encodedString+=c;  
      }
      
      yield();
    }
    
   return encodedString;
}

//Debugging 
void show_config_vars ()
{
  Serial.println ("From c_vars....");
  
  Serial.print ("SSID=");
  Serial.println (c_vars[EV_SSID]);
  Serial.print ("Password=");
  Serial.println (c_vars[EV_PASS]);
  Serial.print ("Timezone=");
  Serial.println (c_vars[EV_TZ]);
  Serial.print ("Military=");
  Serial.println (c_vars[EV_24H]);
  Serial.print ("Metric=");
  Serial.println (c_vars[EV_METRIC]);
  Serial.print ("Date-Format=");
  Serial.println (c_vars[EV_DATEFMT]);
  Serial.print ("apiKey=");
  Serial.println (c_vars[EV_OWMK]);
  Serial.print ("Location=");
  Serial.println (c_vars[EV_GEOLOC]);
  Serial.print ("DST=");
  Serial.println (c_vars[EV_DST]);
  Serial.print ("Weather Animation=");
  Serial.println (c_vars[EV_WANI]);
}

//If the config file is not setup copy from param.h
void init_config_vars ()
{
      strcpy (c_vars[EV_SSID], wifi_ssid); 
      strcpy (c_vars[EV_PASS], wifi_pass);
      strcpy (c_vars[EV_TZ], timezone);
      strcpy (c_vars[EV_24H], military);
      strcpy (c_vars[EV_METRIC], u_metric);
      strcpy (c_vars[EV_DATEFMT], date_fmt);
      strcpy (c_vars[EV_OWMK], apiKey);
      strcpy (c_vars[EV_GEOLOC], location);
      strcpy (c_vars[EV_DST], dst_sav);
      strcpy (c_vars[EV_WANI], w_animation);
}

//Wifi Connection
int connect_wifi (String n_wifi, String n_pass) 
{
  int c_cnt = 0;
  Serial.print ("Trying WiFi Connect:");
  Serial.println (n_wifi);
  
  WiFi.begin (n_wifi, n_pass);
  WiFi.mode(WIFI_STA);
  while (WiFi.status () != WL_CONNECTED)
  {
    delay (500);
    Serial.print(".");
    c_cnt++;
    if (c_cnt > 50) {
      Serial.println ("Wifi Connect Failed");
      return 1;
    }
  }
  Serial.println ("success!");
  Serial.print ("IP Address is: ");
  Serial.println (WiFi.localIP ());  //
  return 0;

}
  
void setup ()
{  
  Serial.begin (9600);
  while (!Serial)
    delay (500); //delay for Leonardo
    display.begin (16);
//     display.setDriverChip(FM6126A);
//     display.setMuxDelay(0,1,0,0,0);
        
#ifdef ESP8266
  display_ticker.attach (0.002, display_updater);
#endif
  //
  TFDrawText (&display, String ("  MORPH CLOCK  "), 0, 1, cc_blu);
  TFDrawText (&display, String ("  STARTING UP  "), 0, 10, cc_blu);

  // Read the config file
  if (SPIFFS.begin ())
  {
    Serial.println ("SPIFFS Initialize....ok");
    if (!vars_read ())
    {
      init_config_vars ();  //Copy from params.h to EV array
    }
  }
  else
  {
    Serial.println ("SPIFFS Initialization...failed");
  }

   String lstr = String ("TIMEZONE:") + String (EV_TZ);
   TFDrawText (&display, lstr, 4, 24, cc_cyan);
   
   show_config_vars ();
  
  
  //connect to wifi network

  if ( connect_wifi(c_vars[EV_SSID],c_vars[EV_PASS]) == 1) {  // Try settings in config file
    TFDrawText (&display, String("WIFI FAILED CONFIG"), 1, 10, cc_grn);
    if (connect_wifi(wifi_ssid,wifi_pass) == 1) {  // Try settings in params.h
        Serial.println ("Cannot connect to anything, RESTART ESP");
        TFDrawText (&display, String ("WIFI FAILED PARAMS.H"), 1, 10, cc_grn);
        delay(1000);
        ESP.restart(); // Restart the ESP, cannot connect to Wifi
    }
  }
      
  TFDrawText (&display, String("WIFI CONNECTED "), 3, 10, cc_grn);
  TFDrawText (&display, String(WiFi.localIP().toString()), 4, 17, cc_grn);
  

//  delay (3000);  Why wait?
 
   getWeather ();
   httpsvr.begin (); // Start the HTTP Server

  //start NTP
  Serial.print ("TimeZone for NTP.Begin:");
  Serial.println (c_vars[EV_TZ]);
  NTP.begin (ntpsvr, String (c_vars[EV_TZ]).toInt(), toBool (String (c_vars[EV_DST])));
  NTP.setInterval (10);//force rapid sync in 10sec
  //
  NTP.onNTPSyncEvent ([](NTPSyncEvent_t ntpEvent)
  {
  
  switch (ntpEvent) {
    case noResponse:
       Serial.print ("Time Sync error: ");
       Serial.println ("NTP server not reachable");
       break;
    case invalidAddress:
       Serial.print ("Time Sync error: ");
       Serial.println ("Invalid NTP server address");
       break;
    default:
       Serial.print ("Got NTP time: ");
       Serial.println (NTP.getTimeDateString (NTP.getLastNTPSync ()));
       ntpsync = 1;
       break;
  }
  
  });

  //prep screen for clock display

  display.fillScreen (0);
}


const char server[]   = "api.openweathermap.org";
WiFiClient client;
int in = -10000;
int tempMax = -10000;
int tempM = -10000;
int presM = -10000;
int humiM = -10000;
int condM = -1;  //-1 - undefined, 0 - unk, 1 - sunny, 2 - cloudy, 3 - overcast, 4 - rainy, 5 - thunders, 6 - snow
String condS = "";
int wind_speed;
int wind_nr;
String wind_direction = "";
int gust = 0;

  
void getWeather ()
{
 //GQGQ if (!apiKey.length ())
//  if (apiKey.length() == 0)
//  {
//    Serial.println ("w:missing API KEY for weather data, skipping"); 
//    return;
//  }
  Serial.print ("i:connecting to weather server.. "); 
  // if you get a connection, report back via serial: 
  if (client.connect (server, 80))
  { 
    Serial.println ("connected."); 
    // Make a HTTP request: 
    client.print ("GET /data/2.5/weather?"); 
    client.print ("q="+ String(c_vars[EV_GEOLOC]));
    client.print ("&appid="+ String(c_vars[EV_OWMK]));
    client.print ("&cnt=1"); 
    (*u_metric=='Y')?client.println ("&units=metric"):client.println ("&units=imperial");
    client.println ("Host: api.openweathermap.org"); 
    client.println ("Connection: close");
    client.println (); 
  } 
  else 
  { 
    Serial.println ("Weather:unable to connect");
    return;
  } 
  delay (1500);
  String sval = "";
  int bT, bT2;
  //do your best
  String line = client.readStringUntil ('\n');
  if (!line.length ())
    Serial.println ("Weather:unable to retrieve data");
  else
  {
    bT = line.indexOf ("\"icon\":\"");
    if (bT > 0)
    {
      bT2 = line.indexOf ("\"", bT + 8);
      sval = line.substring (bT + 8, bT2);
      //0 - unk, 1 - sunny, 2 - cloudy, 3 - overcast, 4 - rainy, 5 - thunders, 6 - snow
      if (sval.equals("01d"))
        condM = 1; //sunny
      else if (sval.equals("01n"))
        condM = 8; //clear night
      else if (sval.equals("02d"))
        condM = 2; //partly cloudy day
      else if (sval.equals("02n"))
        condM = 10; //partly cloudy night
      else if (sval.equals("03d"))
        condM = 3; //overcast day
      else if (sval.equals("03n"))
        condM = 11; //overcast night
      else if (sval.equals("04d"))
        condM = 3;//overcast day
      else if (sval.equals("04n"))
        condM = 11;//overcast night
      else if (sval.equals("09d"))
        condM = 4; //rain
      else if (sval.equals("09n"))
        condM = 4;
      else if (sval.equals("10d"))
        condM = 4;
      else if (sval.equals("10n"))
        condM = 4;
      else if (sval.equals("11d"))
        condM = 5; //thunder
      else if (sval.equals("11n"))
        condM = 5;
      else if (sval.equals("13d"))
        condM = 6; //snow
      else if (sval.equals("13n"))
        condM = 6;
      else if (sval.equals("50d"))
        condM = 7; //haze (day)
      else if (sval.equals("50n"))
        condM = 9; //fog (night)
      //
      condS = sval;
    }
    //tempM
    bT = line.indexOf ("\"temp\":");
    if (bT > 0)
    {
      bT2 = line.indexOf (",\"", bT + 7);
      sval = line.substring (bT + 7, bT2);
      tempM = sval.toInt ();
    }
    else
      Serial.println ("temp NOT found!");

      
    //pressM
    bT = line.indexOf ("\"pressure\":");
    if (bT > 0)
    {
      bT2 = line.indexOf (",\"", bT + 11);
      sval = line.substring (bT + 11, bT2);
      presM = sval.toInt();
    }
    else
      Serial.println ("pressure NOT found!");
    //humiM
    bT = line.indexOf ("\"humidity\":");
    if (bT > 0)
    {
      bT2 = line.indexOf (",\"", bT + 11);
      sval = line.substring (bT + 11, bT2);
      humiM = sval.toInt();
    }
    else
      Serial.println ("humidity NOT found!");
    //gust
//    bT = line.indexOf ("\"gust\":");
//    if (bT > 0)
//    {
//      bT2 = line.indexOf (",\"", bT + 7);
//      sval = line.substring (bT + 7, bT2);
//      gust = sval.toInt();
//    }
//    else
//    {
//      gust = 0;
//    }   

  //wind speed
    bT = line.indexOf ("\"speed\":");
    if (bT > 0)
    {
      bT2 = line.indexOf (",\"", bT + 8);
      sval = line.substring (bT + 8, bT2);
      wind_speed = sval.toInt();
    }
    else
      Serial.println ("windspeed NOT found 2!");
    // timezone offset
     bT = line.indexOf ("\"timezone\":");
    if (bT > 0)
    {
      int tz;
      bT2 = line.indexOf (",\"", bT + 11);
      sval = line.substring (bT + 11, bT2);
      tz = sval.toInt()/3600;
    }
    else
      Serial.println ("timezone offset NOT found!");
              
    //wind direction
    bT = line.indexOf ("\"deg\":");
    if (bT > 0)
    {
      bT2 = line.indexOf (",\"", bT + 6);
      sval = line.substring (bT + 6, bT2);
      wind_nr = round(((sval.toInt() % 360))/45.0) + 1;
      switch (wind_nr){
        case 1:
          wind_direction = "N";
          break;
        case 2:
          wind_direction = "NE";
          break;
        case 3:
          wind_direction = "E";
          break;
        case 4:
          wind_direction = "SE";
          break;
        case 5:
          wind_direction = "S";
          break;
        case 6:
          wind_direction = "SW";
          break;
        case 7:
          wind_direction = "W";
          break;
        case 8:
          wind_direction = "NW";
          break;
        case 9:
          wind_direction = "N";
          break;        
        default:
          wind_direction = "";
          break;
      }                
    }
    else
    {
      Serial.println ("Windspeed NOT found 3!");    
      wind_direction = "";
    } 
  }//connected
  
}

#include "TinyIcons.h"
#include "WeatherIcons.h"

int xo = 1, yo = 26;
char use_ani = 0;
void draw_weather_conditions ()
{
  //0 - unk, 1 - sunny, 2 - cloudy, 3 - overcast, 4 - rainy, 5 - thunders, 6 - snow, 7, hazy, 9 - fog
  
  xo = img_x;
  yo = img_y;
  
  if (condM == 0)
  {
    Serial.print ("!weather condition icon unknown, show: ");
    Serial.println (condS);
    int cc_dgr = display.color565 (30, 30, 30);
    //draw the first 5 letters from the unknown weather condition
    String lstr = condS.substring (0, (condS.length () > 5?5:condS.length ()));
    lstr.toUpperCase ();
    TFDrawText (&display, lstr, xo, yo, cc_dgr);
  }
  else
  {
  //  TFDrawText (&display, String("     "), xo, yo, 0);
  }
    xo = img_x;
    yo = img_y;
    switch (condM)
    {
    case 0://unk
      break;
    case 1://sunny
      DrawIcon (&display, sunny_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 2://cloudy
      DrawIcon (&display, cloudy_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 3://overcast
      DrawIcon (&display, ovrcst_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 4://rainy
      DrawIcon (&display, rain_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 5://thunders
      DrawIcon (&display, thndr_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 6://snow
      DrawIcon (&display, snow_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 7://mist
      DrawIcon (&display, mist_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 8://clear night
      DrawIcon (&display, moony_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 9://fog night
      DrawIcon (&display, mistn_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 10://partly cloudy night
      DrawIcon (&display, cloudyn_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 11://cloudy night
      DrawIcon (&display, ovrcstn_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
  }
}

//Convert weather condition code to text
void weather_text_conversion()
{

switch (condM)
    {
    case 1:
      weather_text = " SUNNY  ";
      break;
    case 2:
      weather_text = "P-CLOUDY";
      break;
    case 3:
      weather_text = "OVERCAST";
      break;
    case 4:
      weather_text = "  RAIN  ";
      break;
    case 5:
      weather_text = "T-STORMS";
      break;
    case 6:
      weather_text = "  SNOW  ";
      break;
    case 7:
      weather_text = "  HAZY  ";
      break;
    case 8:
      weather_text = " CLEAR  ";
      break;
    case 9:
      weather_text = " FOGGY  ";
      break;
    case 10:
      weather_text = " CLOUDY ";
      break;
    case 11:
      weather_text = "OVERCAST";
      break;
    default:
      weather_text = "        ";  
    }

}
          
//
void draw_weather ()
{
  int value = 0;

  // Clear the top line
//GQGQ  TFDrawText (&display, String("                   "), tmp_x, tmp_y, cc_dgr);
  
  if (tempM == -10000 || humiM == -10000 || presM == -10000)
  {
    //TFDrawText (&display, String("NO WEATHER DATA"), 1, 1, cc_dgr);
    Serial.println ("No weather data available");
  }
  else {
  
    //-temperature
    int lcc = cc_red;
    if (*u_metric == 'Y')
    {
      
        lcc = cc_red;
      if (tempM < 30)
        lcc = cc_org;
      if (tempM < 25)
        lcc = cc_ylw;
      if (tempM < 20)
        lcc = cc_grn;
      if (tempM < 15)
        lcc = cc_blu;
      if (tempM < 10)
        lcc = cc_cyan;        
      if (tempM < 1)
        lcc = cc_wht;
    }
    else
    {
      //F
      if (tempM < 79)
        lcc = cc_grn;
      if (tempM < 64)
        lcc = cc_blu;
      if (tempM < 43)
        lcc = cc_wht;
    }

    String lstr = String (tempM) + String((*u_metric=='Y')?"C":"F");
    
    //Padding Temp with spaces to keep position the same 
    switch (lstr.length ()) {
       case 2:
         lstr = String ("  ") + String (lstr);
         break;
       case 3:
         lstr = String (" ") + String (lstr);
         break;      
    }    
 
    TFDrawText (&display, lstr, tmp_x, tmp_y, lcc); // draw temperature
    
    //weather conditions
    //-humidity
    lcc = cc_red;
    if (humiM < 80)
      lcc = cc_org;
    if (humiM < 60)
      lcc = cc_grn;
    if (humiM < 40)
      lcc = cc_blu;
    if (humiM < 20)
      lcc = cc_wht;
    humi_lstr = String (humiM) + String (humi_label) + String (" ");
    TFDrawText (&display, humi_lstr, humi_x, humi_y, lcc); // humidity
 
    int cc = color_disp;
    cc = color_disp;
    
   //-pressure
//    lstr = String (presM)+ String(press_label);
//    xo = press_x;
//    yo = press_y;
//  	if(presM < 1000)
//        xo= press_x + 1;   
//    TFDrawText (&display, lstr, xo, yo, cc);
   
    //draw wind speed and direction
    if (wind_speed > -10000)
    { 
      if (wind_speed != 0) {
        wind_lstr = String (wind_direction) + String (wind_speed);
        switch (wind_lstr.length ()) {     //We have to pad the string to exactly 4 characters
        case 2:
         wind_lstr = String (wind_lstr) + String ("  ");
         break;
        case 3:
         wind_lstr = String (wind_lstr) + String (" ");
         break;
        }      
      }  
      else
        wind_lstr = String ("CALM");   
        
      wind_humi = 1;  //Reset switch for toggling wind or humidity display
      TFDrawText (&display,wind_lstr, wind_x, wind_y, cc_wind);
    }

    if ( String (c_vars[EV_WANI]) == "N" ) {
      weather_text_conversion();
      lstr = String (weather_text);
      TFDrawText (&display,lstr, wtext_x, wtext_y, cc);
    }  
    else {
      draw_weather_conditions ();
    }
  }
}
// End display weather

void draw_date ()
{
  //date below the clock
  long tnow = now();
  String lstr = "";
  
  for (int i = 0; i < 5; i += 2)
  {
    switch (date_fmt[i])
    {
      case 'D':
        lstr += (day(tnow) < 10 ? " " + String(day(tnow)) : String(day(tnow)));
        if (i < 4)
          lstr += date_fmt[i + 1];
        break;
      case 'M':  //Replaced leading 0 with space where double quotes are
        lstr += (month(tnow) < 10 ? " " + String(month(tnow)) : String(month(tnow)));
        if (i < 4)
          lstr += date_fmt[i + 1];
        break;
      case 'Y':
        lstr += String(year(tnow));
        if (i < 4)
          lstr += date_fmt[i + 1];
        break;
    }
  }
  //
  String DayofWeek = "  ";
  switch (weekday(tnow)) {
    case 1:
      DayofWeek = " SUN";
      break;
    case 2:
      DayofWeek = " MON";
      break;
    case 3:
      DayofWeek = " TUE";
      break;
    case 4:
      DayofWeek = " WED";
      break;
    case 5:
      DayofWeek = " THR";
      break;
    case 6:
      DayofWeek = " FRI";
      break;
    case 7:
      DayofWeek = " SAT";
      break;
  }

  lstr += String(DayofWeek);
  
  if (lstr.length())
     TFDrawText (&display, lstr, date_x, date_y, cc_date);

}


void draw_animations (int stp)
{
  //weather icon animation
    String lstr = "";
    xo = img_x;
    yo = img_y;
  //0 - unk, 1 - sunny, 2 - cloudy, 3 - overcast, 4 - rainy, 5 - thunders, 6 - snow
  if (use_ani)
  {
    int *af = NULL;
	  switch (condM)
	  {
		case 1:
			af = suny_ani[stp%5];
		  break;
		case 2:
			af = clod_ani[stp%10];
		  break;
		case 3:
			af = ovct_ani[stp%5];
		  break;
		case 4:
			af = rain_ani[stp%5];
		  break;
		case 5:
			af = thun_ani[stp%5];
		  break;
		case 6:
			af = snow_ani[stp%5];
		  break;
		case 7:
			af = mist_ani[stp%4];
		  break;
		case 8:
			af = mony_ani[stp%17];
		  break;
		case 9:
			af = mistn_ani[stp%4];
		  break;
		case 10:
			af = clodn_ani[stp%10];
		  break;
		case 11:
			af = ovctn_ani[stp%1];
		  break;
	  }
    //draw animation
    if (af)
      DrawIcon (&display, af, xo, yo, 10, 5);
  }
}

byte prevhh = 0;
byte prevmm = 0;
byte prevss = 0;
long tnow;
WiFiClient httpcli;


//convert hex letter to value
char hexchar2code (const char *hb)
{
  if (*hb >= 'a')
    return *hb - 'a' + 10;
  if (*hb >= 'A')
    return *hb - 'A' + 10;
  return *hb - '0';
}

//To find the values they are sandwiched between search and it always ends before "HTTP /"
//Pidx + ? is length of string searching for ie "?geoloc=" = length 8, pidx + 8
//pidx2 is end of string location for HTTP /
void web_server ()
{
  httpcli = httpsvr.available ();
  if (httpcli) 
  {
    char svf = 0, rst = 0;
    //Read what the browser has sent into a String class and print the request to the monitor
    String httprq = httpcli.readString ();
    // Looking under the hood
    // Serial.println (httprq);
    int pidx = -1;
    //
    String httprsp = "HTTP/1.1 200 OK\r\n";
    httprsp += "Content-type: text/html\r\n\r\n";
    httprsp += "<!DOCTYPE HTML>\r\n<html>\r\n";

    if ((pidx = httprq.indexOf ("GET /datetime/")) != -1)
    {
      int pidx2 = httprq.indexOf (" ", pidx + 14);
      if (pidx2 != -1)
      {
        String datetime = httprq.substring (pidx + 14, pidx2);
        //display.setBrightness (bri.toInt ());
        int yy = datetime.substring(0, 4).toInt();
        int MM = datetime.substring(4, 6).toInt();
        int dd = datetime.substring(6, 8).toInt();
        int hh = datetime.substring(8, 10).toInt();
        int mm = datetime.substring(10, 12).toInt();
        int ss = 0;
        if (datetime.length() == 14)
        {
          ss = datetime.substring(12, 14).toInt();
        }
        //void setTime(int hr,int min,int sec,int dy, int mnth, int yr)
        setTime(hh, mm, ss, dd, MM, yy);
        ntpsync = 1;
      }
    }

    else if (httprq.indexOf ("GET /ota/") != -1)
    {
      //GET /ota/?otaloc=192.168.2.38%3A8000%2Fespweather.bin HTTP/1.1
      pidx = httprq.indexOf ("?otaloc=");
      int pidx2 = httprq.indexOf (" HTTP/", pidx);
      if (pidx2 > 0)
      {
        strncpy(c_vars[EV_OTA], httprq.substring(pidx + 8, pidx2).c_str(), LVARS * 3);
        //debug_print (">ota1:");
        //debug_println (c_vars[EV_OTA]);
        char *bc = c_vars[EV_OTA];
        int   ck = 0;
        //debug_print (">ota2:");
        //debug_println (bc);
        //convert in place url %HH excaped chars
        while (*bc > 0 && ck < LVARS * 3)
        {
          if (*bc == '%')
          {
            //convert URL chars to ascii
            c_vars[EV_OTA][ck] = hexchar2code (bc+1)<<4 | hexchar2code (bc+2);
            bc += 2;
          }
          else
            c_vars[EV_OTA][ck] = *bc;
          //next one
          //debug_println (c_vars[EV_OTA][ck]);
          bc++;
          ck++;
        }
        c_vars[EV_OTA][ck] = 0;
        svf = 1;
      }
    }
    //location
    else if (httprq.indexOf ("GET /geoloc/") != -1)
    {
      pidx = httprq.indexOf ("?geoloc=");
      int pidx2 = httprq.indexOf (" HTTP/", pidx);
      if (pidx2 > 0)
      {
        String  location = urldecode ( httprq.substring(pidx + 8, pidx2).c_str() );
        strncpy(c_vars[EV_GEOLOC], location.c_str(), LVARS*3 );
        getWeather ();
        draw_weather_conditions ();
        svf = 1;
      }
    }
    //openweathermap.org key
    else if (httprq.indexOf ("GET /owm/") != -1)
    {
      pidx = httprq.indexOf ("?owmkey=");
      int pidx2 = httprq.indexOf (" HTTP/", pidx);
      if (pidx2 > 0)
      {
        strncpy(c_vars[EV_OWMK], httprq.substring(pidx + 8, pidx2).c_str(), LVARS * 3);
        getWeather ();
        draw_weather_conditions ();
        svf = 1;
      }
      //
    }
    else if (httprq.indexOf ("GET /wifi/") != -1)
    {
      //GET /wifi/?ssid=ssid&pass=pass HTTP/1.1
      pidx = httprq.indexOf ("?ssid=");
      int pidx2 = httprq.indexOf ("&pass=");
      String ssid = httprq.substring (pidx + 6, pidx2);
      pidx = httprq.indexOf (" HTTP/", pidx2);
      String pass = httprq.substring (pidx2 + 6, pidx);
      if ( connect_wifi(ssid.c_str(),pass.c_str() ) == 0 ) {
        strncpy(c_vars[EV_SSID], ssid.c_str(), LVARS * 2);
        strncpy(c_vars[EV_PASS], pass.c_str(), LVARS * 2);
        svf = 1;
     //   rst = 1;
      }
      else
      {
        Serial.println ("Wifi Connect failed, will try prior SSID and Password");
        if ( connect_wifi(c_vars[EV_SSID],c_vars[EV_PASS] ) == 1 )
          ESP.restart();  //Give up reboot
      } 
      
    }
    else if (httprq.indexOf ("GET /daylight/on ") != -1)
    {
      strcpy (c_vars[EV_DST], "true");
      NTP.begin (ntpsvr, String (c_vars[EV_TZ]).toInt (), toBool(String (c_vars[EV_DST])));
      httprsp += "<strong>daylight: on</strong><br>";
      svf = 1;
    }
    else if (httprq.indexOf ("GET /daylight/off ") != -1)
    {
      strcpy (c_vars[EV_DST], "false");
      NTP.begin (ntpsvr, String (c_vars[EV_TZ]).toInt (), toBool(String (c_vars[EV_DST])));
      httprsp += "<strong>daylight: off</strong><br>";
      svf = 1;
    }
    else if ((pidx = httprq.indexOf ("GET /brightness/")) != -1)
    {
      int pidx2 = httprq.indexOf (" ", pidx + 16);
      if (pidx2 != -1)
      {
        String bri = httprq.substring (pidx + 16, pidx2);
        dbri = (unsigned char)bri.toInt ();
        ntpsync = 1; //force full redraw
      }
    }
    else if ((pidx = httprq.indexOf ("GET /timezone/")) != -1)
    {
      int pidx2 = httprq.indexOf (" ", pidx + 14);
      if (pidx2 != -1)
      {
        String tz = httprq.substring (pidx + 14, pidx2);
        //strcpy (timezone, tz.c_str ());
        strcpy (c_vars[EV_TZ], tz.c_str ());
        NTP.begin (ntpsvr, String (c_vars[EV_TZ]).toInt (), toBool(String (c_vars[EV_DST])));
        httprsp += "<strong>timezone:" + tz + "</strong><br>";
        svf = 1;
      }
      else
      {
        httprsp += "<strong>!invalid timezone!</strong><br>";
      }
    }
    else if (httprq.indexOf ("GET /weather_animation/on ") != -1)
    {
      strcpy (c_vars[EV_WANI], "Y");
      httprsp += "<strong>Weather Animation: on</strong><br>";
      getWeather ();
      draw_weather_conditions ();
      svf = 1;
    }
    else if (httprq.indexOf ("GET /weather_animation/off ") != -1)
    {
      strcpy (c_vars[EV_WANI], "N");
      httprsp += "<strong>Weather Animation: off</strong><br>";
      getWeather ();
      draw_weather_conditions ();
      svf = 1;
    }
    else if (httprq.indexOf ("GET /military/on ") != -1)
    {
      strcpy (c_vars[EV_24H], "Y");
      httprsp += "<strong>Military Time: on</strong><br>";
      prevhh = -1;
      svf = 1;
    }
    else if (httprq.indexOf ("GET /military/off ") != -1)
    {
      strcpy (c_vars[EV_24H], "N");
      httprsp += "<strong>Military Time: off</strong><br>";
      prevhh = -1;
      svf = 1;
    }
    //Reset Config file
    else if (httprq.indexOf ("GET /reset_config_file ") != -1)
    {
      init_config_vars ();
      vars_write ();
      vars_read ();
      httprsp += "<strong>Config file resetted</strong><br>";
    }
    //
    httprsp += "<br>MORPH CLOCK<br>";
    httprsp += "<br>Use the following configuration links<br>";
    httprsp += "<a href='/daylight/on'>Daylight Savings on</a><br>";
    httprsp += "<a href='/daylight/off'>Daylight Savings off</a><br><br>";
    httprsp += "<a href='/military/on'>Military Time on</a><br>";
    httprsp += "<a href='/military/off'>Military Time off</a><br><br>";
    httprsp += "<a href='/weather_animation/on'>Weather Animation on</a><br>";
    httprsp += "<a href='/weather_animation/off'>Weather Animation off</a><br><br>";
    
    httprsp += "<a href='/timezone/-5'>East Coast, USA</a><br>";
    httprsp += "<a href='/timezone/-6'>Central, USA</a><br>";
    httprsp += "<a href='/timezone/-7'>Mountain, USA</a><br>";
    httprsp += "<a href='/timezone/-8'>Pacific, USA</a><br>";
    httprsp += "use /timezone/x for timezone 'x'<br>";
    
    httprsp += "<br><a href='/brightness/0'>brightness 0 (turn off display)</a><br>";
    httprsp += "use /brightness/x for display brightness 'x' from 0 (darkest) to 255 (brightest)<br>";
    
    //openweathermap.org
    httprsp += "<br>openweathermap.org API key<br>";
    httprsp += "<form action='/owm/'>" \
      "http://<input type='text' size=\"40\" name='owmkey' value='" + String(c_vars[EV_OWMK]) + "'>(hex string)<br>" \
      "<input type='submit' value='set OWM key'></form><br>";

    //geo location
     httprsp += "<br>Location: City,Country<br>";
    httprsp += "<form action='/geoloc/'>" \
      "http://<input type='text' name='geoloc' value='" + String(c_vars[EV_GEOLOC]) + "'>(e.g.: New York City,NY)<br>" \
      "<input type='submit' value='set Location'></form><br>";
 
    //OTA
    httprsp += "<br>OTA update configuration (every minute)<br>";
    httprsp += "<form action='/ota/'>" \
      "http://<input type='text' name='otaloc' value='" + String(c_vars[EV_OTA]) + "'>(ip address:port/filename)<br>" \
      "<input type='submit' value='set OTA location'></form><br>";
      
    httprsp += "<br>wifi configuration<br>";
    httprsp += "<form action='/wifi/'>" \
      "ssid:<input type='text' name='ssid'>" + String(c_vars[EV_SSID]) + "<br>" \
      "pass:<input type='text' name='pass'>" + String(c_vars[EV_PASS]) + "<br>" \
      "<input type='submit' value='set wifi'></form><br>";
      
    //Reset config file  (You probably will never need to but it's really handy for debugging)
    httprsp += "<a href='/reset_config_file'>Reset Config file to defaults</a><br>";
    
    httprsp += "Current Configuration<br>";
    httprsp += "Daylight: " + String (c_vars[EV_DST]) + "<br>";
    httprsp += "Military: " + String (c_vars[EV_24H]) + "<br>";
    httprsp += "Timezone: " + String (c_vars[EV_TZ]) + "<br>";
    httprsp += "Weather Animation: " + String (c_vars[EV_WANI]) + "<br>";
    
    httprsp += "<br><a href='/'>home</a><br>";
    httprsp += "<br>" \
      "<script language='javascript'>" \
      "var today = new Date();" \
      "var hh = today.getHours();" \
      "var mm = today.getMinutes();" \
      "if(hh<10)hh='0'+hh;" \
      "if(mm<59)mm=1+mm;" \
      "if(mm<10)mm='0'+mm;" \
      "var dd = today.getDate();" \
      "var MM = today.getMonth()+1;" \
      "if(dd<10)dd='0'+dd;" \
      "if(MM<10)MM='0'+MM;" \
      "var yyyy = today.getFullYear();" \
      "document.write('set date and time to <a href=/datetime/'+yyyy+MM+dd+hh+mm+'>'+yyyy+'.'+MM+'.'+dd+' '+hh+':'+mm+':00 (next minute, disable wifi)</a><br>');" \
      "document.write('using current date and time '+today);" \
      "</script>";
    httprsp += "</html>\r\n";
    httpcli.flush (); //clear previous info in the stream
    httpcli.print (httprsp); // Send the response to the client
    delay (1);
    //save settings?
    if (svf)
    {
      if (vars_write () > 0)
        Serial.println ("Variables stored");
      else
        Serial.println ("Variables storing failed");
    }

    if (rst)
      ESP.reset();
    //turn off wifi if in ap mode with date&time
  }
}
//
//Web server end
//

void draw_am_pm ()
{
      // this sets AM/PM display and is disabled when military time is used
      if (String (c_vars[EV_24H]) == "N")
      {
        if (hh >= 12) 
          TFDrawText (&display, String(" PM"), 42, 19, cc_time);
        else  
          TFDrawText (&display, String(" AM"), 42, 19, cc_time);
      }
  
}

void set_digit_color ()
{
    digit0.SetColor (cc_time);
    digit1.SetColor (cc_time);
    digit2.SetColor (cc_time);
    digit3.SetColor (cc_time);
    digit4.SetColor (cc_time);
    //If time is less than 10 or greater than 12 o'clock don't show leading zero
    //FYI hh is military time even when set to no
    Serial.print ("hh:");
    Serial.println (hh);
    if (String (c_vars[EV_24H]) == "N" && ( (hh > 12 && hh < 22) ))
     digit5.SetColor (cc_blk);
    else
     digit5.SetColor (cc_time);
}

//
// Main program
//
void loop()
{

  digit1.DrawColon (cc_time);
  digit3.DrawColon (cc_time);
    
	static int i = 0;
	static int last = 0;
  static int cm;
  
  //handle web server requests
  web_server ();
  
  //animations?
  cm = millis ();
  if ((cm - last) > ani_speed) // animation speed
  {
    last = cm;
    i++;
  }
  //time changes every miliseconds, we only want to draw when digits actually change.

  tnow = now (); 
  hh = hour (tnow);  
  mm = minute (tnow);
  ss = second (tnow);
  //

    if (ntpsync or (hh != prevhh))
    {
       
    ntpsync = 0;
    //
    prevss = ss;
    prevmm = mm;
    prevhh = hh;

    //we had a sync so draw without morphing
    
    //clear screen
    display_updater ();
    set_digit_color ();
    display.fillScreen (0);
    Serial.println ("Display cleared");
    
    //date and weather
    draw_date ();
    draw_am_pm ();
    draw_weather ();
    //

    //military time?
    if (hh > 12 && String (c_vars[EV_24H]) == "N")  // when not using military time
      hh -= 12;
    if (hh == 0 && String (c_vars[EV_24H]) == "N")  // this makes the first hour of the day 12a when military time isn't used.
       hh += 12;

    //

    digit0.Draw (ss % 10);
    digit1.Draw (ss / 10);
    digit2.Draw (mm % 10);
    digit3.Draw (mm / 10);
    digit4.Draw (hh % 10);
    digit5.Draw (hh / 10);
  }
  else
  {
    //seconds
    if (ss != prevss) 
    {
      
      int s0 = ss % 10;
      int s1 = ss / 10;
        if (s0 != digit0.Value ()) digit0.Morph (s0);
        if (s1 != digit1.Value ()) digit1.Morph (s1);
      prevss = ss;
      //refresh weather at 30sec in the minute
      if (ss == 30 && ((mm % weather_refresh) == 0)) {
        getWeather ();
      }
      else if ( (ss % 10) == 0) {       // Toggle display every 10 seconds between wind and humidity
        if (wind_humi == 1) {
          TFDrawText (&display,humi_lstr, wind_x, wind_y, cc_wind);
          wind_humi = 0;
        }
        else {
          TFDrawText (&display,wind_lstr, wind_x, wind_y, cc_wind);
          wind_humi = 1;
        }
      }
          
    }
    //minutes
    if (mm != prevmm)
    {
      int m0 = mm % 10;
      int m1 = mm / 10;
      if (m0 != digit2.Value ()) digit2.Morph (m0);
      if (m1 != digit3.Value ()) digit3.Morph (m1);
      prevmm = mm;
      draw_weather ();
    }
    //hours
   
    if (hh != prevhh)
    {
      display_updater ();
      set_digit_color ();
      prevhh = hh;
      
      draw_date ();
      draw_am_pm ();

      //military time?
      if (hh > 12 && String (c_vars[EV_24H]) == "N")
        hh -= 12;
      //
      if (hh == 0 && String (c_vars[EV_24H]) == "N") // this makes the first hour of the day 12a when military time isn't used.
       hh += 12;

     
      int h0 = hh % 10;
      int h1 = hh / 10;
      if (h0 != digit4.Value ()) digit4.Morph (h0);
      if (h1 != digit5.Value ()) digit5.Morph (h1);
    }//hh changed

  if ( String (c_vars[EV_WANI]) == "Y" )
    draw_animations (i);


  //set NTP sync interval as needed
  if (NTP.getInterval() < 3600 && year(now()) > 1970)
  {
    //reset the sync interval if we're already in sync
    NTP.setInterval (3600 * 24);//re-sync once a day
    Serial.println("24h Sync Enabled");
  }
  //
	delay (0);
  }
}
