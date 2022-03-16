Make sure from the board manager you select is 2.7.x for the ESP8266, any 3.x version will not compile.

Visit [OpenWeather](https://openweathermap.org/) to get a free account for your weather API key.  This is mandatory for the weather to work.  

Edit the params.h file (which needs to be renamed).  You must setup your SSID, password, timezone, weather api key and several other options.  

This version of Hari's Morphing clock is from several remix versions including timz3818. They all did a nice job enhancing this cool clock - Thank you!

I found the issue with daylight savings time not working for USA locations.  It requires editing the NTPClientLib (more details in the .ino).  

Added option to display weather text ie - Cloudy instead of using icons, I found the icons confusing to decipher.  It's on toggle, you can choose what you like.

Replaced 0 wind with CALM display.

Modifed the Tiny font for the 1 digit.  It was a straight line down and looked weird.

Fixed the config file getting corrupted.  It was the declarations for the location and apikey. The are now char arrays. 

Leading zero removed from the date display.

Modifed the NTP code to actually compile, the version I had refused to compile with any version of NTPClientLib I tried

Added more configuration options to the web interface.

Added urldecode function to remove the special characters from web entries.  They were being written to the config file which caused issues.  
