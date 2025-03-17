# Notes

## Unifi APs

Several people have reported issues using Unifi APs.

https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA/issues/692

https://github.com/visualapproach/WiFi-remote-for-Bestway-Lay-Z-SPA/issues/504

If you are getting broken page loads, dropped/slow pings, web socket disconnects, etc. try restarting the AP.

I spent a lot of time looking for hardware/firmware issues which turned out to be network issues.

Disabling non existant MQTT server also seems to help in this scenario.

### Unifi specific tips

- Use a dedicated IoT SSID and enable `Enhanced IoT Connectivity`

- Disable `Daily channel optimization`

## General Wifi tips

- Try using a fixed Wifi channel 1 or 11, issues reported with channels 5, 6 & 7

- Try reducing transmit power if using multiple APs in close proximity

## OTA Upload

After initial firmware upload via USB/UART, subsequent updates can be performed over-the-air (OTA).

Comment out the line below in `platformio.ini` and uncomment the 4 lines below it to enable OTA:

```ini
; upload_protocol = esptool
upload_protocol = espota
upload_port = layzspa.local
upload_flags = 
  --auth=esp8266
```

Upload filesystem image OTA is possible if using the dev branch for esp8266 platform, this is also set in `platformio.ini`:

```ini
; platform = espressif8266@^4
platform = https://github.com/platformio/platform-espressif8266.git
```

If OTA fails with no response from device ensure there is no firewall blocking at network or OS level.  You may need to allow traffic between subnets or incoming connections to VS Code.

Uploading filesystem resets saved preferences for Wifi, MQTT, hardware model, etc.

In addition to uploading `.json` configuration files through the web UI you can also preload them in the `data` folder so that they are uploaded with the filesystem.

You cannot download `wifi.json` or `mqtt.json` by default, examples below:

### wifi.json
```json
{
    "enableAp": true,
    "enableWM": true,
    "apSsid": "ssid-name",
    "apPwd": "ssid-password",
    "enableStaticIp4": false,
    "ip4Address": "",
    "ip4Gateway": "",
    "ip4Subnet": "",
    "ip4DnsPrimary": "",
    "ip4DnsSecondary": "",
    "ip4NTP": ""
}
```

### mqtt.json
```json
{
    "enableMqtt": true,
    "mqttHost": "192.168.0.20",
    "mqttPort": 1883,
    "mqttUsername": "mqtt-username",
    "mqttPassword": "mqtt-password",
    "mqttClientId": "layzspa",
    "mqttBaseTopic": "layzspa",
    "mqttTelemetryInterval": 60
}
```

## mDNS

If you are getting NXDOMAIN for `layzspa.local` or `socket.gaierror: [Errno 11001] getaddrinfo failed` during OTA then there is likely an issue with mDNS.

Retrying the request will often work.

If using multiple subnets check you have mDNS reflection enabled.

## Other tips

If AP details have been previously saved in internal flash but don't exist in in wifi.json connection will take longer while soft AP loads before falling back to saved AP.  Make sure to save the wifi details for fast reconnect.

When using multiple ESPs update the hostname in `config.h`:

```h
#define DEVICE_NAME "layzspa2"
```

`platformio.ini`:

```ini
upload_port = layzspa2.local
```

Counters are expected to have bogus values on initial firmware upload