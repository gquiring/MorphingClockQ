//Rename this file to params.h!!!!!!!!!!!!!!!!!
//You must edit these fields for your SSID and Password
//Weather requires your location to work (Postal or lat/long depending on the service)
//Change the values inside the ""

//wifi network settings
char wifi_ssid[] = "";        //SSID
char wifi_pass[] = "";        //Password

//Time settings
char timezone[] = "-5" ;   //Timezone 
char military[] = "N";     // 24 hour mode? Y/N
char date_fmt[] = "M.D.Y"; // date format: D.M.Y or M.D.Y or M.D or D.M or D/M/Y
char dst_sav[] = "true";   //Daylight savings time

// RGB Maxtix settings
char c_palet[] = "1";      //Color palette
char brightness[] = "60";  //Brightness  (Don't go over 70)

char nightStart[] = "23";    //Set the hour using 24 hr format for the brightness to dim mode
char nightEnd[] = "7";       //set the hour using 24 hr format for returning clock to daytime brightness

// Weather configuration

// Set weather service (Default is 4 - Open Meteo, it does not require an API key or account setup)
//
// 1 = WeatherApi.com  (Supports Postal code)  Requires an account setup please read complete weather docs for details
// 2 = WeatherBit.io   (Supports Postal code)
// 3 = PirateWeather   (Must use latitude/logtitude)
// 4 = Open-meteo      (Must use latitude/logtitude)
// 5 = WeatherUnlocked (Supports Postal code, must setup country code) requires 2 API keys read weather docs for details

// For the most accurate location of your weather I recommend using the the latitude/longtitude
// Here is one web site to use, many others are also available
// https://www.latlong.net/

// If you entered a postal code and latitude/lontitude settings, postal will be ignored

// WeatherAPI needs addtional setup per account
// Carefully check all the boxes below from this link
// https://www.weatherapi.com/my/fields.aspx
// Boxes to to check for Current weather: temp_c, temp_f, is_day, text, icon, code, wind_mph, wind_kph, wind_degree, wind_dir, humidity, cloud

//WeatherUnlocked has 2 apikeys
//Place the app key in the wdefine field


//Weather sites for account setups/API key
// 1 https://www.weatherapi.com/
// 2 https://www.weatherbit.io/
// 3 https://pirateweather.net/
// 4 https://open-meteo.com/  (nothing is required for setup)
// 5 http://www.weatherunlocked.com/

char weatherservice[] = "4";         //4 is the default, I recommend using this one, no account is required
char apiKey[]         = "";          //API key from weather service
char postal_code[]    = "";          //Postal or Zip code
char country_code[]   = "";          //Country code

char latitude[]       = "";          //Some weather services require latitude
char longitude[]      = "";          //Some weather services require longitude

char u_metric[] = "N";              // Use Farenheit or Celcius for temp/wind
char w_animation[] = "N";           // Weather animation graphics or text? Y/N
char wdefine[] = "";                // Weather definable var based on weather service

//If you want to try multiple weather services to see which ones gives you the most accurate data
//you can setup your API Keys in the array.  It will override the web config setup when you switch services
//Leave the 4th array as "" since Meteo does not require a key
String apikeys[8] = {"",    //WeatherAPI
                     "",    //WeatherBit
                     "",    //PirateWeather
                     "",    //OpenMeteo - not required
                     "",    //WeatherUnlocked
                     "",    //future weather service
                     ""};   //Accuweather (Currently not working)









