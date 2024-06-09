#include "main.h"

// initial stack
char *stack_start;
uint32_t heap_water_mark;

// Setup a oneWire instance to communicate with any OneWire devices
// Setting arbitrarily to 231 since this isn't an actual pin
// Later during "setup" the correct pin will be set, if enabled 
OneWire *oneWire;
// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature *tempSensors;

WiFiEventHandler gotIpEventHandler, disconnectedEventHandler;
void cb_gotIP(const WiFiEventStationModeGotIP& event)
{
    Serial.print("got IP: ");
    Serial.println(WiFi.localIP());

    if(webSocket != nullptr)
    {
        Serial.println(F("deleting ws"));
        webSocket->disconnect();
        webSocket->close();
        delete webSocket;
        webSocket = nullptr;
        Serial.println(F("deleted ws"));
        Serial.println(F("restarting ws"));
        startWebSocket();
    }
    if(server != nullptr)
    {
        Serial.println(F("deleting server"));
        server->stop();
        server->close();
        delete server;
        server = nullptr;
        Serial.println(F("deleted server"));
        Serial.println(F("restarting server"));
        startHttpServer();
    }
}

void cb_disconnected(const WiFiEventStationModeDisconnected& event)
{
    Serial.println(F("disconnected"));
}

void setup()
{
    
    // init record of stack
    char stack;
    stack_start = &stack;

    Serial.begin(76800);
    BWC_LOG_P(PSTR("\nStart\n"),0);
    BWC_LOG_P(PSTR("Millis: %d @ line: %d\n"), millis(), __LINE__);
    /*register wifi events */
    gotIpEventHandler = WiFi.onStationModeGotIP(cb_gotIP);
    disconnectedEventHandler = WiFi.onStationModeDisconnected(cb_disconnected);

    LittleFS.begin();
    BWC_LOG_P(PSTR("Millis: %d @ line: %d\n"), millis(), __LINE__);
    {
        HeapSelectIram ephemeral;
        // Serial.printf_P(PSTR("IRamheap %d\n"), ESP.getFreeHeap());
        bwc = new BWC;
        oneWire = new OneWire(231);
        tempSensors = new DallasTemperature(oneWire);
    }
    BWC_LOG_P(PSTR("Millis: %d @ line: %d\n"), millis(), __LINE__);
    bwc->setup();
    BWC_LOG_P(PSTR("Millis: %d @ line: %d\n"), millis(), __LINE__);
    bwc->loop();
    BWC_LOG_P(PSTR("Millis: %d @ line: %d\n"), millis(), __LINE__);
    periodicTimer.attach(periodicTimerInterval, []{ periodicTimerFlag = true; });
    // delayed mqtt start
    startComplete.attach(30, []{ if(useMqtt) enableMqtt = true; startComplete.detach(); });
    // update webpage every 2 seconds. (will also be updated on state changes)
    updateWSTimer.attach(2.0, []{ sendWSFlag = true; });
    BWC_LOG_P(PSTR("Millis: %d @ line: %d\n"), millis(), __LINE__);
    loadWebConfig();
    BWC_LOG_P(PSTR("Millis: %d @ line: %d\n"), millis(), __LINE__);
    startWiFi();
    BWC_LOG_P(PSTR("Millis: %d @ line: %d\n"), millis(), __LINE__);
    startNTP();
    BWC_LOG_P(PSTR("Millis: %d @ line: %d\n"), millis(), __LINE__);
    startOTA();
    BWC_LOG_P(PSTR("Millis: %d @ line: %d\n"), millis(), __LINE__);
    startHttpServer();
    BWC_LOG_P(PSTR("Millis: %d @ line: %d\n"), millis(), __LINE__);
    startWebSocket();
    BWC_LOG_P(PSTR("Millis: %d @ line: %d\n"), millis(), __LINE__);
    startMqtt();
    BWC_LOG_P(PSTR("Millis: %d @ line: %d\n"), millis(), __LINE__);
    if(bwc->hasTempSensor)
    { 
        oneWire->begin(bwc->tempSensorPin);
        tempSensors->begin();
    }
    Serial.println(WiFi.localIP().toString());
    BWC_LOG_P(PSTR("Millis: %d @ line: %d\n"), millis(), __LINE__);
    bwc->print("   ");  //No overloaded function exists for the F() macro
    BWC_LOG_P(PSTR("Millis: %d @ line: %d\n"), millis(), __LINE__);
    bwc->print(WiFi.localIP().toString());
    BWC_LOG_P(PSTR("Millis: %d @ line: %d\n"), millis(), __LINE__);
    bwc->print("   ");
    BWC_LOG_P(PSTR("Millis: %d @ line: %d\n"), millis(), __LINE__);
    bwc->print(FW_VERSION);
    BWC_LOG_P(PSTR("Millis: %d @ line: %d\n"), millis(), __LINE__);
    Serial.println(F("End of setup()"));
    BWC_LOG_P(PSTR("Millis: %d @ line: %d\n"), millis(), __LINE__);
    heap_water_mark = ESP.getFreeHeap();
    Serial.println(ESP.getFreeHeap());
}

void loop()
{
    uint32_t freeheap = ESP.getFreeHeap();
    if(freeheap < heap_water_mark) heap_water_mark = freeheap;

    // We need this self-destructing info several times, so save it on the stack
    bool newData = bwc->newData();
    // Fiddle with the pump computer
    bwc->loop();

    // run only when a wifi connection is established
    if (WiFi.status() == WL_CONNECTED)
    {
        // listen for webserver events
        server->handleClient();

        // listen for OTA events
        ArduinoOTA.handle();

        // MQTT
        if (enableMqtt && mqttClient->loop())
        {
            String msg;
            msg.reserve(32);
            bwc->getButtonName(msg);
            // publish pretty button name if display button is pressed (or NOBTN if released)
            if (!msg.equals(prevButtonName))
            {
                mqttClient->publish((String(mqttBaseTopic) + "/button").c_str(), String(msg).c_str(), true);
                prevButtonName = msg;
            }

            if (newData || sendMQTTFlag)
            {
                sendMQTT();
                sendMQTTFlag = false;
            }

            if(send_mqtt_cfg_needed)
            {
                send_mqtt_cfg_needed = false;
                sendMQTTConfig();
            }
        }

        // web socket
        if (newData || sendWSFlag)
        {
            sendWSFlag = false;
            sendWS();
        }
    }

    // run every X seconds
    if (periodicTimerFlag)
    {
        periodicTimerFlag = false;
        if (WiFi.status() != WL_CONNECTED)
        {
            bwc->print(F("check network"));
            // Serial.println(F("WiFi > Trying to reconnect ..."));
        }
        if (WiFi.status() == WL_CONNECTED)
        {
            // could be interesting to display the IP
            //bwc->print(WiFi.localIP().toString());

            if (time(nullptr)<57600)
            {
                // Serial.println(F("NTP > Start synchronisation"));
                startNTP();
            }

            if (enableMqtt && !mqttClient->loop())
            {
                // Serial.println(F("MQTT > Not connected"));
                mqttConnect();
            }
        }
        // Leverage the pre-existing periodicTimerFlag to also set temperature, if enabled
        setTemperatureFromSensor();

        /* Debug */
        // static uint8_t minutes = 0;
        // minutes++;
        // if(minutes >= 5)
        // {
            // minutes = 0;
            // write_mem_stats_to_file();
        // }
    }


    //Only do this if locked out! (by pressing POWER - LOCK - TIMER - POWER)
    if(bwc->getBtnSeqMatch())
    {
    
    resetWiFi();
    delay(3000);
    ESP.reset();
    delay(3000);
    }
    //handleAUX();
}

/* Debugging to file, normally not used */
void write_mem_stats_to_file()
{
    File file = LittleFS.open(F("memstats.txt"), "a");
    if (!file)
    {
        file.close();
        return;
    }
    time_t now = time(nullptr);
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    // Serial.print(asctime(&timeinfo));
    {
        HeapSelectIram ephemeral;
        file.printf_P(PSTR("Time: %s, IRam: free %d, frag %d, max block %d "),
            asctime(&timeinfo),
            ESP.getFreeHeap(), 
            ESP.getHeapFragmentation(),
            ESP.getMaxFreeBlockSize()
            );
    }
    /*Dram*/
    file.printf_P(PSTR("DRam: free %d, frag %d, max block %d\n"),
        ESP.getFreeHeap(), 
        ESP.getHeapFragmentation(),
        ESP.getMaxFreeBlockSize()
        );
    file.close();
}
    


/**
 * Send status data to web client in JSON format (because it is easy to decode on the other side)
 */
void sendWS()
{
    if(webSocket->connectedClients() == 0) return;
    // HeapSelectIram ephemeral;
    // Serial.printf("IRamheap %d\n", ESP.getFreeHeap());
    // send states
    String json;
    json.reserve(320);

    bwc->getJSONStates(json);
    webSocket->broadcastTXT(json);
    // send times
    json.clear();
    bwc->getJSONTimes(json);
    webSocket->broadcastTXT(json);
    // send other info
    json.clear();
    getOtherInfo(json);
    webSocket->broadcastTXT(json);
    // json = bwc->getDebugData();
    // webSocket->broadcastTXT(json);
    // time_t now = time(nullptr);
    // struct tm timeinfo;
    // gmtime_r(&now, &timeinfo);
    // Serial.print("Current time: ");
    // Serial.print(asctime(&timeinfo));
}

void getOtherInfo(String &rtn)
{
    // DynamicJsonDocument doc(512);
    StaticJsonDocument<512> doc;
    // Set the values in the document
    doc[F("CONTENT")] = F("OTHER");
    doc[F("MQTT")] = mqttClient->state();
    /*TODO: add these:*/
    //   doc[F("PressedButton")] = bwc->getPressedButton();
    doc[F("HASJETS")] = bwc->hasjets;
    doc[F("HASGOD")] = bwc->hasgod;
    doc[F("MODEL")] = bwc->getModel();
    doc[F("RSSI")] = WiFi.RSSI();
    doc[F("IP")] = WiFi.localIP().toString();
    doc[F("SSID")] = WiFi.SSID();
    doc[F("FW")] = FW_VERSION;
    doc[F("loopfq")] = bwc->loop_count;
    bwc->loop_count = 0;

    // Serialize JSON to string
    if (serializeJson(doc, rtn) == 0)
    {
        rtn = F("{\"error\": \"Failed to serialize other\"}");
    }
}

/**
 * Send STATES and TIMES to MQTT
 * It would be more elegant to send both states and times on the "message" topic
 * and use the "CONTENT" field to distinguish between them
 * but it might break peoples home automation setups, so to keep it backwards
 * compatible I choose to start a new topic "/times"
 * @author 877dev
 */
void sendMQTT()
{
    // HeapSelectIram ephemeral;
    // Serial.printf("IRamheap %d\n", ESP.getFreeHeap());
    String json;
    json.reserve(320);

    // send states
    bwc->getJSONStates(json);
    if (mqttClient->publish((String(mqttBaseTopic) + F("/message")).c_str(), String(json).c_str(), true))
    {
        //Serial.println(F("MQTT > message published"));
    }
    else
    {
        //Serial.println(F("MQTT > message not published"));
    }
// delay(2);

    // send times
    json.clear();
    bwc->getJSONTimes(json);
    if (mqttClient->publish((String(mqttBaseTopic) + F("/times")).c_str(), String(json).c_str(), true))
    {
        //Serial.println(F("MQTT > times published"));
    }
    else
    {
        //Serial.println(F("MQTT > times not published"));
    }
// delay(2);

    //send other info
    json.clear();
    getOtherInfo(json);
    if (mqttClient->publish((String(mqttBaseTopic) + F("/other")).c_str(), String(json).c_str(), true))
    {
        //Serial.println(F("MQTT > other published"));
    }
    else
    {
        //Serial.println(F("MQTT > other not published"));
    }
}

void sendMQTTConfig()
{
    String json;
    json.reserve(320);
    bwc->getJSONSettings(json);
    mqttClient->publish((String(mqttBaseTopic) + F("/get_config")).c_str(), String(json).c_str(), true);
    mqttClient->loop();
}

/**
 * Start a Wi-Fi access point, and try to connect to some given access points.
 * Then wait for either an AP or STA connection
 */
void startWiFi()
{
    //WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
    WiFi.hostname(netHostname);
    sWifi_info wifi_info;
    wifi_info = loadWifi();


    if (wifi_info.enableStaticIp4)
    {
        Serial.println(F("Setting static IP"));
        IPAddress ip4Address;
        IPAddress ip4Gateway;
        IPAddress ip4Subnet;
        IPAddress ip4DnsPrimary;
        IPAddress ip4DnsSecondary;
        ip4Address.fromString(wifi_info.ip4Address_str);
        ip4Gateway.fromString(wifi_info.ip4Gateway_str);
        ip4Subnet.fromString(wifi_info.ip4Subnet_str);
        ip4DnsPrimary.fromString(wifi_info.ip4DnsPrimary_str);
        ip4DnsSecondary.fromString(wifi_info.ip4DnsSecondary_str);
        // Serial.println("WiFi > using static IP \"" + ip4Address.toString() + "\" on gateway \"" + ip4Gateway.toString() + "\"");
        WiFi.config(ip4Address, ip4Gateway, ip4Subnet, ip4DnsPrimary, ip4DnsSecondary);
    }

    if (wifi_info.enableAp)
    {
        Serial.print(F("WiFi > using WiFi configuration with SSID \""));
        Serial.println(wifi_info.apSsid + "\"");

        WiFi.begin(wifi_info.apSsid.c_str(), wifi_info.apPwd.c_str());

        Serial.print(F("WiFi > Trying to connect ..."));
        int maxTries = 1000;
        int tryCount = 0;

        while (WiFi.status() != WL_CONNECTED)
        {
            /* I suspect this old delay(1000) made the 4-wire DSPs to go nuts */
            bwc->loop();
            delay(10);
            // Serial.print(".");
            tryCount++;

            if (tryCount >= maxTries)
            {
                // Serial.println("");
                // Serial.println(F("WiFi > NOT connected!"));
                if (wifi_info.enableWmApFallback)
                {
                    // disable specific WiFi config
                    wifi_info.enableAp = false;
                    wifi_info.enableStaticIp4 = false;
                    // fallback to WiFi config portal
                    startWiFiConfigPortal();
                }
                break;
            }
            // Serial.println("");
        }
    }
    else
    {
        startWiFiConfigPortal();
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        wifi_info.enableAp = true;
        wifi_info.apSsid = WiFi.SSID();
        wifi_info.apPwd = WiFi.psk();
        saveWifi(wifi_info);
    }
}

/**
 * start WiFiManager configuration portal
 */
void startWiFiConfigPortal()
{
    Serial.println(F("WiFi > Using WiFiManager Config Portal"));
    ESP_WiFiManager wm;
    wm.autoConnect(wmApName, wmApPassword);
    // Serial.print(F("WiFi > Trying to connect ..."));
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        // Serial.print(".");
    }
    // Serial.println("");
}

/**
 * start NTP sync
 */
void startNTP()
{
    sWifi_info wifi_info;
    wifi_info = loadWifi();
    Serial.println(F("start NTP"));
    // configTime(0,0,"pool.ntp.org", "time.nist.gov");
    configTime(0,0,wifi_info.ip4NTP_str, F("pool.ntp.org"), F("time.nist.gov"));
    time_t now = time(nullptr);
    int count = 0;
    while (now < 8 * 3600 * 2) {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
        if(count++ > 10) return;
    }
    Serial.println();
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    // Serial.print("Current time: ");
    // Serial.print(asctime(&timeinfo));

    time_t boot_timestamp = getBootTime();
    tm * boot_time_tm = gmtime(&boot_timestamp);
    char boot_time_str[64];
    strftime(boot_time_str, 64, "%F %T", boot_time_tm);
    bwc->reboot_time_str = String(boot_time_str);
    bwc->reboot_time_t = boot_timestamp;
    bwc->saveRebootInfo();
}

void startOTA()
{
    ArduinoOTA.setHostname(OTAName);
    ArduinoOTA.setPassword(OTAPassword);

    ArduinoOTA.onStart([]() {
        // Serial.println(F("OTA > Start"));
        stopall();
    });
    ArduinoOTA.onEnd([]() {
        // Serial.println(F("OTA > End"));
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        // Serial.printf("OTA > Progress: %u%%\r\n", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        // Serial.printf("OTA > Error[%u]: ", error);
        // if (error == OTA_AUTH_ERROR) Serial.println(F("Auth Failed"));
        // else if (error == OTA_BEGIN_ERROR) Serial.println(F("Begin Failed"));
        // else if (error == OTA_CONNECT_ERROR) Serial.println(F("Connect Failed"));
        // else if (error == OTA_RECEIVE_ERROR) Serial.println(F("Receive Failed"));
        // else if (error == OTA_END_ERROR) Serial.println(F("End Failed"));
    });
    ArduinoOTA.begin();
    // Serial.println(F("OTA > ready"));
}

void stopall()
{
    Serial.printf_P(PSTR("Free mem before stop: %d\n"), ESP.getFreeHeap());
    bwc->stop();
    Serial.println(F("detaching"));
    updateMqttTimer.detach();
    periodicTimer.detach();
    updateWSTimer.detach();
    //bwc->saveSettings();
    delete tempSensors;
    delete oneWire;
    Serial.println(F("stopping mqtt"));
    if(enableMqtt) mqttClient->disconnect();
    delete aWifiClient;
    delete mqttClient;
    mqttClient = nullptr;
    Serial.println(F("stopping server"));
    server->stop();
    delete server;
    server = nullptr;
    Serial.println(F("stopping ws"));
    webSocket->close();
    delete webSocket;
    webSocket = nullptr;
    Serial.println(F("stopping FS"));
    LittleFS.end();
    Serial.println(F("end stopall"));
    Serial.printf_P(PSTR("Free mem after stop: %d\n"), ESP.getFreeHeap());
}

/*pause: action=true cont: action=false*/
void pause_all(bool action)
{
    if(action)
    {
        if(periodicTimer.active()) periodicTimer.detach();
        if(startComplete.active()) startComplete.detach();
        if(updateWSTimer.active()) updateWSTimer.detach();
        if(bootlogTimer.active()) bootlogTimer.detach();
    } else 
    {
        periodicTimer.attach(periodicTimerInterval, []{ periodicTimerFlag = true; });
        startComplete.attach(60, []{ if(useMqtt) enableMqtt = true; startComplete.detach(); });
        updateWSTimer.attach(2.0, []{ sendWSFlag = true; });
        //bootlogTimer.attach(5, []{ if(DateTime.isTimeValid()) {bwc->saveRebootInfo(); bootlogTimer.detach();} });
    }
    bwc->pause_all(action);
}

void startWebSocket()
{
    HeapSelectIram ephemeral;
    Serial.printf_P(PSTR("WS IRamheap %d\n"), ESP.getFreeHeap());
    if(webSocket == nullptr) 
    {
        webSocket = new WebSocketsServer(81);
    }
    // In case we are already running
    webSocket->close();
    webSocket->begin();
    webSocket->enableHeartbeat(3000, 3000, 1);
    webSocket->onEvent(webSocketEvent);
    // Serial.println(F("WebSocket > server started"));
}

/**
 * handle web socket events
 */
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t len)
{
    // When a WebSocket message is received
    switch (type)
    {
        // if the websocket is disconnected
        case WStype_DISCONNECTED:
        // Serial.printf("WebSocket > [%u] Disconnected!\r\n", num);
        break;

        // if a new websocket connection is established
        case WStype_CONNECTED:
        {
            // IPAddress ip = webSocket->remoteIP(num);
            // Serial.printf("WebSocket > [%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);
            sendWS();
        }
        break;

        // if new text data is received
        case WStype_TEXT:
        {
            // Serial.printf("WebSocket > [%u] get Text: %s\r\n", num, payload);
            // DynamicJsonDocument doc(256);
            StaticJsonDocument<256> doc;
            DeserializationError error = deserializeJson(doc, payload);
            if (error)
            {
            Serial.println(F("WebSocket > JSON command failed"));
            return;
            }

            // Copy values from the JsonDocument to the Config
            Commands command = doc[F("CMD")];
            int64_t value = doc[F("VALUE")];
            int64_t xtime = doc[F("XTIME")];
            int64_t interval = doc[F("INTERVAL")];
            String txt = doc[F("TXT")] | "";
            command_que_item item;
            item.cmd = command;
            item.val = value;
            item.xtime = xtime;
            item.interval = interval;
            item.text = txt;
            bwc->add_command(item);
        }
        break;

        default:
        break;
    }
}

/**
 * start a HTTP server with a file read and upload handler
 */
void startHttpServer()
{
    if(server != nullptr) return;
    {
        HeapSelectIram ephemeral;
        server = new ESP8266WebServer(80);
        // In case we are already running
        server->stop();
        server->on(F("/getconfig/"), handleGetConfig);
        server->on(F("/setconfig/"), handleSetConfig);
        server->on(F("/getcommands/"), handleGetCommandQueue);
        server->on(F("/addcommand/"), handleAddCommand);
        server->on(F("/editcommand/"), handleEditCommand);
        server->on(F("/delcommand/"), handleDelCommand);
        server->on(F("/getwebconfig/"), handleGetWebConfig);
        server->on(F("/setwebconfig/"), handleSetWebConfig);
        server->on(F("/getwifi/"), handleGetWifi);
        server->on(F("/setwifi/"), handleSetWifi);
        server->on(F("/resetwifi/"), handleResetWifi);
        server->on(F("/getmqtt/"), handleGetMqtt);
        server->on(F("/setmqtt/"), handleSetMqtt);
        server->on(F("/dir/"), handleDir);
        server->on(F("/hwtest/"), handleHWtest);
        server->on(F("/inputs/"), handleInputs);
        server->on(F("/upload.html"), HTTP_POST, [](){
            server->send(200, F("text/plain"), "");
        }, handleFileUpload);
        server->on(F("/remove.html"), HTTP_POST, handleFileRemove);
        server->on(F("/remove/"), HTTP_GET, handleFileRemove);
        server->on(F("/restart/"), handleRestart);
        server->on(F("/metrics"), handlePrometheusMetrics);  //prometheus metrics
        server->on(F("/info/"), handleESPInfo);
        server->on(F("/sethardware/"), handleSetHardware);
        server->on(F("/gethardware/"), handleGetHardware);
        server->on(F("/debug-on/"), [](){bwc->BWC_DEBUG = true; server->send(200, F("text/plain"), "ok");});
        server->on(F("/debug-off/"), [](){bwc->BWC_DEBUG = false; server->send(200, F("text/plain"), "ok");});
        server->on(F("/cmdq_file/"), handle_cmdq_file);

        // if someone requests any other file or page, go to function 'handleNotFound'
        // and check if the file exists
        server->onNotFound(handleNotFound);
        // start the HTTP server
        server->begin();
    }
    
    // Serial.println(F("HTTP > server started"));
}

void handleGetHardware()
{
    if (!checkHttpPost(server->method())) return;
    File file = LittleFS.open(F("hwcfg.json"), "r");
    if (!file)
    {
        // Serial.println(F("Failed to open hwcfg.json"));
        server->send(404, F("text/plain"), F("not found"));
        return;
    }
    server->send(200, F("text/plain"), file.readString());
    file.close();
}

void handleSetHardware()
{
    if (!checkHttpPost(server->method())) return;
    String message = server->arg(0);
    // Serial.printf("Set hw message; %s\n", message.c_str());
    File file = LittleFS.open(F("hwcfg.json"), "w");
    if (!file)
    {
        // Serial.println(F("Failed to save hwcfg.json"));
        return;
    }
    file.print(message);
    file.close();
    server->send(200, F("text/plain"), "ok");
    // Serial.println("sethardware done");
}

void preparefortest()
{
    for(int i = 0; i < 7; i++)
    {
        pinMode(bwc->pins[i], INPUT);
    }
}

void handleInputs()
{
    server->setContentLength(CONTENT_LENGTH_UNKNOWN);
    server->send(200, F("text/plain"), "");

    bwc->stop();
    preparefortest();

    bool old_pin_state[7] = {0}, new_pin_state[7] = {0};
    int counter[7] = {0};
    unsigned long t = millis(); //start timestamp

    while(millis() < t+5000)
    {
        for(uint8_t i = 0; i < 7; i++)
        {
            new_pin_state[i] = digitalRead(bwc->pins[i]);
            if(new_pin_state[i] != old_pin_state[i]) counter[i]++;
            old_pin_state[i] = new_pin_state[i];
        }
        yield();
    }

    /* send statistics to client */
    char s[128];
    for(int i = 0; i < 7; i++)
    {
        sprintf_P(s, PSTR("Edges received on pin D%d: %d\n"), gpio2dp(bwc->pins[i]), counter[i]);
        server->sendContent(s);
    }
    sprintf_P(s, PSTR("On 6-w pump the highest number is CLK, next is DATA and third is CS. On 4-wires the highest is CIO or DSP TX to ESP."));
    server->sendContent(s);
    server->sendContent("");
    bwc->setup();
}

void handleHWtest()
{
    server->setContentLength(CONTENT_LENGTH_UNKNOWN);
    server->send(200, F("text/plain"), "");

    int errors = 0;
    bool state = false;
    char result[128];

    bwc->stop();
    preparefortest();

    for(int i = 0; i < 10; i++)
    {
        sprintf_P(result, PSTR("\nConnect the cables now!\nStarting test in %d seconds...\n"), 10-i);
        server->sendContent(result);
        for(int t = 0; t < 512; t++)
            server->sendContent(" ");
        delay(1000);
    }

    /* First test CIO out/ DSP in ports */
    sprintf_P(result, PSTR("Start test. Seq begins with HIGH, then alters.\n\n"));
    server->sendContent(result);
    for(int pin = 0; pin < 3; pin++)
    {
        sprintf_P(result, PSTR("Sending on D%d, receiving on D%d\n"), gpio2dp(bwc->pins[pin]), gpio2dp(bwc->pins[pin+3]));
        server->sendContent(result);
        pinMode(bwc->pins[pin], OUTPUT);
        pinMode(bwc->pins[pin+3], INPUT);
        for(int t = 0; t < 100; t++)
        {
            state = !state;
            digitalWrite(bwc->pins[pin], state);
            delayMicroseconds(100);
            bool error = digitalRead(bwc->pins[pin+3]) != state;
            errors += error;
            if(error)
                if(state)
                    server->sendContent("1");
                else
                    server->sendContent("0");
            else
                server->sendContent("-");
        }
        sprintf_P(result, PSTR(" // %d errors out of 100\n"), errors);
        server->sendContent(result);
        errors = 0;
        delay(0);
    }

    /* Test the other way around */

    for(int pin = 0; pin < 3; pin++)
    {
        sprintf_P(result, PSTR("Sending on D%d, receiving on D%d\n"), gpio2dp(bwc->pins[pin+3]), gpio2dp(bwc->pins[pin]));
        server->sendContent(result);
        pinMode(bwc->pins[pin+3], OUTPUT);
        pinMode(bwc->pins[pin], INPUT);
        for(int t = 0; t < 100; t++)
        {
            state = !state;
            digitalWrite(bwc->pins[pin+3], state);
            delayMicroseconds(100);
            bool error = digitalRead(bwc->pins[pin]) != state;
            errors += error;
            if(error)
                if(state)
                    server->sendContent("1");
                else
                    server->sendContent("0");
            else
                server->sendContent("-");
        }
        sprintf_P(result, PSTR(" // %d errors out of 100\n"), errors);
        server->sendContent(result);
        errors = 0;
        delay(0);
    }

    sprintf_P(result, PSTR("End of test!\n\"1\" or \"0\" indicates ERROR, depending on test state. \"-\" is good.\n"));
    server->sendContent(result);
    sprintf_P(result, PSTR("Switching cio pins 5s HIGH -> 5s LOW -> input\n"));
    server->sendContent(result);
    sprintf_P(result, PSTR("then DSP pins 5s HIGH -> 5s LOW -> input, repeating\n"));
    server->sendContent(result);
    sprintf_P(result, PSTR("Disconnect cables then reset chip when done!\n"));
    server->sendContent(result);

    server->sendContent("");
    while(true)
    {
        /*CIO pins HIGH*/
        for(int pin = 0; pin < 3; pin++)
        {
            pinMode(bwc->pins[pin+3], INPUT);
            pinMode(bwc->pins[pin+0], OUTPUT);
            digitalWrite(bwc->pins[pin], HIGH);
        }
        delay(5000);
        /*CIO pins LOW*/
        for(int pin = 0; pin < 3; pin++)
        {
            pinMode(bwc->pins[pin+3], INPUT);
            pinMode(bwc->pins[pin+0], OUTPUT);
            digitalWrite(bwc->pins[pin], LOW);
        }
        delay(5000);
        /*DSP pins HIGH*/
        for(int pin = 0; pin < 3; pin++)
        {
            pinMode(bwc->pins[pin+0], INPUT);
            pinMode(bwc->pins[pin+3], OUTPUT);
            digitalWrite(bwc->pins[pin+3], HIGH);
        }
        delay(5000);
        /*DSP pins LOW*/
        for(int pin = 0; pin < 3; pin++)
        {
            pinMode(bwc->pins[pin+0], INPUT);
            pinMode(bwc->pins[pin+3], OUTPUT);
            digitalWrite(bwc->pins[pin+3], LOW);
        }
        delay(5000);
    }
    bwc->setup();
}

void handleNotFound()
{
    // check if the file exists in the flash memory (LittleFS), if so, send it
    if (!handleFileRead(server->uri()))
    {
        server->send(404, F("text/plain"), F("404: File Not Found"));
    }
}

String getContentType(const String& filename)
{
    if (filename.endsWith(".html")) return F("text/html");
    else if (filename.endsWith(".css")) return F("text/css");
    else if (filename.endsWith(".js")) return F("application/javascript");
    else if (filename.endsWith(".ico")) return F("image/x-icon");
    else if (filename.endsWith(".gz")) return F("application/x-gzip");
    else if (filename.endsWith(".json")) return F("application/json");
    return F("text/plain");
}

/**
 * send the right file to the client (if it exists)
 */
bool handleFileRead(String path)
{
    pause_all(true);
    // Serial.println("HTTP > request: " + path);
    // If a folder is requested, send the index file
    if (path.endsWith("/"))
    {
        path += F("index.html");
    }
    // deny reading credentials
    if (path.equalsIgnoreCase("/mqtt.json") || path.equalsIgnoreCase("/wifi.json"))
    {
        server->send(403, F("text/plain"), F("Permission denied."));
        // Serial.println(F("HTTP > file reading denied (credentials)."));
        pause_all(false);
        return false;
    }

    String contentType = getContentType(path);             // Get the MIME type
    String pathWithGz = path + ".gz";
    if (LittleFS.exists(pathWithGz) || LittleFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
        if (LittleFS.exists(pathWithGz))                         // If there's a compressed version available
            path += ".gz";                                         // Use the compressed version
        File file = LittleFS.open(path, "r");                    // Open the file
        size_t fsize = file.size();
        size_t sent = server->streamFile(file, contentType);    // Send it to the client
        // server->streamFile(file, contentType);    // Send it to the client
        file.close();                                          // Close the file again
        Serial.println(F("File size: ") + String(fsize));
        Serial.println(F("HTTP > file sent: ") + path + F(" (") + sent + F(" bytes)"));
        if(fsize != sent){
            Serial.println(F("********* File not completed *******"));
        }
        pause_all(false);
        return true;
    }
    // Serial.println("HTTP > file not found: " + path);   // If the file doesn't exist, return false
    pause_all(false);
    return false;
}

/**
 * checks the method to be a POST
 */
bool checkHttpPost(HTTPMethod method)
{
    if (method != HTTP_POST)
    {
        server->send(405, F("text/plain"), F("Method not allowed."));
        return false;
    }
    return true;
}

/**
 * response for /getconfig/
 * web server prints a json document
 */
void handleGetConfig()
{
    if (!checkHttpPost(server->method())) return;

    String json;
    json.reserve(320);
    bwc->getJSONSettings(json);
    server->send(200, F("text/plain"), json);
}

/**
 * response for /setconfig/
 * web server writes a json document
 */
void handleSetConfig()
{
    if (!checkHttpPost(server->method())) return;

    String message = server->arg(0);
    bwc->setJSONSettings(message);

    server->send(200, F("text/plain"), "");
    send_mqtt_cfg_needed = true;
}

/**
 * response for /getcommands/
 * web server prints a json document
 */
void handleGetCommandQueue()
{
    if (!checkHttpPost(server->method())) return;

    String json = bwc->getJSONCommandQueue();
    server->send(200, F("application/json"), json);
}

/**
 * response for /addcommand/
 * add a command to the queue
 */
void handleAddCommand()
{
    // if (!checkHttpPost(server->method())) return;

    // DynamicJsonDocument doc(256);
    StaticJsonDocument<256> doc;
    String message = server->arg(0);
    DeserializationError error = deserializeJson(doc, message);
    if (error)
    {
        server->send(400, F("text/plain"), F("Error deserializing message: ")+message);
        return;
    }

    Commands command = doc[F("CMD")];
    int64_t value = doc[F("VALUE")];
    int64_t xtime = doc[F("XTIME")];
    int64_t interval = doc[F("INTERVAL")];
    String txt = doc[F("TXT")] | "";
    command_que_item item;
    item.cmd = command;
    item.val = value;
    item.xtime = xtime;
    item.interval = interval;
    item.text = txt;
    bwc->add_command(item);

    server->send(200, F("text/plain"), F("ok"));
}

/**
 * response for /editcommand/
 * replace a command in the queue with new command
 */
void handleEditCommand()
{
    if (!checkHttpPost(server->method())) return;

    // DynamicJsonDocument doc(256);
    StaticJsonDocument<256> doc;
    String message = server->arg(0);
    DeserializationError error = deserializeJson(doc, message);
    if (error)
    {
        server->send(400, F("text/plain"), F("Error deserializing message"));
        return;
    }

    Commands command = doc[F("CMD")];
    int64_t value = doc[F("VALUE")];
    int64_t xtime = doc[F("XTIME")];
    int64_t interval = doc[F("INTERVAL")];
    String txt = doc[F("TXT")] | "";
    uint8_t index = doc[F("IDX")];
    command_que_item item;
    item.cmd = command;
    item.val = value;
    item.xtime = xtime;
    item.interval = interval;
    item.text = txt;
    bwc->edit_command(index, item);

    server->send(200, F("text/plain"), "");
}

/**
 * response for /delcommand/
 * replace a command in the queue with new command
 */
void handleDelCommand()
{
    if (!checkHttpPost(server->method())) return;

    // DynamicJsonDocument doc(256);
    StaticJsonDocument<256> doc;
    String message = server->arg(0);
    DeserializationError error = deserializeJson(doc, message);
    if (error)
    {
        server->send(400, F("text/plain"), F("Error deserializing message"));
        return;
    }

    uint8_t index = doc[F("IDX")];
    bwc->del_command(index);

    server->send(200, F("text/plain"), "");
}

void handle_cmdq_file()
{
    if (!checkHttpPost(server->method())) return;

    // DynamicJsonDocument doc(256);
    StaticJsonDocument<256> doc;
    String message = server->arg(0);
    DeserializationError error = deserializeJson(doc, message);
    if (error)
    {
        server->send(400, F("text/plain"), F("Error deserializing message"));
        return;
    }

    String action = doc[F("ACT")].as<String>();
    String filename = "/";
    filename += doc[F("NAME")].as<String>();

    if(action.equals("load"))
    {
        copyFile("/cmdq.json", "/cmdq.backup");
        copyFile(filename, "/cmdq.json");
        bwc->reloadCommandQueue();
    }
    if(action.equals("save"))
    {
        copyFile("/cmdq.json", filename);
    }

    server->send(200, F("text/plain"), "");
}

void copyFile(String source, String dest)
{
    char ibuffer[64];  //declare a buffer
    
    File f_source = LittleFS.open(source, "r");    //open source file to read
    if (!f_source)
    {
        return;
    }

    File f_dest = LittleFS.open(dest, "w");    //open destination file to write
    if (!f_dest)
    {
        return;
    }
    
    while (f_source.available() > 0)
    {
        byte i = f_source.readBytes(ibuffer, 64); // i = number of bytes placed in buffer from file f_source
        f_dest.write(ibuffer, i);               // write i bytes from buffer to file f_dest
    }
    
    f_dest.close(); // done, close the destination file
    f_source.close(); // done, close the source file
}

/**
 * load "Web Config" json configuration from "webconfig.json"
 */
void loadWebConfig()
{
    // DynamicJsonDocument doc(1024);
    StaticJsonDocument<256> doc;

    File file = LittleFS.open(F("/webconfig.json"), "r");
    if (file)
    {
        DeserializationError error = deserializeJson(doc, file);
        if (error)
        {
        // Serial.println(F("Failed to deserialize webconfig.json"));
        file.close();
        return;
        }
    }
    else
    {
        // Serial.println(F("Failed to read webconfig.json. Using defaults."));
    }

    showSectionTemperature = (doc.containsKey(F("SST")) ? doc[F("SST")] : true);
    showSectionDisplay = (doc.containsKey(F("SSD")) ? doc[F("SSD")] : true);
    showSectionControl = (doc.containsKey(F("SSC")) ? doc[F("SSC")] : true);
    showSectionButtons = (doc.containsKey(F("SSB")) ? doc[F("SSB")] : true);
    showSectionTimer = (doc.containsKey(F("SSTIM")) ? doc[F("SSTIM")] : true);
    showSectionTotals = (doc.containsKey(F("SSTOT")) ? doc[F("SSTOT")] : true);
    useControlSelector = (doc.containsKey(F("UCS")) ? doc[F("UCS")] : false);
}

/**
 * save "Web Config" json configuration to "webconfig.json"
 */
void saveWebConfig()
{
    File file = LittleFS.open(F("/webconfig.json"), "w");
    if (!file)
    {
        // Serial.println(F("Failed to save webconfig.json"));
        return;
    }

    // DynamicJsonDocument doc(256);
    StaticJsonDocument<256> doc;

    doc[F("SST")] = showSectionTemperature;
    doc[F("SSD")] = showSectionDisplay;
    doc[F("SSC")] = showSectionControl;
    doc[F("SSB")] = showSectionButtons;
    doc[F("SSTIM")] = showSectionTimer;
    doc[F("SSTOT")] = showSectionTotals;
    doc[F("UCS")] = useControlSelector;

    if (serializeJson(doc, file) == 0)
    {
        // Serial.println(F("{\"error\": \"Failed to serialize file\"}"));
    }
    file.close();
}

/**
 * response for /getwebconfig/
 * web server prints a json document
 */
void handleGetWebConfig()
{
    if (!checkHttpPost(server->method())) return;

    // DynamicJsonDocument doc(256);
    StaticJsonDocument<256> doc;

    doc[F("SST")] = showSectionTemperature;
    doc[F("SSD")] = showSectionDisplay;
    doc[F("SSC")] = showSectionControl;
    doc[F("SSB")] = showSectionButtons;
    doc[F("SSTIM")] = showSectionTimer;
    doc[F("SSTOT")] = showSectionTotals;
    doc[F("UCS")] = useControlSelector;

    String json;
    if (serializeJson(doc, json) == 0)
    {
        json = F("{\"error\": \"Failed to serialize webcfg\"}");
    }
    server->send(200, F("application/json"), json);
}

/**
 * response for /setwebconfig/
 * web server writes a json document
 */
void handleSetWebConfig()
{
    if (!checkHttpPost(server->method())) return;

    // DynamicJsonDocument doc(256);
    StaticJsonDocument<256> doc;
    String message = server->arg(0);
    DeserializationError error = deserializeJson(doc, message);
    if (error)
    {
        // Serial.println(F("Failed to read config file"));
        server->send(400, F("text/plain"), F("Error deserializing message"));
        return;
    }

    showSectionTemperature = doc[F("SST")];
    showSectionDisplay = doc[F("SSD")];
    showSectionControl = doc[F("SSC")];
    showSectionButtons = doc[F("SSB")];
    showSectionTimer = doc[F("SSTIM")];
    showSectionTotals = doc[F("SSTOT")];
    useControlSelector = doc[F("UCS")];

    saveWebConfig();

    server->send(200, F("text/plain"), "");
}

/**
 * load WiFi json configuration from "wifi.json"
 */
sWifi_info loadWifi()
{
    sWifi_info wifi_info;
    File file = LittleFS.open(F("/wifi.json"), "r");
    if (!file)
    {
        // Serial.println(F("Failed to read wifi.json. Using defaults."));
        return wifi_info;
    }

    DynamicJsonDocument doc(1024);

    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
        // Serial.println(F("Failed to deserialize wifi.json"));
        file.close();
        return wifi_info;
    }

    wifi_info.enableAp = doc[F("enableAp")];
    if(doc.containsKey(F("enableWM"))) wifi_info.enableWmApFallback = doc[F("enableWM")];
    wifi_info.apSsid = doc[F("apSsid")].as<String>();
    wifi_info.apPwd = doc[F("apPwd")].as<String>();

    wifi_info.enableStaticIp4 = doc[F("enableStaticIp4")];
    String s(30);
    wifi_info.ip4Address_str = doc[F("ip4Address")].as<String>();
    wifi_info.ip4Gateway_str = doc[F("ip4Gateway")].as<String>();
    wifi_info.ip4Subnet_str = doc[F("ip4Subnet")].as<String>();
    wifi_info.ip4DnsPrimary_str = doc[F("ip4DnsPrimary")].as<String>();
    wifi_info.ip4DnsSecondary_str = doc[F("ip4DnsSecondary")].as<String>();
    wifi_info.ip4NTP_str = doc[F("ip4NTP")].as<String>();

    return wifi_info;

    // ip4Address[0] = doc[F("ip4Address")][0];
    // ip4Address[1] = doc[F("ip4Address")][1];
    // ip4Address[2] = doc[F("ip4Address")][2];
    // ip4Address[3] = doc[F("ip4Address")][3];
    // ip4Gateway[0] = doc[F("ip4Gateway")][0];
    // ip4Gateway[1] = doc[F("ip4Gateway")][1];
    // ip4Gateway[2] = doc[F("ip4Gateway")][2];
    // ip4Gateway[3] = doc[F("ip4Gateway")][3];
    // ip4Subnet[0] = doc[F("ip4Subnet")][0];
    // ip4Subnet[1] = doc[F("ip4Subnet")][1];
    // ip4Subnet[2] = doc[F("ip4Subnet")][2];
    // ip4Subnet[3] = doc[F("ip4Subnet")][3];
    // ip4DnsPrimary[0] = doc[F("ip4DnsPrimary")][0];
    // ip4DnsPrimary[1] = doc[F("ip4DnsPrimary")][1];
    // ip4DnsPrimary[2] = doc[F("ip4DnsPrimary")][2];
    // ip4DnsPrimary[3] = doc[F("ip4DnsPrimary")][3];
    // ip4DnsSecondary[0] = doc[F("ip4DnsSecondary")][0];
    // ip4DnsSecondary[1] = doc[F("ip4DnsSecondary")][1];
    // ip4DnsSecondary[2] = doc[F("ip4DnsSecondary")][2];
    // ip4DnsSecondary[3] = doc[F("ip4DnsSecondary")][3];
}

/**
 * save WiFi json configuration to "wifi.json"
 */
void saveWifi(const sWifi_info& wifi_info)
{
    File file = LittleFS.open(F("/wifi.json"), "w");
    if (!file)
    {
        // Serial.println(F("Failed to save wifi.json"));
        return;
    }

    DynamicJsonDocument doc(1024);

    doc[F("enableAp")] = wifi_info.enableAp;
    doc[F("enableWM")] = wifi_info.enableWmApFallback;
    doc[F("apSsid")] = wifi_info.apSsid;
    doc[F("apPwd")] = wifi_info.apPwd;
    doc[F("enableStaticIp4")] = wifi_info.enableStaticIp4;
    doc[F("ip4Address")] = wifi_info.ip4Address_str;
    doc[F("ip4Gateway")] = wifi_info.ip4Gateway_str;
    doc[F("ip4Subnet")] = wifi_info.ip4Subnet_str;
    doc[F("ip4DnsPrimary")] = wifi_info.ip4DnsPrimary_str;
    doc[F("ip4DnsSecondary")] = wifi_info.ip4DnsSecondary_str;
    doc[F("ip4NTP")] = wifi_info.ip4NTP_str;

    if (serializeJson(doc, file) == 0)
    {
        // Serial.println(F("{\"error\": \"Failed to serialize file\"}"));
    }
    file.close();
}

/**
 * response for /getwifi/
 * web server prints a json document
 */
void handleGetWifi()
{
    if (!checkHttpPost(server->method())) return;

    DynamicJsonDocument doc(1024);

    sWifi_info wifi_info;
    wifi_info = loadWifi();

    doc[F("enableAp")] = wifi_info.enableAp;
    doc[F("enableWM")] = wifi_info.enableWmApFallback;
    doc[F("apSsid")] = wifi_info.apSsid;
    doc[F("apPwd")] = F("<enter password>");
    if (!hidePasswords)
    {
        doc[F("apPwd")] = wifi_info.apPwd;
    }

    doc[F("enableStaticIp4")] = wifi_info.enableStaticIp4;
    doc[F("ip4Address")] = wifi_info.ip4Address_str;
    doc[F("ip4Gateway")] = wifi_info.ip4Gateway_str;
    doc[F("ip4Subnet")] = wifi_info.ip4Subnet_str;
    doc[F("ip4DnsPrimary")] = wifi_info.ip4DnsPrimary_str;
    doc[F("ip4DnsSecondary")] = wifi_info.ip4DnsSecondary_str;
    doc[F("ip4NTP")] = wifi_info.ip4NTP_str;
    String json;
    if (serializeJson(doc, json) == 0)
    {
        json = F("{\"error\": \"Failed to serialize message\"}");
    }
    server->send(200, F("application/json"), json);
}

/**
 * response for /setwifi/
 * web server writes a json document
 */
void handleSetWifi()
{
    if (!checkHttpPost(server->method())) return;

    DynamicJsonDocument doc(1024);
    String message = server->arg(0);
    DeserializationError error = deserializeJson(doc, message);
    if (error)
    {
        // Serial.println(F("Failed to read config file"));
        server->send(400, F("text/plain"), F("Error deserializing message"));
        return;
    }

    sWifi_info wifi_info;

    wifi_info.enableAp = doc[F("enableAp")];
    if(doc.containsKey("enableWM")) wifi_info.enableWmApFallback = doc[F("enableWM")];
    wifi_info.apSsid = doc[F("apSsid")].as<String>();
    wifi_info.apPwd = doc[F("apPwd")].as<String>();

    wifi_info.enableStaticIp4 = doc[F("enableStaticIp4")];
    wifi_info.ip4Address_str = doc[F("ip4Address")].as<String>();
    wifi_info.ip4Gateway_str = doc[F("ip4Gateway")].as<String>();
    wifi_info.ip4Subnet_str = doc[F("ip4Subnet")].as<String>();
    wifi_info.ip4DnsPrimary_str = doc[F("ip4DnsPrimary")].as<String>();
    wifi_info.ip4DnsSecondary_str = doc[F("ip4DnsSecondary")].as<String>();
    wifi_info.ip4NTP_str = doc[F("ip4NTP")].as<String>();

    saveWifi(wifi_info);

    server->send(200, F("text/plain"), "");
}

/*
 * response for /resetwifi/
 * do this before giving away the device (be aware of other credentials e.g. MQTT)
 * a complete flash erase should do the job but remember to upload the filesystem as well.
 */
void handleResetWifi()
{
    server->send(200, F("text/html"), F("WiFi connection reset (erase) ..."));
    // Serial.println(F("WiFi connection reset (erase) ..."));
    resetWiFi();

    server->send(200, F("text/html"), F("WiFi connection reset (erase) ... done."));
    // Serial.println(F("WiFi connection reset (erase) ... done."));
    // Serial.println(F("ESP reset ..."));
    #if defined(ESP8266)
    ESP.reset();
    #else
    ESP.restart();
    #endif
}

void resetWiFi()
{
    sWifi_info wifi_info;
    wifi_info.enableAp = false;
    wifi_info.enableWmApFallback = true;
    wifi_info.apSsid = F("empty");
    wifi_info.apPwd = F("empty");
    saveWifi(wifi_info);
    delay(3000);
    periodicTimer.detach();
    updateMqttTimer.detach();
    updateWSTimer.detach();
    bwc->stop();
    bwc->saveSettings();
    delay(1000);
#if defined(ESP8266)
    ESP.eraseConfig();
#endif
    delay(1000);
    ESP_WiFiManager wm;
    wm.resetSettings();
    //WiFi.disconnect();
    delay(1000);
}

/**
 * load MQTT json configuration from "mqtt.json"
 */
void loadMqtt()
{
    File file = LittleFS.open("mqtt.json", "r");
    if (!file)
    {
        Serial.println(F("Failed to read mqtt.json. Using defaults."));
        return;
    }

    DynamicJsonDocument doc(1024);

    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
        // Serial.println(F("Failed to deserialize mqtt.json."));
        file.close();
        return;
    }

    useMqtt = doc[F("enableMqtt")];
    // enableMqtt = useMqtt; //will be set with start complete timer
    mqttIpAddress[0] = doc[F("mqttIpAddress")][0];
    mqttIpAddress[1] = doc[F("mqttIpAddress")][1];
    mqttIpAddress[2] = doc[F("mqttIpAddress")][2];
    mqttIpAddress[3] = doc[F("mqttIpAddress")][3];
    mqttPort = doc[F("mqttPort")];
    mqttUsername = doc[F("mqttUsername")].as<String>();
    mqttPassword = doc[F("mqttPassword")].as<String>();
    mqttClientId = doc[F("mqttClientId")].as<String>();
    mqttBaseTopic = doc[F("mqttBaseTopic")].as<String>();
    mqttTelemetryInterval = doc[F("mqttTelemetryInterval")];
}

/**
 * save MQTT json configuration to "mqtt.json"
 */
void saveMqtt()
{
    File file = LittleFS.open("mqtt.json", "w");
    if (!file)
    {
        // Serial.println(F("Failed to save mqtt.json"));
        return;
    }

    DynamicJsonDocument doc(1024);

    doc[F("enableMqtt")] = useMqtt;
    doc[F("mqttIpAddress")][0] = mqttIpAddress[0];
    doc[F("mqttIpAddress")][1] = mqttIpAddress[1];
    doc[F("mqttIpAddress")][2] = mqttIpAddress[2];
    doc[F("mqttIpAddress")][3] = mqttIpAddress[3];
    doc[F("mqttPort")] = mqttPort;
    doc[F("mqttUsername")] = mqttUsername;
    doc[F("mqttPassword")] = mqttPassword;
    doc[F("mqttClientId")] = mqttClientId;
    doc[F("mqttBaseTopic")] = mqttBaseTopic;
    doc[F("mqttTelemetryInterval")] = mqttTelemetryInterval;

    if (serializeJson(doc, file) == 0)
    {
        // Serial.println(F("{\"error\": \"Failed to serialize file\"}"));
    }
    file.close();
}

/**
 * response for /getmqtt/
 * web server prints a json document
 */
void handleGetMqtt()
{
    if (!checkHttpPost(server->method())) return;

    DynamicJsonDocument doc(1024);

    doc[F("enableMqtt")] = useMqtt;
    doc[F("mqttIpAddress")][0] = mqttIpAddress[0];
    doc[F("mqttIpAddress")][1] = mqttIpAddress[1];
    doc[F("mqttIpAddress")][2] = mqttIpAddress[2];
    doc[F("mqttIpAddress")][3] = mqttIpAddress[3];
    doc[F("mqttPort")] = mqttPort;
    doc[F("mqttUsername")] = mqttUsername;
    doc[F("mqttPassword")] = "<enter password>";
    if (!hidePasswords)
    {
        doc[F("mqttPassword")] = mqttPassword;
    }
    doc[F("mqttClientId")] = mqttClientId;
    doc[F("mqttBaseTopic")] = mqttBaseTopic;
    doc[F("mqttTelemetryInterval")] = mqttTelemetryInterval;

    String json;
    if (serializeJson(doc, json) == 0)
    {
        json = F("{\"error\": \"Failed to serialize message\"}");
    }
    server->send(200, F("text/plain"), json);
}

/**
 * response for /setmqtt/
 * web server writes a json document
 */
void handleSetMqtt()
{
    if (!checkHttpPost(server->method())) return;

    DynamicJsonDocument doc(1024);
    String message = server->arg(0);
    DeserializationError error = deserializeJson(doc, message);
    if (error)
    {
        // Serial.println(F("Failed to read config file"));
        server->send(400, F("text/plain"), F("Error deserializing message"));
        return;
    }

    useMqtt = doc[F("enableMqtt")];
    enableMqtt = useMqtt;
    mqttIpAddress[0] = doc[F("mqttIpAddress")][0];
    mqttIpAddress[1] = doc[F("mqttIpAddress")][1];
    mqttIpAddress[2] = doc[F("mqttIpAddress")][2];
    mqttIpAddress[3] = doc[F("mqttIpAddress")][3];
    mqttPort = doc[F("mqttPort")];
    mqttUsername = doc[F("mqttUsername")].as<String>();
    mqttPassword = doc[F("mqttPassword")].as<String>();
    mqttClientId = doc[F("mqttClientId")].as<String>();
    mqttBaseTopic = doc[F("mqttBaseTopic")].as<String>();
    mqttTelemetryInterval = doc[F("mqttTelemetryInterval")];

    server->send(200, F("text/plain"), "");

    saveMqtt();
    startMqtt();
}

/**
 * response for /dir/
 * web server prints a list of files
 */
void handleDir()
{
    HeapSelectIram ephemeral;
    Serial.printf_P(PSTR("dir IRamheap %d\n"), ESP.getFreeHeap());

    String mydir;
    mydir.reserve(128);
    server->setContentLength(CONTENT_LENGTH_UNKNOWN);
    server->send(200, F("text/html"), "");
    Dir root = LittleFS.openDir("/");
    while (root.next())
    {
        // Serial.println(root.fileName());
        String href = root.fileName();
        if (href.endsWith(".gz")) href.remove(href.length()-3);
        mydir += F("<a href=\"/");
        mydir +=href;
        mydir += F("\">");
        mydir += root.fileName();
        mydir += F("</a>");
        mydir += F(" Size: ");
        mydir += String(root.fileSize());
        mydir += F(" Bytes ");
        mydir += F(" <a href=\"/remove/?FileToRemove=");
        mydir += root.fileName();
        mydir += F("\">remove</a><br>");
        server->sendContent(mydir);
        mydir.clear();
    }
    server->sendContent("");
}

/**
 * response for /upload.html
 * upload a new file to the LittleFS
 */
void handleFileUpload()
{
    HTTPUpload& upload = server->upload();
    String path;
    if (upload.status == UPLOAD_FILE_START)
    {
        path = upload.filename;
        if (!path.startsWith("/"))
        {
            path = "/" + path;
        }

        // The file server always prefers a compressed version of a file
        if (!path.endsWith(".gz"))
        {
            // So if an uploaded file is not compressed, the existing compressed
            String pathWithGz = path + ".gz";
            // version of that file must be deleted (if it exists)
            if (LittleFS.exists(pathWithGz))
            {
                LittleFS.remove(pathWithGz);
            }
        }

        Serial.print(F("handleFileUpload Name: "));
        Serial.println(path);

        // Open the file for writing in LittleFS (create if it doesn't exist)
        fsUploadFile = LittleFS.open(path, "w");
        path = String();
    }
    else if (upload.status == UPLOAD_FILE_WRITE)
    {
        if (fsUploadFile)
        {
            // Write the received bytes to the file
            fsUploadFile.write(upload.buf, upload.currentSize);
            // Serial.print("file write ");
            // Serial.println(path);
        }
    }
    else if (upload.status == UPLOAD_FILE_END)
    {
        if (fsUploadFile)
        {
            fsUploadFile.close();
            Serial.print(F("handleFileUpload Size: "));
            Serial.println(upload.totalSize);
            server->sendHeader(F("location"), F("success.html"));
            server->send(303);
            if (upload.filename == "cmdq.json")
            {
                bwc->reloadCommandQueue();
            }
            if (upload.filename == "settings.json")
            {
                bwc->reloadSettings();
            }
        }
        else
        {
            Serial.println(F("err: 500"));
            server->send(500, F("text/plain"), F("500: couldn't create file"));
        }
    }
    else
    {
        Serial.print(F("upload status"));
        Serial.println(upload.status);
        server->send(500, F("text/plain"), F("500: upload aborted"));
    }
}

/**
 * response for /remove.html
 * delete a file from the LittleFS
 */
void handleFileRemove()
{
    String path;
    path = server->arg(F("FileToRemove"));
    if (!path.startsWith("/"))
    {
        path = "/" + path;
    }

    // Serial.print(F("handleFileRemove Name: "));
    // Serial.println(path);

    if (LittleFS.exists(path) && LittleFS.remove(path))
    {
        // Serial.print(F("handleFileRemove success: "));
        // Serial.println(path);
        if(server->method() == HTTP_GET)
            server->sendHeader(F("Location"), F("/dir/"));
        else
            server->sendHeader(F("Location"), F("/success.html"));
        server->send(303);
    }
    else
    {
        // Serial.print(F("handleFileRemove error: "));
        // Serial.println(path);
        server->send(500, F("text/plain"), F("500: couldn't delete file"));
    }
}

/**
 * response for /restart/
 */
void handleRestart()
{
    server->send(200, F("text/html"), F("ESP restart ..."));

    server->sendHeader(F("Location"), "/");
    server->send(303);

    delay(1000);
    stopall();
    delay(1000);
    Serial.println(F("ESP restart ..."));
    ESP.restart();
    delay(3000);
}

void updateStart(){
    Serial.println(F("update start"));
}
void updateEnd(){
    Serial.println(F("update finish"));
}
void udpateProgress(int cur, int total){
    Serial.printf_P(PSTR("update process at %d of %d bytes...\n"), cur, total);
}
void updateError(int err){
    Serial.printf_P(PSTR("update fatal error code %d\n"), err);
}

/**
 * MQTT setup and connect
 * @author 877dev
 */
void startMqtt()
{
    {
        HeapSelectIram ephemeral;
        Serial.printf_P(PSTR("IRamheap %d\n"), ESP.getFreeHeap());
        Serial.println(F("startmqtt"));
        if(!aWifiClient) aWifiClient = new WiFiClient;
        if(!mqttClient) mqttClient = new PubSubClient(*aWifiClient);
    

        // load mqtt credential file if it exists, and update default strings
        loadMqtt();

        // disconnect in case we are already connected
        mqttClient->disconnect();

        // setup MQTT broker information as defined earlier
        mqttClient->setServer(mqttIpAddress, mqttPort);
        // set buffer for larger messages, new to library 2.8.0
        if (mqttClient->setBufferSize(1536))
        {
            // Serial.println(F("MQTT > Buffer size successfully increased"));
        }
        mqttClient->setKeepAlive(60);
        mqttClient->setSocketTimeout(30);
        // set callback details
        // this function is called automatically whenever a message arrives on a subscribed topic.
        mqttClient->setCallback(mqttCallback);
        // Connect to MQTT broker, publish Status/MAC/count, and subscribe to keypad topic.
    }
    mqttConnect();
}

/**
 * MQTT callback function
 * @author 877dev
 */
void mqttCallback(char* topic, byte* payload, unsigned int length)
{
    // Serial.print(F("MQTT > Message arrived ["));
    // Serial.print(topic);
    // Serial.print(")] ");
    for (unsigned int i = 0; i < length; i++)
    {
        // Serial.print((char)payload[i]);
    }
    // Serial.println();
    if (String(topic).equals(String(mqttBaseTopic) + F("/command")))
    {
        // DynamicJsonDocument doc(256);
        StaticJsonDocument<256> doc;
        String message = (const char *) &payload[0];
        DeserializationError error = deserializeJson(doc, message);
        if (error)
        {
            return;
        }

        Commands command = doc[F("CMD")];
        int64_t value = doc[F("VALUE")];
        int64_t xtime = doc[F("XTIME")];
        int64_t interval = doc[F("INTERVAL")];
        String txt = doc[F("TXT")] | "";
        command_que_item item;
        item.cmd = command;
        item.val = value;
        item.xtime = xtime;
        item.interval = interval;
        item.text = txt;
        bwc->add_command(item);
        return;
    }

    /* author @malfurion, edited by @visualapproach for v4 */
    if (String(topic).equals(String(mqttBaseTopic) + F("/command_batch")))
    {
        DynamicJsonDocument doc(1024);
        String message = (const char *) &payload[0];
        DeserializationError error = deserializeJson(doc, message);
        if (error)
        {
            return;
        }

        JsonArray commandArray = doc.as<JsonArray>();

        for (JsonVariant commandItem : commandArray) {
            Commands command = commandItem[F("CMD")];
            int64_t value = commandItem[F("VALUE")];
            int64_t xtime = commandItem[F("XTIME")];
            int64_t interval = commandItem[F("INTERVAL")];
            String txt = doc[F("TXT")] | "";
            command_que_item item;
            item.cmd = command;
            item.val = value;
            item.xtime = xtime;
            item.interval = interval;
            item.text = txt;
            bwc->add_command(item);
        }

        return;
    }

    if (String(topic).equals(String(mqttBaseTopic) + F("/set_config")))
    {
        String message = (const char *) &payload[0];    
        bwc->setJSONSettings(message);
        send_mqtt_cfg_needed = true;
    }
}

/**
 * Connect to MQTT broker, publish Status/MAC/count, and subscribe to keypad topic.
 */
void mqttConnect()
{
    // do not connect if MQTT is not enabled
    if (!enableMqtt)
    {
        return;
    }
    Serial.println(F("mqttconn"));

    // Serial.print(F("MQTT > Connecting ... "));
    // We'll connect with a Retained Last Will that updates the 'Status' topic with "Dead" when the device goes offline...
    if (mqttClient->connect(
        mqttClientId.c_str(), // client_id : the client ID to use when connecting to the server->
        mqttUsername.c_str(), // username : the username to use. If NULL, no username or password is used (const char[])
        mqttPassword.c_str(), // password : the password to use. If NULL, no password is used (const char[])setupHA
        (String(mqttBaseTopic) + F("/Status")).c_str(), // willTopic : the topic to be used by the will message (const char[])
        0, // willQoS : the quality of service to be used by the will message (int : 0,1 or 2)
        1, // willRetain : whether the will should be published with the retain flag (int : 0 or 1)
        "Dead")) // willMessage : the payload of the will message (const char[])
    {
        // Serial.println(F("success!"));
        mqtt_connect_count++;

        // update MQTT every X seconds. (will also be updated on state changes)
        updateMqttTimer.attach(mqttTelemetryInterval, []{ sendMQTTFlag = true; });

        // These all have the Retained flag set to true, so that the value is stored on the server and can be retrieved at any point
        // Check the 'Status' topic to see that the device is still online before relying on the data from these retained topics
        mqttClient->publish((String(mqttBaseTopic) + F("/Status")).c_str(), "Alive", true);
        mqttClient->publish((String(mqttBaseTopic) + F("/MAC_Address")).c_str(), WiFi.macAddress().c_str(), true);                 // Device MAC Address
        mqttClient->publish((String(mqttBaseTopic) + F("/MQTT_Connect_Count")).c_str(), String(mqtt_connect_count).c_str(), true); // MQTT Connect Count
        mqttClient->loop();

        // Watch the 'command' topic for incoming MQTT messages
        mqttClient->subscribe((String(mqttBaseTopic) + F("/command")).c_str());
        mqttClient->subscribe((String(mqttBaseTopic) + F("/command_batch")).c_str());
        mqttClient->subscribe((String(mqttBaseTopic) + F("/set_config")).c_str());
        mqttClient->loop();

        #ifdef ESP8266
        // mqttClient->publish((String(mqttBaseTopic) + "/reboot_time").c_str(), DateTime.format(DateFormatter::SIMPLE).c_str(), true);
        mqttClient->publish((String(mqttBaseTopic) + F("/reboot_time")).c_str(), (bwc->reboot_time_str+'Z').c_str(), true);
        mqttClient->publish((String(mqttBaseTopic) + F("/reboot_reason")).c_str(), ESP.getResetReason().c_str(), true);
        String buttonname;
        buttonname.reserve(32);
        bwc->getButtonName(buttonname);
        mqttClient->publish((String(mqttBaseTopic) + F("/button")).c_str(), buttonname.c_str(), true);
        mqttClient->loop();
        sendMQTT();
        Serial.println(F("MQTT Sending HA discovery"));
        setupHA();
        mqttClient->setBufferSize(512);
        mqttClient->loop();
        // Serial.println(F("MQTT Sending config"));
        // sendMQTTConfig();    // Stack smashing if doing this here :-(
        send_mqtt_cfg_needed = true;
        Serial.println(F("done"));
        #endif
    }
    else
    {
        // Serial.print(F("failed, Return Code = "));
        // Serial.println(mqttClient->state()); // states explained in webSocket->js
    }
    Serial.println(F("end mqttcon"));
}

time_t getBootTime()
{
    time_t seconds = millis() / 1000;
    time_t result = time(nullptr) - seconds;
    return result;
}

void handleESPInfo()
{
    #ifdef ESP8266
    char stack;
    uint32_t stacksize = stack_start - &stack;
    size_t const BUFSIZE = 1024;
    char response[BUFSIZE];

    char const *response_dram =
    PSTR(
    "Stack size:               %u \n"
    "Free Dram Heap:           %u \n"
    "Min Dram Heap:            %u \n"
    "Max free Dram block size: %u \n\n");

    char const *response_iram =
    PSTR(
    "Free Iram Heap:           %u \n"
    "Max free Iram block size: %u \n\n"
    "Core version:             %s \n"
    "CPU fq:                   %u MHz\n"
    "Cycle count:              %u \n"
    "Free cont stack:          %u \n"
    "Sketch size:              %u \n"
    "Free sketch space:        %u \n"
    );

    server->setContentLength(CONTENT_LENGTH_UNKNOWN);
    server->send(200, F("text/plain"), "");

    snprintf_P(response, BUFSIZE, response_dram,
        stacksize,
        ESP.getFreeHeap(),
        heap_water_mark,
        ESP.getMaxFreeBlockSize() );
    server->sendContent(response);
    uint32_t iram_heap; 
    uint32_t iram_maxblock;
    {
        HeapSelectIram ephemeral;
        iram_heap = ESP.getFreeHeap();
        iram_maxblock = ESP.getMaxFreeBlockSize();
    }
    snprintf_P(response, BUFSIZE, response_iram,
        iram_heap,
        iram_maxblock,
        ESP.getCoreVersion().c_str(),
        ESP.getCpuFreqMHz(),
        ESP.getCycleCount(),
        ESP.getFreeContStack(),
        ESP.getSketchSize(),
        ESP.getFreeSketchSpace()
    );
    
    server->sendContent(response);
    server->sendContent("");

    Serial.println(F("end info"));
    #endif
}

void setTemperatureFromSensor()
{
    if(bwc->hasTempSensor)
    { 
            tempSensors->requestTemperatures(); 
            float temperatureC = tempSensors->getTempCByIndex(0);
            //float temperatureF = tempSensors.getTempFByIndex(0);
            //Serial.print(temperatureC);
            //Serial.println("ºC");
            //Serial.print(temperatureF);
            //Serial.println("ºF");

            // Ignore bad reads
            if(temperatureC >= -20.0)
            {
                bwc->setAmbientTemperature(temperatureC, true);
            }
    }
}

#include "ha.txt"
#include "prometheus.txt"
