Make sure from the board manager you select is 2.7.x for the ESP8266, any 3.x version will not compile.

I made a zip file with all the libraries required for this project on [Google Drive](https://drive.google.com/file/d/1cQjsZGft_tuw0jCCs2JDoIu5awqr7lbc/)

Visit [OpenWeather](https://openweathermap.org/) to get a free account for your weather API key.  This is mandatory for the weather to work.  

Edit the paramsEDITMEFIRST.h file (which needs to be renamed to params.h).  You must setup your SSID and password.  

Daylight savings time not working for USA locations.  It requires editing the NTPClientLib (more details in the .ino).  

Fixed the morphing problem when the hour changed, it now morphs all the digits, much cooler looking.

Added/modified the following features:

Display weather text ie - Cloudy instead of using icons, I found the icons confusing to decipher.  It's on toggle, you can choose what you like.

Show wind and humidity alternately displayed every 10 seconds.

Day of week to the dateline.  FYI it's hardcoded for Sun being the first day of week.

Replaced 0 wind with CALM display.

Color palette options.  You can pick from several different palettes on the web interface.

Leading zero removed from the date display.

Modifed the Tiny font for the 1 digit.  It was a straight line down and looked weird.

Added urldecode function to remove the special characters from web entries.  They were being written to the config file which caused issues.

Lots of new options on the web interface.

Fixed the config file getting corrupted.  Overall this area needed a lot of work, the code was not referencing the correct variables.   

Modifed the NTP code in the main loop to compile, the version I had refused to compile with any version of NTPClientLib I tried.

Credits:
This version of Hari's Morphing clock is from several remix versions including timz3818. They all did a nice job enhancing this cool clock - Thank you!

