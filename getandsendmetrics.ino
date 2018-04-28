#include <DHT.h>
#include <ESP8266WiFi.h>
#include <OneWire.h>
 
// replace with your channel’s thingspeak API key and your SSID and password
String apiKey = "INSERTAPIKEY";
const char* ssid = "INSERTSSID";
const char* password = "INSERTPWD";
const char* server = "api.thingspeak.com";
int connecttimes = 0;
 
#define DHTPIN D2
#define DHTTYPE DHT11 
 
DHT dht(DHTPIN, DHTTYPE);
WiFiClient client;

OneWire  ds(D6);

void setup() 
{
  Serial.begin(115200);
  delay(10);
  dht.begin();
  
  WiFi.begin(ssid, password);
 
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
 
  WiFi.begin(ssid, password); 
  connecttimes = 0;
  while (WiFi.status() != WL_CONNECTED && connecttimes < 120 ) 
  {
    delay(500);
    Serial.print(".");
    connecttimes++;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Cannot connect to WiFi, resetting...");
    delay(3000);
    WiFi.disconnect();
    ESP.restart(); 
    delay(3000);
  }
  Serial.println("");
  Serial.println("WiFi connected");
}


float getTemp(void) 
{
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  
 ds.reset_search();
 
  if ( !ds.search(addr)) 
  {
    ds.reset_search();
    delay(250);
    return -200;
  }
 
 
  if (OneWire::crc8(addr, 7) != addr[7]) 
  {
      Serial.println("CRC is not valid!");
      return -200;
  }
  Serial.println();
 
  // the first ROM byte indicates which chip
  switch (addr[0]) 
  {
    case 0x10:
      type_s = 1;
      break;
    case 0x28:
      type_s = 0;
      break;
    case 0x22:
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
      return -200;
  } 
 
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end  
  delay(1000);
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad
 
  for ( i = 0; i < 9; i++) 
  {           
    data[i] = ds.read();
  }
 
  // Convert the data to actual temperature
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) 
    {
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } 
  else 
  {
    byte cfg = (data[4] & 0x60);
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
 
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;
  Serial.print("  Temperature = ");
  Serial.print(celsius);
  Serial.print(" Celsius, ");
  Serial.print(fahrenheit);
  Serial.println(" Fahrenheit");
  return celsius;
}
 
void loop() 
{
 
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    float tsonda = getTemp();
    if (isnan(h) || isnan(t)) 
    {
      Serial.println("Failed to read from DHT sensor!");
    }
    
    if (client.connect(server,80)) {
        String postStr = apiKey;
        if (!isnan(t)) {
          postStr +="&field1=";
          postStr += String(t);
        }
        if (!isnan(h)) {
          postStr +="&field2=";
          postStr += String(h);
        }
        if (tsonda != -200) {
          postStr +="&field3=";
          postStr += String(tsonda);
        }
        postStr += "\r\n\r\n";
         
        client.print("POST /update.json HTTP/1.1\n");
        client.print("Host: api.thingspeak.com\n");
        client.print("Connection: close\n");
        client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n");
        client.print("Content-Type: application/x-www-form-urlencoded\n");
        client.print("Content-Length: ");
        client.print(postStr.length());
        client.print("\n\n");
        client.print(postStr);
         
        Serial.print("Temperature: ");
        Serial.print(t);
        Serial.print(" degrees Celsius Humidity: ");
        Serial.print(h);
        Serial.print(" Temp Sonda: ");
        Serial.print(tsonda);        
        Serial.println(" Sending data to Thingspeak");
    }
    client.stop();
     
    Serial.println("Waiting 1 min");
    delay(60000);
}

