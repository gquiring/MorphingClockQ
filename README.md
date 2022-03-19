Make sure from the board manager you select is 2.7.x for the ESP8266, any 3.x version will not compile.

Visit [OpenWeather](https://openweathermap.org/) to get a free account for your weather API key.  This is mandatory for the weather to work.  

Edit the params.h file (which needs to be renamed).  You must setup your SSID, password, timezone, weather api key and several other options.  

This version of Hari's Morphing clock is from several remix versions including timz3818. They all did a nice job enhancing this cool clock - Thank you!

I found the issue with daylight savings time not working for USA locations.  It requires editing the NTPClientLib (more details in the .ino).  

Fixed the morphing problem when the hour changed, it now morphs all the digits, much cooler looking

Added option to display weather text ie - Cloudy instead of using icons, I found the icons confusing to decipher.  It's on toggle, you can choose what you like.

Wind and humidity will be alternately displayed every 10 seconds.

Added day of week to the dateline.  Right now it's hardcoded for Sun being the first day of week.

Replaced 0 wind with CALM display.

Leading zero removed from the date display.

Modifed the Tiny font for the 1 digit.  It was a straight line down and looked weird.

Fixed the config file getting corrupted. 

Modifed the NTP code to actually compile, the version I had refused to compile with any version of NTPClientLib I tried

Added more configuration options to the web interface.

Added urldecode function to remove the special characters from web entries.  They were being written to the config file which caused issues.

Fixed changing SSID and Password on the web interface.

Added color palette options to the web config.  You can pick from several different palettes.  


