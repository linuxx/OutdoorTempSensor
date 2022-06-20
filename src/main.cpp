#include <Arduino.h>

//dht
#include <Adafruit_Sensor.h>
#include <DHT.h>
//wifi
#include <WiFiManager.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h> 
//mqtt
#include <PubSubClient.h>
//file save
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

//save spot
#define ESP_DRD_USE_SPIFFS true
#define JSON_CONFIG_FILE "/sample_config.json"

//setup the wifi manager
WiFiManager wifiManager;

//global configs
char strServer[50] = "";
char strUser[50] = "";
char strPass[50] = "";
char strPort[50] = "";
char strTopic[50] = "";

//mqtt configs
WiFiManagerParameter mqtt_server("mqtt_server", "MQTT Server", "", 50);
WiFiManagerParameter mqtt_user("mqtt_user", "MQTT Username", "", 50);
WiFiManagerParameter mqtt_pass("mqtt_pass", "MQTT Password", "", 50);
WiFiManagerParameter mqtt_port("mqtt_port", "MQTT Port", "", 50);
WiFiManagerParameter mqtt_topic("mqtt_topic", "MQTT Topic", "", 50);

//mqtt
WiFiClient espClient;
PubSubClient objClient(espClient);

//settings
bool settingsLoaded = false; //if the settings have been loaded, lets try to connect MQTT

//web config
bool portalRunning = false; //if the web portal is running, we process requests

//timer
unsigned long everyMinute; //hearbeat every minute

//dht settings
#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

//sets all the values in the config screen so we can see them
void pushSettingsToParameters()
{
  //the last value sets the possible length again... kidna dumb
  mqtt_server.setValue(strServer, 50);
  mqtt_user.setValue(strUser, 50);
  mqtt_pass.setValue(strPass, 50);
  mqtt_port.setValue(strPort, 50);
  mqtt_topic.setValue(strTopic, 50);
}

//saves the tofle to JSON
void saveSettings()
{
  //erase all settings
  LittleFS.format();

  Serial.println("Saving configs...");
  StaticJsonDocument<512> objJSON;
  objJSON["Server"] = mqtt_server.getValue();
  objJSON["User"] = mqtt_user.getValue();
  objJSON["Pass"] = mqtt_pass.getValue();
  objJSON["Port"] = mqtt_port.getValue();
  objJSON["Topic"] = mqtt_topic.getValue();

  File configFile = LittleFS.open(JSON_CONFIG_FILE, "w");
  if (!configFile)
  {
    Serial.println("failed to open config file for writing");
  }

  serializeJsonPretty(objJSON, Serial);
  if (serializeJson(objJSON, configFile) == 0)
  {
    Serial.println(F("Failed to write to file"));
  }
  configFile.close();
}

//pulls the file from memory, and parse JSON
void loadSettings()
{
  if(LittleFS.exists(JSON_CONFIG_FILE))
  {
    Serial.println("Reading config file...");
    File configFile = LittleFS.open(JSON_CONFIG_FILE, "r");
    if(configFile)
    {
      Serial.println("Opened config file");
      StaticJsonDocument<512> objJSON;
      DeserializationError objError = deserializeJson(objJSON, configFile);
      serializeJsonPretty(objJSON, Serial);
      if(!objError)
      {
        //mqtt_server.setValue(objJSON["Server"], objJSON["Server"].size());
        strcpy(strServer, objJSON["Server"]);
        strcpy(strUser, objJSON["User"]);
        strcpy(strPass, objJSON["Pass"]);
        strcpy(strPort, objJSON["Port"]);
        strcpy(strTopic, objJSON["Topic"]);

        settingsLoaded = true; //lets MQTT start to connect

        pushSettingsToParameters(); //makes it so the settings appear in the web interface 
      }
      else
      {
        Serial.println("JSON data malformatted");
      }
    }
    else
    {
      Serial.println("Cant read config file");
    }
  }
  else
  {
    Serial.println("Cant see config file");
  }
}

//when the ettings are saved again, we push them to JSON
void saveConfigCallback()
{
  //load settings to the file
  saveSettings();

  //pull them back out as the global variables
  loadSettings();

  //force a disconnect for a reconnect
  objClient.disconnect(); 
}

//create JSON string 
String sensorJSON()
{
  String strReturn = "";
  String strTemp = (String)dht.readTemperature(true);
  String strHum = (String)dht.readHumidity();
  strReturn = "{\"humidity\":" + strHum + ",\"temperature\":" + strTemp + "}";
  Serial.println(strReturn);
  return strReturn;
}

//connects to MQTT
void MQTTConnect() {

  //Had issues with reconnects... This solved it..
  if (objClient.state() == 0) {
    return;
  }
  // Loop until we're reconnected
  if (!objClient.connected()) {
    Serial.print("Client state: ");
    Serial.println(objClient.state());

    Serial.print("Attempting MQTT connection to address: ");
    Serial.println(strServer);

    // Attempt to connect
    if (objClient.connect(strServer,strUser, strPass)) {
      Serial.println("Connected!!");

      //wake publish
      objClient.publish(strTopic, sensorJSON().c_str());
    } 
    else 
    {
      //get the return code
      Serial.print("failed, rc=");
      Serial.print(objClient.state());
      Serial.println(" try again in 1 second");
      // Wait 1 second before retrying
      delay(1000);
    }
  }
}

void setup() {
  Serial.begin(9600);

  //setup connection to the file system
  if (!LittleFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }

  //create a unique SSID
  String strMac = WiFi.macAddress();
  String strSSIDName = "TempSen-" + strMac.substring(8);
  strSSIDName.replace(":", "");

  //add the parameters 
  wifiManager.addParameter(&mqtt_server);
  wifiManager.addParameter(&mqtt_user);
  wifiManager.addParameter(&mqtt_pass);
  wifiManager.addParameter(&mqtt_port);
  wifiManager.addParameter(&mqtt_topic);

  //When we hit save
  wifiManager.setSaveParamsCallback(saveConfigCallback);

  //wait 3 minutes for the wifi to come up
  wifiManager.setConfigPortalTimeout(180);

  //keep config portal going
  wifiManager.setDisableConfigPortal(false);

  //set hostname
  wifiManager.setHostname(strSSIDName.c_str());

  //setup the menu (removing some items)
  std::vector<const char *> menu = {"wifi","param","sep","info","sep","restart","exit"};
  wifiManager.setMenu(menu);

  //this will load the settings from the 
  loadSettings();

  //mqtt configs
  objClient.setKeepAlive(15); //send pings

  int intPort = atoi(strPort); //convert int
  objClient.setServer(strServer, intPort); //set server and port

  //setup the SSID if we are in discovery mode
  if(wifiManager.autoConnect(strSSIDName.c_str()))
  {
    wifiManager.startWebPortal();
    portalRunning = true;

  }
  
  //get the sensor running
  dht.begin();
}

//main loop
void loop() {

  //config portal
  if(portalRunning){
    //process the web portal traffic
    wifiManager.process();
  }

  //if we have settings from JSON
  if(settingsLoaded)
  {
    //connect to MQTT
    MQTTConnect();
    //process MQTT
    objClient.loop();
  }

  //every minute we publish the values
  if(millis() - everyMinute >= 60*1000UL)
  {
    objClient.publish(strTopic, sensorJSON().c_str());
    everyMinute = millis();
  }

}