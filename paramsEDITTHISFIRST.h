//wifi network
char wifi_ssid[] = "";
//wifi password
char wifi_pass[] = "";
//timezone
char timezone[5] = "-5" ;
//use 24h time format
char military[3] = "N";     // 24 hour mode? Y/N
//use metric data
char u_metric[3] = "N";     // use metric for units? Y/N
//date format
char date_fmt[7] = "M.D.Y"; // date format: D.M.Y or M.D.Y or M.D or D.M or D/M/Y.. looking for trouble
//open weather map api key 
//To Get your API Key - https://openweathermap.org/
char apiKey[]   = ""; //e.g a hex string like "abcdef0123456789abcdef0123456789"
//the city you want the weather for 
char location[] = ""; //e.g. "Paris,FR"
//Daylight savings time
char dst_sav[6] = "true";  
// Weather animation graphics or text? Y/N
char w_animation[3] = "N";
// Color palette
char c_palet[3] = "1";
