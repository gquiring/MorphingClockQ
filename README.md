This is a remix of Harry's Morphing Clock that includes weather like temperature, wind and humidity.  The project requires an ESP8266.  All other hardware required is in the .ino source.  

If you already have the Morphing Clock installed prior to May 2024 you must wipe your flash memory.  From the Tools menu, select Erase Flash-All flash settings.  The config file has changed requiring this one time only to wipe it. This new version also uses new libraries.  The Google Drive link is in the ino source.  

Please note when installing the board manager for the ESP8266 you must install the old version 2.7.4.  Newer versions will NOT compile.

Due to the change with OpenWeatherMap requiring a new API call and your account must have a credit card on file (starting June 2024) I have desided to discontinue using their service.  The weather code has been completely rewritten to be more flexible with multiple weather services.  It currently supports WeatherAPI, WeatherBit.io, PirateWeather, OpenMeteo and WeatherUnlocked.  I can add more weather services as long as they don't require a SSL connection and the JSON string is not huge.  

Edit the paramsEDITMEFIRST.h file (which needs to be renamed to params.h). You must setup your SSID, password and weather location.

I highly recommend starting with weather service OpenMeteo which is service 4.  This service requires no account or API key.  It's the easiest way to get your weather setup.  Depending on the weather service you use your location can be zip/postal code or lat/long.  

Daylight savings time not working for USA locations. It requires editing the NTPClientLib (more details in the .ino).

New feature added - Day / Night mode.

The web page has been entirely redone using the Async library.  It's much easier to main and modify the HTML. 

Instead of searching for all the libraries I made a Zip file on Google Drive for an easier install.  The link is in the .ino source.  

Hardware instuctions are on youtube:
https://youtu.be/5IvTE6gUA08

Software only instuctions (IDE, libraries, params.h)
https://youtu.be/_MmN9ZMG3VI


Credits:
This version of Hari's Morphing clock is from several remix versions including timz3818. They all did a nice job enhancing this cool clock - Thank you!

![Alt text](https://github.com/gquiring/MorphingClockQ/blob/main/morphclock.png?raw=true "Optional Title")
