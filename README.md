# OutdoorTempSensor
Reads a DHT22 and publishes it to MQTT

- Uses WiFiManager with SPIFFS for persistant storage
- Allows connection to private IP after WiFi config to add/change MQTT settings
- Uses PubSubClient for MQTT

# Setup instructions
- Flash using PlatformIO to ESP01 or NodeMCU 
 - Change platformio.ini to reflect model
- Connect DHT22 to GPIP 2 (pin 4 on NodeMCU)
 - If you want to change these parameters, just look for them in the code
- Power up
- Look for the new Wireless Access Point TempSen-###### (the last 6 characters of the MAC address) 
 - Write this hostname down as you will need this to configure MQTT later
 - Use your phone or anything with wireless
- Connect and then access the web admin
 - Most phones will use the captive portal, if not access 192.168.4.1 with a browser
- Click "Configure WiFi" and add your WiFi credentials
- After the reboot access the device with the hostname TempSen-###### or refer to your router for DHCP IP
- Click Setup to access the MQTT settings


![Main Page](https://github.com/linuxx/OutdoorTempSensor/blob/master/images/main.jpg)
![WiFi Scan](https://github.com/linuxx/OutdoorTempSensor/blob/master/images/wifiscan.jpg)
![MQTT Page](https://github.com/linuxx/OutdoorTempSensor/blob/master/images/mqtt.jpg)

