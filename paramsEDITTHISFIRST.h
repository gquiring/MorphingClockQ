//wifi network
char wifi_ssid[] = "";
//wifi password
char wifi_pass[] = "";
//timezone
char timezone[] = "-5" ; // in theory, this is set by the location in openweathermap
//use 24h time format
char military[] = "N";     // 24 hour mode? Y/N
//use metric data
char u_metric[] = "N";     // use metric for units? Y/N
//date format
char date_fmt[] = "M.D.Y"; // date format: D.M.Y or M.D.Y or M.D or D.M or D/M/Y.. looking for trouble
//open weather map api key 
String apiKey   = ""; //e.g a hex string like "abcdef0123456789abcdef0123456789"
//the city you want the weather for 
String location = "New York, US        "; //e.g. "Paris,FR"
//Daylight savings time
char dst_sav[] = "true";  
// Weather animation graphics or text
char w_animation[] = "N";
