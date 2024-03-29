/*
Thanks to all who made this project possible!

remix from HarryFun's great Morphing Digital Clock idea https://github.com/hwiguna/HariFun_166_Morphing_Clock
awesome build tutorial here:https://www.instructables.com/id/Morphing-Digital-Clock/

openweathermap compatibility added thanks to Imirel's remix https://github.com/lmirel/MorphingClockRemix
some bits and pieces from kolle86's MorphingClockRemixRemix https://github.com/kolle86/MorphingClockRemixRemix
small seconds by Trenck

this remix by timz3818 adds am/pm, working night mode (change brightness and colors), auto dissapearing first digit, and lower brightness icons

This remix by Gary Quiring
https://github.com/gquiring/MorphingClockQ

Update: 1/23/2024
There is an issue checking the weather that produces a HTTP 400 Bad request error after the code is running for many hours/days.  
Something internally must be overflowing and I don't know what it is.  So simple solution will be to auto reboot the ESP.  
Default weather refresh check will occur every 5 minutes (changed from 2 minutes)

Update: 2/20/2023
There was always a 5-6 second delay getting the weather.  It turns out it was not openweathermap.  The code was waiting for a newline response that never showed up
The default timeout was the issue, I lowered it to .5 seconds and it works perfectly now.  This caused the last digit in the seconds to morph incorrectly
The default interval for checking the weather is now every 2 minutes.  

Update: 2/18/2023
Added hostname: MorphClockQ  (helps insteading of using IP address for web config)
Changed the weather direction to a different color.  When the Wind is from the South the S looks like a 5
Fixed humidity if less than 10%, display issue 

3/2022
Changed NTP calls, would not compile
USA Daylight savings time won't work with NTPClientlib, it's default is EU DST, change the line below for USA default
library/NTPCLientlib/src/NTClientlib.h
#define DEFAULT_DST_ZONE        DST_ZONE_USA

Fixed Morphing for Hour change, it no longer clears the screen and starts over
Removed day/night mode
Changed some RGB colors and made default colors for each area (Wind, Weather Text, Clock, Date)
Changed Tiny font for number 1.  It looked weird with straight line always right justified
Removed leading zero from date display
Changed wind = 0 to display "CALM"
Add Weather text toggle for text or animation.  I found the animation hard to understand.  Text displays words like Cloudy
Weather Animation/text toggle can be changed from web and stored to config file
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
Added Metric/Imperial options to the web interface
Added brightness option to config file and web interface
Added Color Palettes to web and config file
Commented out OTA feature for the web interaface, the code is not excuted anywhere in the routines, not sure what it was for
=====================================================================
===  INSTALLATION INSTRUCTIONS  ===
=====================================================================
Youtube: https://youtu.be/5IvTE6gUA08

If you don't want to manually download the libraries I have them all in a zip file
https://drive.google.com/file/d/1cQjsZGft_tuw0jCCs2JDoIu5awqr7lbc/


copy paramsEDITTHISFIRST.h to params.h
Edit params.h and fill in your SSID, Password and other settings
Required libraries to compile:
AdaFruit GFX Library v1.10.4 (Install all dependancies)
PxMatrix LED Matrix Library by Dominic Buchstaller v1.3.0
Wifi Manager by TZAPU .16.0 (listed as Tablatronix)
NTPClientLib by German Martin 3.0.2 Beta
Timelib by Paul Stoffregen 1.6.1
Arduino JSON 5.13.5 (Do not install 6.x)
ESP Async UDP Not in the IDE library use the link below
https://github.com/me-no-dev/ESPAsyncUDP

From the File, Preferences menu, install this additional Link in the board
Manager URL option:
http://arduino.esp8266.com/stable/package_esp8266com_index.json

From the Tools menu, Board Manager select ESP8266 2.7.4 anything 3.0 or greater won't compile

To fix the Daylight Savings option for USA you must use an external editor
You need to edit this file: (located in your Arduino library directory)
library/NTPCLientlib/src/NTClientlib.h
#define DEFAULT_DST_ZONE        DST_ZONE_USA   //It's default is EU
======================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////
                                COMPONENTS 

P3 RGB Pixel panel HD Display 64x32 dot Matrix SMD2121 Led Module Indoor Screen Full Color Video Wall 192x96mm Message Board
https://www.aliexpress.com/item/32728985432.html

MELIFE 3pcs ESP8266 WiFi Development Module CH340 Serial Wireless Module NodeMcu Lua 4M WiFi WLAN Internet New Version Dev Board
https://www.amazon.com/dp/B09F8GCVC8

ALITOVE 5V 8A 40W AC to DC Adapter Power Supply Converter Transformer 5.5x2.5mm Plug AC 100V~240V Input
https://www.amazon.com/dp/B078RZBL8X

EDGELEC 120pcs Breadboard Jumper Wires 7.8 inch (7.8CM)
https://www.amazon.com/dp/B07GD2BWPY
///////////////////////////////////////////////////////////////////////////////////////////////////

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


//void getWeather();  What is this doing here?
// needed variables
byte hh;
byte mm;
byte ss;
byte ntpsync = 1;
//  const char ntpsvr[]   = "pool.ntp.org";
const char ntpsvr[] = "time.google.com";


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
// Set your wifi info, api key, date format, unit type, and location in params.h


//If you have more than one Morphing Clock you will need to change the hostname
const char *WiFi_hostname = "MorphClockQ";

//If you want to adjust color/brightness/position of screen objects you can do that in the following sections.
byte day_bright = 70;  //sets daytime brightness; 70 is default. values higher than this may not work.
byte dim_bright = 20;  // sets brightness for partly dimmed mode
byte nm_bright = 25;   // sets brightness for night mode

//=== SEGMENTS ===
// This section determines the position of the HH:MM ss digits onscreen with format digit#(&display, 0, x_offset, y_offset, irrelevant_color)

byte digit_offset_amount;

byte Digitsec_x = 56 + digit_offset_amount;
byte Digit_x = 62 + digit_offset_amount;

Digitsec digit0(&display, 0, Digitsec_x - 7 * 1, 14, display.color565(255, 255, 255));
Digitsec digit1(&display, 0, Digitsec_x - 7 * 2, 14, display.color565(255, 255, 255));
Digit digit2(&display, 0, Digit_x - 4 - 9 * 3, 8, display.color565(255, 255, 255));
Digit digit3(&display, 0, Digit_x - 4 - 9 * 4, 8, display.color565(255, 255, 255));
Digit digit4(&display, 0, Digit_x - 7 - 9 * 5, 8, display.color565(255, 255, 255));
Digit digit5(&display, 0, Digit_x - 7 - 9 * 6, 8, display.color565(255, 255, 255));


//=== COLORS ===
// this defines the colors used for the time, date, and wind info
// format:  display.color565 (R,G,B), with RGB values from 0 to 255
// default is set really dim (~10% of max), increase for more brightness if needed

int color_disp = display.color565(40, 40, 50);  // primary color


// some other colors
// R G B
int cc_blk = display.color565(0, 0, 0);         // black
int cc_wht = display.color565(25, 25, 25);      // white
int cc_bwht = display.color565(255, 255, 255);  // bright white
int cc_red = display.color565(50, 0, 0);        // red
int cc_bred = display.color565(255, 0, 0);      // bright red
int cc_org = display.color565(25, 10, 0);       // orange
int cc_borg = display.color565(255, 165, 0);    // bright orange
int cc_grn = display.color565(0, 45, 0);        // green
int cc_bgrn = display.color565(0, 255, 0);      // bright green
int cc_blu = display.color565(0, 0, 150);       // blue
int cc_bblu = display.color565(0, 128, 255);    // bright blue
int cc_ylw = display.color565(45, 45, 0);       // yellow
int cc_bylw = display.color565(255, 255, 0);    // bright yellow
int cc_gry = display.color565(10, 10, 10);      // gray
int cc_bgry = display.color565(128, 128, 128);  // bright gray
int cc_dgr = display.color565(3, 3, 3);         // dark grey
int cc_cyan = display.color565(0, 30, 30);      // cyan
int cc_bcyan = display.color565(0, 255, 255);   // bright cyan
int cc_ppl = display.color565(25, 0, 25);       // purple
int cc_bppl = display.color565(255, 0, 255);    // bright purple

// Colors for time, wind, date and weather text  (Temperature color varies based on actual temp)
int cc_time;
int cc_wind;
int cc_date;
int cc_wtext;

//===OTHER SETTINGS===
int ani_speed = 500;      // sets animation speed
int weather_refresh = 5;  // sets weather refresh interval in minutes; must be between 1 and 59
int morph_off = 0;        // display issue due to weather check

//=== POSITION ===
// to stop seeing data use "nosee" to move its position outside of the display range
byte nosee = 100;

// Set weather icon position; TF_cols = 4 by default
byte img_x = 7 * TF_COLS + 1;
byte img_y = 2;

// Date position
byte date_x = 2;
byte date_y = 26;

// Temperature position
byte tmp_x = 12 * TF_COLS;
byte tmp_y = 2;

// Wind position and label
// I'm not clear on why the first position is shifted by TF_COLS wasting space
//  byte wind_x = 1*TF_COLS;
byte wind_x = 0;
byte wind_y = 2;

// Pressure position and label
byte press_x = nosee;
byte press_y = nosee;
char press_label[] = "";

// Humidity postion and label
byte humi_x = nosee;
byte humi_y = nosee;
char humi_label[] = "%";

// Text weather condition
byte wtext_x = 5 * TF_COLS - 3;
byte wtext_y = 2;

String wind_lstr = "   ";
String humi_lstr = "   ";
int wind_humi;
String wind_direction[10] = { "  ", "N ", "NE", "E ", "SE", "S ", "SW", "W ", "NW", "N " };
String weather_text[12] = {"        "," SUNNY  ","P-CLOUDY","OVERCAST","  RAIN  ","T-STORMS","  SNOW  ","  HAZY  "," CLEAR  "," FOGGY  "," CLOUDY ","OVERCAST"};

const char server[] = "api.openweathermap.org";
WiFiClient client;
int in = -10000;
int tempMax = -10000;
int tempM = -10000;
int presM = -10000;
int humiM = -10000;
int wind_speed = -10000;
int condM = -1;  //-1 - undefined, 0 - unk, 1 - sunny, 2 - cloudy, 3 - overcast, 4 - rainy, 5 - thunders, 6 - snow
String condS = "";
int wind_nr;

//Keep count on how many times it failed to connect to weather service
int failed_connect = 0;
int failed_data = 0;
int failed_missing = 0;

WiFiServer httpsvr(80);  //Initialize the server on Port 80

//settings
#define NVARS 13
#define LVARS 40
char c_vars[NVARS][LVARS];
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
  EV_WANI,
  EV_PALET,
  EV_BRIGHT
};

// Color palette
void select_palette() {
  int x;
  x = atoi(c_vars[EV_PALET]);

  switch (x) {
    case 1:
      cc_time = cc_cyan;
      cc_wind = cc_ylw;
      cc_date = cc_grn;
      cc_wtext = cc_wht;
      break;
    case 2:
      cc_time = cc_red;
      cc_wind = cc_ylw;
      cc_date = cc_blu;
      cc_wtext = cc_grn;
      break;
    case 3:
      cc_time = cc_blu;
      cc_wind = cc_grn;
      cc_date = cc_ylw;
      cc_wtext = cc_wht;
      break;
    case 4:
      cc_time = cc_ylw;
      cc_wind = cc_cyan;
      cc_date = cc_blu;
      cc_wtext = cc_grn;
      break;
    case 5:
      cc_time = cc_bblu;
      cc_wind = cc_grn;
      cc_date = cc_ylw;
      cc_wtext = cc_grn;
      break;
    case 6:
      cc_time = cc_org;
      cc_wind = cc_red;
      cc_date = cc_grn;
      cc_wtext = cc_ylw;
      break;
    case 7:
      cc_time = cc_grn;
      cc_wind = cc_ppl;
      cc_date = cc_cyan;
      cc_wtext = cc_ylw;
      break;
    default:
      cc_time = cc_cyan;
      cc_wind = cc_ylw;
      cc_date = cc_grn;
      cc_wtext = cc_wht;
      break;
  }
}

//#ifdef ESP8266
// ISR for display refresh
void display_updater() {
  int x;
  x = atoi(c_vars[EV_BRIGHT]);
  if (x > 70 or x < 0)
    x = 70;

  display.display(x);  //set brightness
}

//#endif

bool toBool(String s) {
  return s.equals("true");
}

int vars_read() {
  //Remove file for testing when it has corrupt data
  //SPIFFS.remove("/mvars.cfg");
  //File varf = SPIFFS.open ("/mvars.cfg", "w");
  //  varf.close ();
  //return 1;

  File varf = SPIFFS.open("/mvars.cfg", "r");
  if (!varf) {
    Serial.println("Failed to open config file");
    return 0;
  }
  //read vars
  for (int i = 0; i < NVARS; i++)
    for (int j = 0; j < LVARS; j++)
      c_vars[i][j] = (char)varf.read();

  varf.close();
  show_config_vars();
  return 1;
}

int vars_write() {
  File varf = SPIFFS.open("/mvars.cfg", "w");
  if (!varf) {
    Serial.println("Failed to open config file");
    return 0;
  }
  Serial.println("Writing config file ......");
  for (int i = 0; i < NVARS; i++) {
    for (int j = 0; j < LVARS; j++) {
      if (varf.write(c_vars[i][j]) != 1)
        Serial.println("error writing var");
    }
  }
  //
  varf.close();
  return 1;
}

unsigned char h2int(char c) {
  if (c >= '0' && c <= '9') {
    return ((unsigned char)c - '0');
  }
  if (c >= 'a' && c <= 'f') {
    return ((unsigned char)c - 'a' + 10);
  }
  if (c >= 'A' && c <= 'F') {
    return ((unsigned char)c - 'A' + 10);
  }
  return (0);
}

//Strip out URL encoding
String urldecode(String str) {

  String encodedString = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == '+') {
      encodedString += ' ';
    } else if (c == '%') {
      i++;
      code0 = str.charAt(i);
      i++;
      code1 = str.charAt(i);
      c = (h2int(code0) << 4) | h2int(code1);
      encodedString += c;
    } else {

      encodedString += c;
    }

    yield();
  }

  return encodedString;
}

//Debugging
void show_config_vars() {
  Serial.println("From config file ....");

  Serial.print("SSID=");
  Serial.println(c_vars[EV_SSID]);
  Serial.print("Password=");
  Serial.println(c_vars[EV_PASS]);
  Serial.print("Timezone=");
  Serial.println(c_vars[EV_TZ]);
  Serial.print("Military=");
  Serial.println(c_vars[EV_24H]);
  Serial.print("Metric=");
  Serial.println(c_vars[EV_METRIC]);
  Serial.print("Date-Format=");
  Serial.println(c_vars[EV_DATEFMT]);
  Serial.print("apiKey=");
  Serial.println(c_vars[EV_OWMK]);
  Serial.print("Location=");
  Serial.println(c_vars[EV_GEOLOC]);
  Serial.print("DST=");
  Serial.println(c_vars[EV_DST]);
  Serial.print("Weather Animation=");
  Serial.println(c_vars[EV_WANI]);
  Serial.print("Color Palette=");
  Serial.println(c_vars[EV_PALET]);
  Serial.print("Brightness=");
  Serial.println(c_vars[EV_BRIGHT]);
}

//If the config file is not setup copy from params.h
void init_config_vars() {
  strcpy(c_vars[EV_SSID], wifi_ssid);
  strcpy(c_vars[EV_PASS], wifi_pass);
  strcpy(c_vars[EV_TZ], timezone);
  strcpy(c_vars[EV_24H], military);
  strcpy(c_vars[EV_METRIC], u_metric);
  strcpy(c_vars[EV_DATEFMT], date_fmt);
  strcpy(c_vars[EV_OWMK], apiKey);
  strcpy(c_vars[EV_GEOLOC], location);
  strcpy(c_vars[EV_DST], dst_sav);
  strcpy(c_vars[EV_WANI], w_animation);
  strcpy(c_vars[EV_PALET], c_palet);
  strcpy(c_vars[EV_BRIGHT], brightness);
}

//Wifi Connection
int connect_wifi(String n_wifi, String n_pass) {
  int c_cnt = 0;
  Serial.print("Trying WiFi Connect:");
  Serial.println(n_wifi);

  WiFi.hostname(WiFi_hostname);  //hostname not in params.h

  WiFi.begin(n_wifi, n_pass);
  WiFi.mode(WIFI_STA);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    c_cnt++;
    if (c_cnt > 50) {
      Serial.println("Wifi Connect Failed");
      return 1;
    }
  }
  Serial.println("success!");
  Serial.print("IP Address is: ");
  Serial.println(WiFi.localIP());  //
  return 0;
}

void setup() {
  Serial.begin(9600);
  while (!Serial)
    delay(500);  //delay for Leonardo
  display.begin(16);
  //     display.setDriverChip(FM6126A);
  //     display.setMuxDelay(0,1,0,0,0);

#ifdef ESP8266
  display_ticker.attach(0.002, display_updater);
#endif

  TFDrawText(&display, String("  MORPH CLOCK  "), 0, 1, cc_blu);
  TFDrawText(&display, String("  STARTING UP  "), 0, 10, cc_blu);

  // Read the config file
  if (SPIFFS.begin()) {
    Serial.println("SPIFFS Initialize....ok");
    if (!vars_read()) {
      init_config_vars();  //Copy from params.h to EV array
    }
  } else {
    Serial.println("SPIFFS Initialization...failed");
  }

  String lstr = String("TIMEZONE:") + String(c_vars[EV_TZ]);
  TFDrawText(&display, lstr, 4, 24, cc_cyan);

  show_config_vars();


  //connect to wifi network

  if (connect_wifi(c_vars[EV_SSID], c_vars[EV_PASS]) == 1) {  // Try settings in config file
    TFDrawText(&display, String("WIFI FAILED CONFIG"), 1, 10, cc_grn);
    if (connect_wifi(wifi_ssid, wifi_pass) == 1) {  // Try settings in params.h
      Serial.println("Cannot connect to anything, RESTART ESP");
      TFDrawText(&display, String("WIFI FAILED PARAMS.H"), 1, 10, cc_grn);
      resetclock();
    }
  }

  TFDrawText(&display, String("WIFI CONNECTED "), 3, 10, cc_grn);
  TFDrawText(&display, String(WiFi.localIP().toString()), 4, 17, cc_grn);

  select_palette();

  getWeather();
  if (wind_speed < 0)  //sometimes weather does not work on first attempt
   getWeather();

  httpsvr.begin();  // Start the HTTP Server

  //start NTP
  Serial.print("TimeZone for NTP.Begin:");
  Serial.println(c_vars[EV_TZ]);
  NTP.begin(ntpsvr, String(c_vars[EV_TZ]).toInt(), toBool(String(c_vars[EV_DST])));
  NTP.setInterval(10);  //force rapid sync in 10sec
  //
  NTP.onNTPSyncEvent([](NTPSyncEvent_t ntpEvent) {
    switch (ntpEvent) {
      case noResponse:
        Serial.print("Time Sync error: ");
        Serial.println("NTP server not reachable");
        break;
      case invalidAddress:
        Serial.print("Time Sync error: ");
        Serial.println("Invalid NTP server address");
        break;
      default:
        Serial.print("Got NTP time: ");
        Serial.println(NTP.getTimeDateString(NTP.getLastNTPSync()));
        ntpsync = 1;
        break;
    }
  });

  //prep screen for clock display

  display.fillScreen(0);
}

void getWeather() {
  if (!sizeof(apiKey)) {
    Serial.println("Missing API KEY for weather data, skipping");
    return;
  }
  
  if (client.connect(server, 80)) {
    client.print("GET /data/2.5/weather?q=" + String(c_vars[EV_GEOLOC]) + "&appid=" + String(c_vars[EV_OWMK]) + "&cnt=1");
    if (String(c_vars[EV_METRIC]) == "Y")
      client.println("&units=metric");
    else
      client.println("&units=imperial");

    client.println("Host: api.openweathermap.org");
    client.println("Connection: close");
    client.println();
  } else {
    Serial.println("Weather: Unable to connect");
    failed_connect++;
    if (failed_connect >= 5) {
      Serial.println("Weather: 5 failed attempts to connect.  Reset");
      resetclock();
    }
  }
//Serial.println(ESP.getFreeHeap());
//Serial.println(ESP.getMaxFreeBlockSize());

  // Sample of what the weather API sends back
  //  {"coord":{"lon":-80.1757,"lat":33.0185},"weather":[{"id":741,"main":"Fog","description":"fog","icon":"50n"},
  //  {"id":500,"main":"Rain","description":"light rain","icon":"10n"}],"base":"stations","main":{"temp":55.47,
  //  "feels_like":55.33,"temp_min":52.81,"temp_max":57.79,"pressure":1014,"humidity":98},"visibility":402,
  //  "wind":{"speed":4.61,"deg":0},"rain":{"1h":0.25},"clouds":{"all":100},
  //  "dt":1647516313,"sys":{"type":2,"id":2034311,"country":"US","sunrise":1647516506,
  //  "sunset":1647559782},"timezone":-14400,"id":4597919,"name":"Summerville","cod":200}

 
  String sval = "";
  int bT, bT2;
  failed_missing = 0;

  client.setTimeout(500);                      //We need to reduce the timeout delay for the readStringUntil
  String line = client.readStringUntil('\n');  //apparently the returned data does not have a new line so a timeout occurs

  if (!line.length()) {
    Serial.println("Weather: Unable to retrieve data");
    return;
  }

    bT = line.indexOf("\"icon\":\"");
    if (bT > 0) {
      bT2 = line.indexOf("\"", bT + 8);
      sval = line.substring(bT + 8, bT2);
      //0 - unk, 1 - sunny, 2 - cloudy, 3 - overcast, 4 - rainy, 5 - thunders, 6 - snow
      
      if (sval.equals("01d"))
        condM = 1;  //sunny
      else if (sval.equals("01n"))
        condM = 8;  //clear night
      else if (sval.equals("02d"))
        condM = 2;  //partly cloudy day
      else if (sval.equals("02n"))
        condM = 10;  //partly cloudy night
      else if (sval.equals("03d"))
        condM = 3;  //overcast day
      else if (sval.equals("03n"))
        condM = 11;  //overcast night
      else if (sval.equals("04d"))
        condM = 3;  //overcast day
      else if (sval.equals("04n"))
        condM = 11;  //overcast night
      else if (sval.equals("09d"))
        condM = 4;  //rain
      else if (sval.equals("09n"))
        condM = 4;
      else if (sval.equals("10d"))
        condM = 4;
      else if (sval.equals("10n"))
        condM = 4;
      else if (sval.equals("11d"))
        condM = 5;  //thunder
      else if (sval.equals("11n"))
        condM = 5;
      else if (sval.equals("13d"))
        condM = 6;  //snow
      else if (sval.equals("13n"))
        condM = 6;
      else if (sval.equals("50d"))
        condM = 7;  //haze (day)
      else if (sval.equals("50n"))
        condM = 9;  //fog (night)
       
      condS = sval;
    }
    //tempM
    bT = line.indexOf("\"temp\":");
    if (bT > 0) {
      bT2 = line.indexOf(",\"", bT + 7);
      sval = line.substring(bT + 7, bT2);
      tempM = sval.toInt();
    } else {
        Serial.println("temp NOT found!");
        Serial.println(line);
        failed_missing++;
        Serial.println(failed_missing);
      }

    //humiM
    bT = line.indexOf("\"humidity\":");
    if (bT > 0) {
      bT2 = line.indexOf(",\"", bT + 11);
      sval = line.substring(bT + 11, bT2);
      humiM = sval.toInt();
    } else {
      Serial.println("humidity NOT found!");
      failed_missing++;
      Serial.println(failed_missing);
      }

    //wind speed
    bT = line.indexOf("\"speed\":");
    if (bT > 0) {
      bT2 = line.indexOf(",\"", bT + 8);
      sval = line.substring(bT + 8, bT2);
      wind_speed = sval.toInt();
    } else {
      wind_speed = -10000;
      Serial.println("windspeed NOT found");
      failed_missing++;
      Serial.println(failed_missing);
    }
    //   bT = line.indexOf ("\"timezone\":");  // Timezone offset
    //   if (bT > 0)
    //   {
    //     int tz;
    //     bT2 = line.indexOf (",\"", bT + 11);
    //     sval = line.substring (bT + 11, bT2);
    //     tz = sval.toInt()/3600;
    //   }
    //   else
    //     Serial.println ("timezone offset NOT found!");

    //wind direction
    bT = line.indexOf("\"deg\":");
    if (bT > 0) {
      bT2 = line.indexOf(",\"", bT + 6);
      sval = line.substring(bT + 6, bT2);
      wind_nr = round(((sval.toInt() % 360)) / 45.0) + 1;
    } else {
      wind_nr = 0;
      Serial.println("Wind direction NOT found");
      failed_missing++;
      Serial.println(failed_missing);
    }
  
  if (failed_missing >= 4) {
    Serial.println("Weather: Most likely HTTP 400 error.  Reset");
    resetclock();
  }
}
// End of Get Weather

#include "TinyIcons.h"
#include "WeatherIcons.h"

int xo = 1, yo = 26;
char use_ani = 0;
void draw_weather_conditions() {
  //0 - unk, 1 - sunny, 2 - cloudy, 3 - overcast, 4 - rainy, 5 - thunders, 6 - snow, 7, hazy, 9 - fog

  xo = img_x;
  yo = img_y;

  if (condM == 0) {
    Serial.print("!weather condition icon unknown, show: ");
    Serial.println(condS);
    int cc_dgr = display.color565(30, 30, 30);
    //draw the first 5 letters from the unknown weather condition
    String lstr = condS.substring(0, (condS.length() > 5 ? 5 : condS.length()));
    lstr.toUpperCase();
    TFDrawText(&display, lstr, xo, yo, cc_dgr);
  } else {
    //  TFDrawText (&display, String("     "), xo, yo, 0);
  }
  xo = img_x;
  yo = img_y;
  switch (condM) {
    case 0:  //unk
      break;
    case 1:  //sunny
      DrawIcon(&display, sunny_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 2:  //cloudy
      DrawIcon(&display, cloudy_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 3:  //overcast
      DrawIcon(&display, ovrcst_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 4:  //rainy
      DrawIcon(&display, rain_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 5:  //thunders
      DrawIcon(&display, thndr_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 6:  //snow
      DrawIcon(&display, snow_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 7:  //mist
      DrawIcon(&display, mist_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 8:  //clear night
      DrawIcon(&display, moony_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 9:  //fog night
      DrawIcon(&display, mistn_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 10:  //partly cloudy night
      DrawIcon(&display, cloudyn_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
    case 11:  //cloudy night
      DrawIcon(&display, ovrcstn_ico, xo, yo, 10, 5);
      use_ani = 1;
      break;
  }
}


void draw_wind() {
  if (wind_speed < 0)
    return;
  wind_lstr = String(wind_speed);
  if (wind_speed != 0) {
    switch (wind_lstr.length()) {  //We have to pad the string to exactly 4 characters
      case 1:
        wind_lstr = String(wind_lstr) + String(" ");
        break;
    }
    TFDrawText(&display, wind_direction[wind_nr], wind_x, wind_y, cc_wht);  //Change Wind Direction color for readability
    TFDrawText(&display, wind_lstr, wind_x + 8, wind_y, cc_wind);
  } else {
    wind_lstr = String("CALM");
    TFDrawText(&display, wind_lstr, wind_x, wind_y, cc_wind);
  }
  wind_humi = 1;  //Reset switch for toggling wind or humidity display
}

//
void draw_weather() {
  int value = 0;

  if (tempM == -10000 && humiM == -10000 && presM == -10000) {
     Serial.println("No weather data available");
     failed_data++;
     if (failed_data >= 5)
       resetclock();
     return;
  }

    //-temperature
    int lcc = cc_red;
    char tmp_Metric;
    if (String(c_vars[EV_METRIC]) == "Y") {
      tmp_Metric = 'C';
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
    } else {
      tmp_Metric = 'F';
      //F
      if (tempM < 90)
        lcc = cc_grn;
      if (tempM < 75)
        lcc = cc_blu;
      if (tempM < 43)
        lcc = cc_wht;
    }

    String lstr = String(tempM) + String(tmp_Metric);

    //Padding Temp with spaces to keep position the same
    switch (lstr.length()) {
      case 2:
        lstr = String("  ") + String(lstr);
        break;
      case 3:
        lstr = String(" ") + String(lstr);
        break;
    }

    TFDrawText(&display, lstr, tmp_x, tmp_y, lcc);  // draw temperature

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

    // Pad humi to exactly 4 characters
    humi_lstr = String(humiM) + String(humi_label);
    switch (humi_lstr.length()) {
      case 2:
        humi_lstr = String(humi_lstr) + String("  ");
        break;
      case 3:
        humi_lstr = String(humi_lstr) + String(" ");
        break;
    }
    TFDrawText(&display, humi_lstr, humi_x, humi_y, lcc);  // humidity

    int cc = color_disp;
    cc = color_disp;

    //Draw wind speed and direction
    draw_wind();
    wind_humi = 1;  //Reset switch for toggling wind or humidity display

    if (String(c_vars[EV_WANI]) == "N") {
     TFDrawText(&display, weather_text[condM], wtext_x, wtext_y, cc_wtext);
    } else {
      draw_weather_conditions();
    }
}
// End display weather

void draw_date() {
  //date below the clock
  long tnow = now();
  String lstr = "";

  for (int i = 0; i < 5; i += 2) {
    switch (date_fmt[i]) {
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
    TFDrawText(&display, lstr, date_x, date_y, cc_date);
}


void draw_animations(int stp) {
  //weather icon animation
  String lstr = "";
  xo = img_x;
  yo = img_y;
  //0 - unk, 1 - sunny, 2 - cloudy, 3 - overcast, 4 - rainy, 5 - thunders, 6 - snow
  if (use_ani) {
    int *af = NULL;
    switch (condM) {
      case 1:
        af = suny_ani[stp % 5];
        break;
      case 2:
        af = clod_ani[stp % 10];
        break;
      case 3:
        af = ovct_ani[stp % 5];
        break;
      case 4:
        af = rain_ani[stp % 5];
        break;
      case 5:
        af = thun_ani[stp % 5];
        break;
      case 6:
        af = snow_ani[stp % 5];
        break;
      case 7:
        af = mist_ani[stp % 4];
        break;
      case 8:
        af = mony_ani[stp % 17];
        break;
      case 9:
        af = mistn_ani[stp % 4];
        break;
      case 10:
        af = clodn_ani[stp % 10];
        break;
      case 11:
        af = ovctn_ani[stp % 1];
        break;
    }
    //draw animation
    if (af)
      DrawIcon(&display, af, xo, yo, 10, 5);
  }
}

byte prevhh = 0;
byte prevmm = 0;
byte prevss = 0;
long tnow;
WiFiClient httpcli;


//convert hex letter to value
char hexchar2code(const char *hb) {
  if (*hb >= 'a')
    return *hb - 'a' + 10;
  if (*hb >= 'A')
    return *hb - 'A' + 10;
  return *hb - '0';
}

//To find the values they are sandwiched between search and it always ends before "HTTP /"
//Pidx + ? is length of string searching for ie "?geoloc=" = length 8, pidx + 8
//pidx2 is end of string location for HTTP /
void web_server() {
  httpcli = httpsvr.available();
  if (httpcli) {
    char svf = 0, rst = 0;
    //Read what the browser has sent into a String class and print the request to the monitor
    String httprq = httpcli.readString();
    // Looking under the hood
    // Serial.println (httprq);
    int pidx = -1;
    //
    String httprsp = "HTTP/1.1 200 OK\r\n";
    httprsp += "Content-type: text/html\r\n\r\n";
    httprsp += "<!DOCTYPE HTML>\r\n<html>\r\n";

    if ((pidx = httprq.indexOf("GET /datetime/")) != -1) {
      int pidx2 = httprq.indexOf(" ", pidx + 14);
      if (pidx2 != -1) {
        String datetime = httprq.substring(pidx + 14, pidx2);
        //display.setBrightness (bri.toInt ());
        int yy = datetime.substring(0, 4).toInt();
        int MM = datetime.substring(4, 6).toInt();
        int dd = datetime.substring(6, 8).toInt();
        int hh = datetime.substring(8, 10).toInt();
        int mm = datetime.substring(10, 12).toInt();
        int ss = 0;
        if (datetime.length() == 14) {
          ss = datetime.substring(12, 14).toInt();
        }
        //void setTime(int hr,int min,int sec,int dy, int mnth, int yr)
        setTime(hh, mm, ss, dd, MM, yy);
        ntpsync = 1;
      }
    }

    else if (httprq.indexOf("GET /ota/") != -1) {
      //GET /ota/?otaloc=192.168.2.38%3A8000%2Fespweather.bin HTTP/1.1
      pidx = httprq.indexOf("?otaloc=");
      int pidx2 = httprq.indexOf(" HTTP/", pidx);
      if (pidx2 > 0) {
        strncpy(c_vars[EV_OTA], httprq.substring(pidx + 8, pidx2).c_str(), LVARS * 3);
        //debug_print (">ota1:");
        //debug_println (c_vars[EV_OTA]);
        char *bc = c_vars[EV_OTA];
        int ck = 0;
        //debug_print (">ota2:");
        //debug_println (bc);
        //convert in place url %HH excaped chars
        while (*bc > 0 && ck < LVARS * 3) {
          if (*bc == '%') {
            //convert URL chars to ascii
            c_vars[EV_OTA][ck] = hexchar2code(bc + 1) << 4 | hexchar2code(bc + 2);
            bc += 2;
          } else
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
    else if (httprq.indexOf("GET /geoloc/") != -1) {
      pidx = httprq.indexOf("?geoloc=");
      int pidx2 = httprq.indexOf(" HTTP/", pidx);
      if (pidx2 > 0) {
        String location = urldecode(httprq.substring(pidx + 8, pidx2).c_str());
        strncpy(c_vars[EV_GEOLOC], location.c_str(), LVARS * 3);
        getWeather();
        draw_weather_conditions();
        svf = 1;
      }
    }
    //openweathermap.org key
    else if (httprq.indexOf("GET /owm/") != -1) {
      pidx = httprq.indexOf("?owmkey=");
      int pidx2 = httprq.indexOf(" HTTP/", pidx);
      if (pidx2 > 0) {
        strncpy(c_vars[EV_OWMK], httprq.substring(pidx + 8, pidx2).c_str(), LVARS * 3);
        getWeather();
        draw_weather_conditions();
        svf = 1;
      }
      //
    } else if (httprq.indexOf("GET /wifi/") != -1) {
      //GET /wifi/?ssid=ssid&pass=pass HTTP/1.1
      pidx = httprq.indexOf("?ssid=");
      int pidx2 = httprq.indexOf("&pass=");
      String ssid = httprq.substring(pidx + 6, pidx2);
      pidx = httprq.indexOf(" HTTP/", pidx2);
      String pass = httprq.substring(pidx2 + 6, pidx);
      if (connect_wifi(ssid.c_str(), pass.c_str()) == 0) {
        strncpy(c_vars[EV_SSID], ssid.c_str(), LVARS * 2);
        strncpy(c_vars[EV_PASS], pass.c_str(), LVARS * 2);
        svf = 1;
        //   rst = 1;
      } else {
        Serial.println("Wifi Connect failed, will try prior SSID and Password");
        if (connect_wifi(c_vars[EV_SSID], c_vars[EV_PASS]) == 1)
          ESP.restart();  //Give up reboot
      }

    } else if (httprq.indexOf("GET /daylight/on ") != -1) {
      strcpy(c_vars[EV_DST], "true");
      NTP.begin(ntpsvr, String(c_vars[EV_TZ]).toInt(), toBool(String(c_vars[EV_DST])));
      httprsp += "<strong>daylight: on</strong><br>";
      svf = 1;
    } else if (httprq.indexOf("GET /daylight/off ") != -1) {
      strcpy(c_vars[EV_DST], "false");
      NTP.begin(ntpsvr, String(c_vars[EV_TZ]).toInt(), toBool(String(c_vars[EV_DST])));
      httprsp += "<strong>daylight: off</strong><br>";
      svf = 1;
    } else if (httprq.indexOf("GET /metric/on ") != -1) {
      strcpy(c_vars[EV_METRIC], "Y");
      httprsp += "<strong>metric: on</strong><br>";
      getWeather();
      draw_weather_conditions();
      svf = 1;
    } else if (httprq.indexOf("GET /metric/off ") != -1) {
      strcpy(c_vars[EV_METRIC], "N");
      httprsp += "<strong>metric: off</strong><br>";
      getWeather();
      draw_weather_conditions();
      svf = 1;
    } else if ((pidx = httprq.indexOf("GET /brightness/")) != -1) {
      int pidx2 = httprq.indexOf(" ", pidx + 16);
      if (pidx2 != -1) {
        String bri = httprq.substring(pidx + 16, pidx2);
        strcpy(c_vars[EV_BRIGHT], bri.c_str());
        display_updater();
        ntpsync = 1;  //force full redraw
        svf = 1;
      }
    } else if ((pidx = httprq.indexOf("GET /timezone/")) != -1) {
      int pidx2 = httprq.indexOf(" ", pidx + 14);
      if (pidx2 != -1) {
        String tz = httprq.substring(pidx + 14, pidx2);
        strcpy(c_vars[EV_TZ], tz.c_str());
        NTP.begin(ntpsvr, String(c_vars[EV_TZ]).toInt(), toBool(String(c_vars[EV_DST])));
        httprsp += "<strong>timezone:" + tz + "</strong><br>";
        svf = 1;
      } else {
        httprsp += "<strong>!invalid timezone!</strong><br>";
      }
    } else if (httprq.indexOf("GET /weather_animation/on ") != -1) {
      strcpy(c_vars[EV_WANI], "Y");
      httprsp += "<strong>Weather Animation: on</strong><br>";
      TFDrawText(&display, "        ", wtext_x, wtext_y, 0);
      getWeather();
      draw_weather_conditions();
      ntpsync = 1;
      svf = 1;
    } else if (httprq.indexOf("GET /weather_animation/off ") != -1) {
      strcpy(c_vars[EV_WANI], "N");
      httprsp += "<strong>Weather Animation: off</strong><br>";
      getWeather();
      draw_weather_conditions();
      ntpsync = 1;
      svf = 1;
    } else if (httprq.indexOf("GET /military/on ") != -1) {
      strcpy(c_vars[EV_24H], "Y");
      httprsp += "<strong>Military Time: on</strong><br>";
      prevhh = -1;
      svf = 1;
    } else if (httprq.indexOf("GET /military/off ") != -1) {
      strcpy(c_vars[EV_24H], "N");
      httprsp += "<strong>Military Time: off</strong><br>";
      prevhh = -1;
      svf = 1;
    }
    //Reset Config file
    else if (httprq.indexOf("GET /reset_config_file ") != -1) {
      init_config_vars();
      vars_write();
      vars_read();
      httprsp += "<strong>Config file resetted</strong><br>";
    } else if ((pidx = httprq.indexOf("GET /colorpalet/")) != -1) {
      int pidx2 = httprq.indexOf(" ", pidx + 16);
      if (pidx2 != -1) {
        String pal = httprq.substring(pidx + 16, pidx2);
        strcpy(c_vars[EV_PALET], pal.c_str());
        httprsp += "<strong>Color Palet:" + pal + "</strong><br>";
        svf = 1;
        rst = 1;
      }
    }

    //
    httprsp += "<br>MORPH CLOCK CONFIG<br>";
    httprsp += "<br>Use the following configuration links<br>";
    httprsp += "<a href='/daylight/on'>Daylight Savings on</a>&nbsp &nbsp &nbsp";
    httprsp += "<a href='/daylight/off'>Daylight Savings off</a><br><br>";
    httprsp += "<a href='/military/on'>Military Time on</a>&nbsp &nbsp &nbsp";
    httprsp += "<a href='/military/off'>Military Time off</a><br><br>";
    httprsp += "<a href='/metric/on'>Metric System</a>&nbsp &nbsp &nbsp";
    httprsp += "<a href='/metric/off'>Imperial System</a><br><br>";
    httprsp += "<a href='/weather_animation/on'>Weather Animation on</a>&nbsp &nbsp &nbsp";
    httprsp += "<a href='/weather_animation/off'>Weather Animation off</a><br><br>";

    httprsp += "<a href='/timezone/-5'>East Coast USA</a>&nbsp &nbsp &nbsp";
    httprsp += "<a href='/timezone/-6'>Central USA</a>&nbsp &nbsp &nbsp";
    httprsp += "<a href='/timezone/-7'>Mountain USA</a>&nbsp &nbsp &nbsp";
    httprsp += "<a href='/timezone/-8'>Pacific USA</a><br>";
    httprsp += "use /timezone/x for timezone 'x'<br><br>";

    httprsp += "<a href='/colorpalet/1'>Clock Color Cyan</a>&nbsp &nbsp &nbsp";
    httprsp += "<a href='/colorpalet/2'>Clock Color Red</a>&nbsp &nbsp &nbsp";
    httprsp += "<a href='/colorpalet/3'>Clock Color Blue</a>&nbsp &nbsp &nbsp<br>";
    httprsp += "<a href='/colorpalet/4'>Clock Color Yellow</a>&nbsp &nbsp &nbsp";
    httprsp += "<a href='/colorpalet/5'>Clock Color Bright Blue</a>&nbsp &nbsp &nbsp";
    httprsp += "<a href='/colorpalet/6'>Clock Color Orange</a>&nbsp &nbsp &nbsp";
    httprsp += "<a href='/colorpalet/7'>Clock Color Green</a>&nbsp &nbsp &nbsp<br><br>";

    httprsp += "<a href='/brightness/70'>Brightness 70</a>&nbsp &nbsp &nbsp";
    httprsp += "<a href='/brightness/35'>Brightness 35</a>&nbsp &nbsp &nbsp";
    httprsp += "<a href='/brightness/0'>Turn off display</a><br>";
    httprsp += "Use /brightness/x for display brightness 'x'<br>";

    //openweathermap.org
    httprsp += "<br>openweathermap.org API key<br>";
    httprsp += "<form action='/owm/'>"
               "http://<input type='text' size=\"35\" name='owmkey' value='"
               + String(c_vars[EV_OWMK]) + "'>(hex string)<br>"
                                           "<input type='submit' value='set OWM key'></form><br>";

    //geo location
    httprsp += "<br>Location: City,Country<br>";
    httprsp += "<form action='/geoloc/'>"
               "http://<input type='text' name='geoloc' value='"
               + String(c_vars[EV_GEOLOC]) + "'>(e.g.: New York City,NY)<br>"
                                             "<input type='submit' value='set Location'></form><br>";

    //GQ
    //I have no idea what someone intended for this to do?
    //EV_OTA is not accessed in the code for any routines
    //I left the vars in place but will comment out in the web interface since it does nothing
    //
    //OTA
    //    httprsp += "<br>OTA update configuration (every minute)<br>";
    //    httprsp += "<form action='/ota/'>" \
//      "http://<input type='text' name='otaloc' value='" + String(c_vars[EV_OTA]) + "'>(ip address:port/filename)<br>" \
//      "<input type='submit' value='set OTA location'></form><br>";

    httprsp += "<br>wifi configuration<br>";
    httprsp += "<form action='/wifi/'>"
               "ssid:<input type='text' name='ssid'>"
               + String(c_vars[EV_SSID]) + "<br>"
                                           "pass:<input type='text' name='pass'>"
               + String(c_vars[EV_PASS]) + "<br>"
                                           "<input type='submit' value='set wifi'></form><br>";

   
    //Reset config file  (You probably will never need to but it's really handy for debugging)
    httprsp += "<a href='/reset_config_file'>Reset Config file to defaults</a><br><br>";

    httprsp += "Current Configuration<br>";
    httprsp += "Daylight: " + String(c_vars[EV_DST]) + "<br>";
    httprsp += "Military: " + String(c_vars[EV_24H]) + "<br>";
    httprsp += "Metric: " + String(c_vars[EV_METRIC]) + "<br>";
    httprsp += "Timezone: " + String(c_vars[EV_TZ]) + "<br>";
    httprsp += "Weather Animation: " + String(c_vars[EV_WANI]) + "<br>";
    httprsp += "Color palette: " + String(c_vars[EV_PALET]) + "<br>";
    httprsp += "Brightness: " + String(c_vars[EV_BRIGHT]) + "<br>";
    httprsp += "<br><a href='/'>home</a><br>";
    httprsp += "<br>"
               "<script language='javascript'>"
               "var today = new Date();"
               "var hh = today.getHours();"
               "var mm = today.getMinutes();"
               "if(hh<10)hh='0'+hh;"
               "if(mm<59)mm=1+mm;"
               "if(mm<10)mm='0'+mm;"
               "var dd = today.getDate();"
               "var MM = today.getMonth()+1;"
               "if(dd<10)dd='0'+dd;"
               "if(MM<10)MM='0'+MM;"
               "var yyyy = today.getFullYear();"
               "document.write('set date and time to <a href=/datetime/'+yyyy+MM+dd+hh+mm+'>'+yyyy+'.'+MM+'.'+dd+' '+hh+':'+mm+':00</a><br>');"
               "document.write('using current date and time '+today);"
               "</script>";
    httprsp += "</html>\r\n";
    httpcli.flush();         //clear previous info in the stream
    httpcli.print(httprsp);  // Send the response to the client
    delay(1);
    //save settings?
    if (svf) {
      if (vars_write() > 0)
        Serial.println("Variables stored");
      else
        Serial.println("Variables storing failed");
    }

    if (rst)
      resetclock();
  }
}
//
//Web server end
//

//Restart Clock
void resetclock() {
  display.fillScreen(0);
  TFDrawText(&display, String("  RESTART  "), 10, 9, cc_blu);
  TFDrawText(&display, String("MORPH CLOCK"), 10, 16, cc_blu);
  delay(2000);
  ESP.reset();
}

void draw_am_pm() {
  // this sets AM/PM display and is disabled when military time is used
  if (String(c_vars[EV_24H]) == "N") {
    if (hh >= 12)
      TFDrawText(&display, String(" PM"), 42, 19, cc_time);
    else
      TFDrawText(&display, String(" AM"), 42, 19, cc_time);
  }
}

void set_digit_color() {
  digit0.SetColor(cc_time);
  digit1.SetColor(cc_time);
  digit2.SetColor(cc_time);
  digit3.SetColor(cc_time);
  digit4.SetColor(cc_time);

  // Don't print leading zero if not Military
  if (String(c_vars[EV_24H]) == "N" && hh < 10)
    digit5.SetColor(cc_blk);
  else
    digit5.SetColor(cc_time);
}

//
// Main program
//
void loop() {

  digit1.DrawColon(cc_time);
  digit3.DrawColon(cc_time);

  static int i = 0;
  static int last = 0;
  static int cm;

  //handle web server requests
  web_server();

  //animations?
  cm = millis();
  if ((cm - last) > ani_speed)  // animation speed
  {
    last = cm;
    i++;
  }
  //time changes every miliseconds, we only want to draw when digits actually change.

  tnow = now();
  hh = hour(tnow);
  mm = minute(tnow);
  ss = second(tnow);
  //

  //GQGQ    if (ntpsync or (hh != prevhh))   Fixed morphing bug that required refreshing the screen for hh change
  if (ntpsync) {

    ntpsync = 0;
    //
    prevss = ss;
    prevmm = mm;
    prevhh = hh;

    //we had a sync so draw without morphing

    //clear screen
    display_updater();
    display.fillScreen(0);
    Serial.println("Display cleared");

    //date and weather
    draw_date();
    draw_am_pm();
    draw_weather();
    //

    //military time?
    if (hh > 12 && String(c_vars[EV_24H]) == "N")  // when not using military time
      hh -= 12;
    if (hh == 0 && String(c_vars[EV_24H]) == "N")  // this makes the first hour of the day 12a when military time isn't used.
      hh += 12;

    set_digit_color();

    digit0.Draw(ss % 10);
    digit1.Draw(ss / 10);
    digit2.Draw(mm % 10);
    digit3.Draw(mm / 10);
    digit4.Draw(hh % 10);
    digit5.Draw(hh / 10);
  } else {
    //seconds
    if (ss != prevss) {

      int s0 = ss % 10;
      int s1 = ss / 10;
      set_digit_color;

      //There is a delay problem when checking the weather site which causes the morph to scramble the digit
      //We must force no morphing to make sure the digit is displayed correctly

      if (morph_off == 0) {
        if (s0 != digit0.Value()) digit0.Morph(s0);
      } else {
        digit0.SetColor(cc_blk);                          //This else block is currently not used
        digit0.Draw(8);  //Turn off all segments to black
        morph_off = 0;
        digit0.SetColor(cc_time);
        digit0.Draw(s0);
      }
      if (s1 != digit1.Value()) digit1.Morph(s1);

      prevss = ss;
      //refresh weather at 31sec in the minute
      if (ss == 31 && ((mm % weather_refresh) == 0)) {
        getWeather();
        morph_off = 0;  //Currently not using, set to 0 this was to address weather delay
      } else if ((ss % 10) == 0) {  // Toggle display every 10 seconds between wind and humidity
        if (wind_humi == 1) {
          TFDrawText(&display, humi_lstr, wind_x, wind_y, cc_wind);
          wind_humi = 0;
        } else {
          draw_wind();
          wind_humi = 1;
        }
      }
    }
    //minutes
    if (mm != prevmm) {
      int m0 = mm % 10;
      int m1 = mm / 10;
      if (m0 != digit2.Value()) digit2.Morph(m0);
      if (m1 != digit3.Value()) digit3.Morph(m1);
      prevmm = mm;
      draw_weather();
    }
    //hours

    if (hh != prevhh) {
      display_updater();
      prevhh = hh;

      draw_date();
      draw_am_pm();

      //military time?
      if (hh > 12 && String(c_vars[EV_24H]) == "N")
        hh -= 12;
      //
      if (hh == 0 && String(c_vars[EV_24H]) == "N")  // this makes the first hour of the day 12a when military time isn't used.
        hh += 12;


      int h0 = hh % 10;
      int h1 = hh / 10;
      set_digit_color();

      digit4.Morph(h0);

      if (String(c_vars[EV_24H]) == "N" && hh < 10)  //We have to clear leading zero to black
        digit5.Draw(h1);

      if (h1 != digit5.Value()) digit5.Morph(h1);
    }  //hh changed

    if (String(c_vars[EV_WANI]) == "Y")
      draw_animations(i);


    //set NTP sync interval as needed
    if (NTP.getInterval() < 3600 && year(now()) > 1970) {
      //reset the sync interval if we're already in sync
      NTP.setInterval(3600 * 24);  //re-sync once a day
      Serial.println("24h Sync Enabled");
    }
    //
    //delay (0);
  }
}
