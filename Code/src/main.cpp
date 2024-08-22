#include "main.h"
#include "ports.h"

#define WS_PERIOD 4.0
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
    gotIP_flag = true;
}

void gotIP()
{
    ESP.wdtFeed();
    BWC_LOG_P(PSTR("start of gotip millis = %d\n"), millis());
    gotIP_flag = false;
    WiFi.softAPdisconnect();
    WiFi.mode(WIFI_STA);
    BWC_LOG_P(PSTR("Soft AP > closed\n"), 0);
    BWC_LOG_P(PSTR("Connected as station with localIP: %s\n"), WiFi.localIP().toString().c_str());
    startNTP();
    // startOTA();
    // startMqtt();
    BWC_LOG_P(PSTR("end of gotip millis = %d\n"), millis());
    bwc->print(WiFi.localIP().toString());
    if(mqtt_info->useMqtt) enableMqtt = true;
    BWC_YIELD;
}

void cb_disconnected(const WiFiEventStationModeDisconnected& event)
{
    disconnected_flag = true;
    // startSoftAp();
}

void setup()
{
    
    // init record of stack
    char stack;
    stack_start = &stack;

    Serial.begin(76800);
    BWC_LOG_P(PSTR("\nSetup > Start @ millis: %d\n"),millis());
    /*register wifi events */
    gotIpEventHandler = WiFi.onStationModeGotIP(cb_gotIP);
    disconnectedEventHandler = WiFi.onStationModeDisconnected(cb_disconnected);

    LittleFS.begin();
    {
        HeapSelectIram ephemeral;
        bwc = new BWC;
        oneWire = new OneWire(231);
        tempSensors = new DallasTemperature(oneWire);
        // bootlogTimer = new Ticker;
        periodicTimer = new Ticker;
        startComplete_ticker = new Ticker;
        ntpCheck_ticker = new Ticker;
        // checkWifi_ticker = new Ticker;
        updateWSTimer = new Ticker;
        updateMqttTimer = new Ticker;
        mqtt_info = new sMQTT_info;
        mqtt_info->mqttBaseTopic = MQTT_BASE_TOPIC_F;
        mqtt_info->mqttClientId = MQTT_CLIENT_ID_F;
        mqtt_info->mqttHost = F("192.168.0.20");
        mqtt_info->mqttPassword = MQTT_PASSWORD_F;
        mqtt_info->mqttPort = 1883;
        mqtt_info->mqttTelemetryInterval = 600;
        mqtt_info->mqttUsername = MQTT_USER_F;
        mqtt_info->useMqtt = true;
        wifi_info = new sWifi_info{.enableWmApFallback = true};
    }
    bwc->setup();
    bwc->loop();
    periodicTimer->attach(periodicTimerInterval, []{ periodicTimerFlag = true; });
    // delayed mqtt start
    startComplete_ticker->attach(30, []{ bwc->restoreStates(); startComplete_ticker->detach(); delete startComplete_ticker; }); //can it destroy itself?
    // update webpage every WS_PERIOD seconds. (will also be updated on state changes)
    updateWSTimer->attach(WS_PERIOD, []{ sendWSFlag = true; });
    loadWebConfig();
    startWiFi();
    if(wifi_info->enableWmApFallback) startSoftAp(); // not blocking anymore so no use case should exist for this to be turned off.
    startHttpServer();
    startWebSocket();
    startOTA();
    startMqtt();
    if(bwc->hasTempSensor)
    { 
        HeapSelectIram ephemeral;
        oneWire->begin(bwc->tempSensorPin);
        tempSensors->begin();
    }
    bwc->print("---");  //No overloaded function exists for the F() macro
    bwc->print(FW_VERSION);
    BWC_LOG_P(PSTR("End of setup() @ Millis: %d @ line: %d. Heap: %d\n"), millis(), __LINE__, ESP.getFreeHeap());
    heap_water_mark = ESP.getFreeHeap();
}

void loop()
{
    uint32_t freeheap = ESP.getFreeHeap();
    if(freeheap < heap_water_mark) heap_water_mark = freeheap;

    if(gotIP_flag) gotIP();
    if(disconnected_flag) startSoftAp();
    // We need this self-destructing info several times, so save it on the stack
    bool newData = bwc->newData();
    // Fiddle with the pump computer
    bwc->loop();

    // listen for webserver events
    if(server){
        server->handleClient();
        // Serial.print(".");
    }
    // listen for OTA events
    ArduinoOTA.handle();
    // web socket
    if (newData || sendWSFlag)
    {
        sendWSFlag = false;
        sendWS();
    }
    // run only when a wifi connection is established
    /* MQTT, OTA & NTP is not relevant in softAP mode */
    if (WiFi.status() == WL_CONNECTED)
    {

        // MQTT
        if (enableMqtt && mqttClient->loop())
        {
            String msg;
            msg.reserve(32);
            bwc->getButtonName(msg);
            // publish pretty button name if display button is pressed (or NOBTN if released)
            if (!msg.equals(prevButtonName))
            {
                mqttClient->publish((String(mqtt_info->mqttBaseTopic) + "/button").c_str(), String(msg).c_str(), true);
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

        if(checkNTP_flag)
        {
            checkNTP_flag = false;
            checkNTP();
        }
    }

    // run every X seconds
    if (periodicTimerFlag)
    {
        periodicTimerFlag = false;
        if(WiFi.getMode() == WIFI_AP_STA)
        {
            wifi_manual_reconnect();
        }
        if (enableMqtt && !mqttClient->loop() && (WiFi.status() == WL_CONNECTED))
        {
            BWC_LOG_P(PSTR("MQTT > Not connected\n"),0);
            mqttConnect();
        }
        // Leverage the pre-existing periodicTimerFlag to also set temperature, if enabled
        setTemperatureFromSensor();
    }

    //Only do this if locked out! (by pressing POWER - LOCK - TIMER - POWER)
    if(bwc->getBtnSeqMatch())
    {   
        resetWiFi();
        delay(3000);
        ESP.reset();
        delay(3000);
    }
}


/**
 * Send status data to web client in JSON format (because it is easy to decode on the other side)
 */
void sendWS()
{
    if(!webSocket) return;
    if(webSocket->connectedClients() == 0) return;
    HeapSelectIram ephemeral;
    // Serial.printf("IRamheap %d\n", ESP.getFreeHeap());
    // send states
    String json;
    json.reserve(384);

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
    BWC_YIELD;
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
    BWC_YIELD;
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
    HeapSelectIram ephemeral;
    // Serial.printf("IRamheap %d\n", ESP.getFreeHeap());
    String json;
    json.reserve(320);

    // send states
    bwc->getJSONStates(json);
    if (mqttClient->publish((String(mqtt_info->mqttBaseTopic) + F("/message")).c_str(), String(json).c_str(), true))
    {
        BWC_LOG_P(PSTR("MQTT > message published\n"),0);
    }
    else
    {
        BWC_LOG_P(PSTR("MQTT > message not published\n"),0);
    }

    // send times
    json.clear();
    bwc->getJSONTimes(json);
    if (mqttClient->publish((String(mqtt_info->mqttBaseTopic) + F("/times")).c_str(), String(json).c_str(), true))
    {
        BWC_LOG_P(PSTR("MQTT > times published\n"),0);
    }
    else
    {
        BWC_LOG_P(PSTR("MQTT > times not published\n"),0);
    }

    //send other info
    json.clear();
    getOtherInfo(json);
    if (mqttClient->publish((String(mqtt_info->mqttBaseTopic) + F("/other")).c_str(), String(json).c_str(), true))
    {
        BWC_LOG_P(PSTR("MQTT > other published\n"),0);
    }
    else
    {
        BWC_LOG_P(PSTR("MQTT > other not published\n"),0);
    }
    BWC_YIELD;
}

void sendMQTTConfig()
{
    BWC_LOG_P(PSTR("MQTT > sending config\n"),0);
    String json;
    json.reserve(320);
    bwc->getJSONSettings(json);
    mqttClient->publish((String(mqtt_info->mqttBaseTopic) + F("/get_config")).c_str(), String(json).c_str(), true);
    mqttClient->loop();
    BWC_YIELD;
}

/**
 * Start a Wi-Fi access point, and try to connect to some given access points.
 * Then wait for either an AP or STA connection
 */
void startWiFi()
{
    BWC_LOG_P(PSTR("startWiFi() @ millis: %d\n"), millis());
    //WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);
    WiFi.persistent(true);
    WiFi.hostname(DEVICE_NAME_F);
    WiFi.mode(WIFI_STA); //WiFi.setOutputPower(15.0);
    loadWifi();

    if (wifi_info->enableStaticIp4)
    {
        BWC_LOG_P(PSTR("Setting static IP\n"),0);
        IPAddress ip4Address;
        IPAddress ip4Gateway;
        IPAddress ip4Subnet;
        IPAddress ip4DnsPrimary;
        IPAddress ip4DnsSecondary;
        ip4Address.fromString(wifi_info->ip4Address_str);
        ip4Gateway.fromString(wifi_info->ip4Gateway_str);
        ip4Subnet.fromString(wifi_info->ip4Subnet_str);
        ip4DnsPrimary.fromString(wifi_info->ip4DnsPrimary_str);
        ip4DnsSecondary.fromString(wifi_info->ip4DnsSecondary_str);
        BWC_LOG_P(PSTR("WiFi > using static IP %s on gateway %s\n"),ip4Address.toString().c_str(), ip4Gateway.toString().c_str());
        WiFi.config(ip4Address, ip4Gateway, ip4Subnet, ip4DnsPrimary, ip4DnsSecondary);
    }

    wifi_manual_reconnect();
    BWC_YIELD;
}

void wifi_manual_reconnect()
{
    /* Connect in station mode to the AP given (your router/ap) */
    if (wifi_info->enableAp)
    {
        BWC_LOG_P(PSTR("WiFi > using WiFi configuration with SSID %s\n"), wifi_info->apSsid.c_str());

        WiFi.begin(wifi_info->apSsid.c_str(), wifi_info->apPwd.c_str());
        // checkWifi_ticker->attach(2.0, checkWiFi_ISR);
        BWC_LOG_P(PSTR("WiFi > AP info loaded. Waiting for connection ...\n"), 0);
    }
    else
    {
        BWC_LOG_P(PSTR("WiFi > AP info not found. Using last known AP ...\n"), 0);
        WiFi.begin();
    }
}

/**
 * start WiFiManager configuration portal
 */
void startSoftAp()
{
    disconnected_flag = false;
    if(WiFi.getMode() == WIFI_AP_STA) {
        BWC_LOG_P(PSTR("Soft AP IP: %s.\n"),WiFi.softAPIP().toString().c_str());
        return;
    }
    BWC_LOG_P(PSTR("Station > disconnected. Starting soft AP\n"),0);
    bwc->print(F("check network"));
    WiFi.mode(WIFI_AP_STA);
    IPAddress local_IP(192,168,4,2);
    IPAddress gateway(192,168,4,1);
    IPAddress subnet(255,255,255,0);
    BWC_LOG_P(PSTR("WiFi > soft-AP configuration: %s\n"),WiFi.softAPConfig(local_IP, gateway, subnet) ? "OK" : "Failed!");
    BWC_LOG_P(PSTR("WiFi > soft AP mode: %s\n"),WiFi.softAP(WM_AP_NAME_F, WM_AP_PASSWORD_F)?"OK": "SoftAP fail");
    BWC_LOG_P(PSTR("WiFi > Soft AP IP: %s\n"),WiFi.softAPIP().toString().c_str());
    BWC_YIELD;
}

void checkNTP_ISR()
{
    checkNTP_flag = true;
}

void checkNTP()
{
    time_t now = time(nullptr);
    // static uint8_t ntpTryNumber = 0;
    if(now < 57600)
    {
        // if (++ntpTryNumber == 10) {
        //     ntpTryNumber = 0; //reset until next check
        //     ntpCheck_ticker->detach(); //give up. Next check won't happen.
        // }
        return;
    }
    ntpCheck_ticker->detach(); //time is set, don't check again
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    time_t boot_timestamp = getBootTime();
    tm * boot_time_tm = gmtime(&boot_timestamp);
    char boot_time_str[64];
    strftime(boot_time_str, 64, "%F %T", boot_time_tm);
    bwc->reboot_time_str = String(boot_time_str);
    bwc->reboot_time_t = boot_timestamp;
    if(firstNtpSyncAfterBoot)
    {
        BWC_LOG_P(PSTR("NTP > synced: %s. Saving boot info.\n"),bwc->reboot_time_str.c_str());
        firstNtpSyncAfterBoot = false;
        bwc->saveRebootInfo();
    }
    BWC_YIELD;
}

/**
 * start NTP sync
 */
void startNTP()
{
    BWC_LOG_P(PSTR("NTP > start\n"),0);
    configTime(0,0,wifi_info->ip4NTP_str, F("pool.ntp.org"), F("time.nist.gov"));
    ntpCheck_ticker->attach(3.0, checkNTP_ISR);
}

void startOTA()
{
    BWC_LOG_P(PSTR("OTA > start\n"),0);
    String dname = DEVICE_NAME_F;
    String pw = OTA_PSWD_F;
    ArduinoOTA.setHostname(dname.c_str());
    ArduinoOTA.setPassword(pw.c_str());

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
    BWC_LOG_P(PSTR("Stop all > Free mem before stop: %d\n"), ESP.getFreeHeap());
    bwc->stop();
    BWC_LOG_P(PSTR("MQTT > detaching\n"),0);
    updateMqttTimer->detach();
    BWC_LOG_P(PSTR("Periodic timer > detaching\n"),0);
    periodicTimer->detach();
    BWC_LOG_P(PSTR("WS > detaching\n"),0);
    updateWSTimer->detach();
    if(ntpCheck_ticker->active()) ntpCheck_ticker->detach();
    // if(checkWifi_ticker->active()) checkWifi_ticker->detach();
    //bwc->saveSettings();
    delete tempSensors;
    delete oneWire;
    BWC_LOG_P(PSTR("MQTT > stopping\n"),0);
    if(enableMqtt) mqttClient->disconnect();
    if(aWifiClient) delete aWifiClient;
    aWifiClient = nullptr;
    // delete mqttClient; //Compiler nagging about not deleting virtual classes.
    mqttClient = nullptr;
    BWC_LOG_P(PSTR("HTTPServer > stopping\n"),0);
    server->stop();
    delete server;
    server = nullptr;
    BWC_LOG_P(PSTR("WS > stopping\n"),0);
    webSocket->close();
    delete webSocket;
    webSocket = nullptr;
    BWC_LOG_P(PSTR("FS > stopping\n"),0);
    LittleFS.end();
    BWC_LOG_P(PSTR("Stop all > done. Free mem: %d\n"), ESP.getFreeHeap());
}

/*pause: action=true cont: action=false*/
void pause_all(bool action)
{
    if(action)
    {
        if(periodicTimer->active()) periodicTimer->detach();
        // if(startComplete_ticker->active()) startComplete_ticker->detach();
        if(updateWSTimer->active()) updateWSTimer->detach();
        // if(bootlogTimer->active()) bootlogTimer->detach();
        if(ntpCheck_ticker->active()) ntpCheck_ticker->detach();
    } else 
    {
        periodicTimer->attach(periodicTimerInterval, []{ periodicTimerFlag = true; });
        // startComplete_ticker->attach(60, []{ if(mqtt_info->useMqtt) enableMqtt = true; startComplete_ticker->detach(); });
        updateWSTimer->attach(WS_PERIOD, []{ sendWSFlag = true; });
        //bootlogTimer.attach(5, []{ if(DateTime.isTimeValid()) {bwc->saveRebootInfo(); bootlogTimer.detach();} });
    }
    bwc->pause_all(action);
}

void startWebSocket()
{
    HeapSelectIram ephemeral;
    BWC_LOG_P(PSTR("WS > start. IRam heap: %d\n"), ESP.getFreeHeap());
    if(webSocket != nullptr)
    {
        webSocket->disconnect();
        webSocket->close();
        delete webSocket;
        webSocket = nullptr;
    }
    webSocket = new WebSocketsServer(81);
    webSocket->begin();
    // webSocket->enableHeartbeat(11000, 5000, 2);
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
            BWC_LOG_P(PSTR("WS > [%u] Disconnected!\n"), num);
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
                BWC_LOG_P(PSTR("WS > JSON command failed"),0);
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
            BWC_LOG_P(PSTR("WebSocket > Type:[%u]\r\n"), (unsigned int)type);
        break;
    }
}

/**
 * start a HTTP server with a file read and upload handler
 */
void startHttpServer()
{
    BWC_LOG_P(PSTR("HTTP Server > start/restart.\n"),0);
    if(server != nullptr)
    {
        server->stop();
        server->close();
        delete server;
        server = nullptr;
    }

    {
        // HeapSelectIram ephemeral;
        server = new ESP8266WebServer(80);
        /* if you want a simple auth you can do something like this for every page you want to "protect" */
        // server->on(F("/"), []() {
        //     if (!server->authenticate("user", "pswd")) {
        //         return server->requestAuthentication();
        //     }
        //     handleNotFound();
        // });
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
    BWC_YIELD;
}

void handleSetHardware()
{
    if (!checkHttpPost(server->method())) return;
    String message = server->arg(0);
    File file = LittleFS.open(F("hwcfg.json"), "w");
    if (!file)
    {
        // Serial.println(F("Failed to save hwcfg.json"));
        return;
    }
    file.print(message);
    file.close();
    server->send(200, F("text/plain"), "ok");
    BWC_YIELD;
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
    server->send(200, F("text/plain"), "wait<br>");

    bwc->stop();
    preparefortest();

    /* 
        Log all edges to a file in HEAP RAM. When that log is full send to web client
    */

    unsigned long pin_states = 0, old_pin_states = 0; //to store result from READ_PERI_REG (GPIOs)
    unsigned long t; //timestamp - micros
    uint32_t edge_count = 0;
    const int array_len = 1024;
    unsigned long* p_input_log = new unsigned long[array_len*2];

    while(edge_count < array_len)
    {
        pin_states = READ_PERI_REG(PIN_IN); //mix unsigned long with uint32_t which is the same
        if(pin_states != old_pin_states)
        {
            t = micros();
            p_input_log[edge_count] = t; //log time
            p_input_log[edge_count + array_len] = pin_states; //log states (all gpios)
            edge_count++;
        }
        old_pin_states = pin_states;
        yield(); //keep the watchdog away and manage wifi etc. Unclear how much time we waste here...
    }

    /* send statistics to client */
    char s[128];
    sprintf_P(s, PSTR("micros, gpio registers\n"));
    server->sendContent(s);
    for(int i = 0; i < array_len; i++)
    {
        sprintf_P(s, PSTR("%u,%X\n"), p_input_log[i], p_input_log[i+array_len]);
        server->sendContent(s);
        yield(); //keep the watchdog away and manage wifi etc. Unclear how much time we waste here...
    }
    sprintf_P(s, PSTR("Cut and paste all above. Zip and post on forum for help.\n"));
    server->sendContent(s);
    server->sendContent("");

    delete [] p_input_log;

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
    String contentType = getContentType(path);                  // Get the MIME type
    String pathWithGz = path + ".gz";
    if (LittleFS.exists(pathWithGz) || LittleFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
        if (LittleFS.exists(pathWithGz))                        // If there's a compressed version available
            path += ".gz";                                      // Use the compressed version
        File file = LittleFS.open(path, "r");                   // Open the file
        size_t fsize = file.size();
        BWC_YIELD;
        size_t sent = server->streamFile(file, contentType);    // Send it to the client
        BWC_LOG_P(PSTR("File size: %d\n"),fsize);
        BWC_LOG_P(PSTR("HTTPServer > Filename: %s. Bytes sent: %d\n"),path.c_str(),sent);
        if(fsize != sent){
            BWC_LOG_P(PSTR("^^^^^ File not completed ^^^^^\n"),0);
        }
        pause_all(false);
        file.close();                                           // Close the file again
        return true;
    }
    pause_all(false);
    // If the file doesn't exist, return false
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
    BWC_YIELD;
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
    BWC_YIELD;
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
    BWC_YIELD;
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
    BWC_YIELD;
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
    BWC_YIELD;
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
    BWC_YIELD;
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
void loadWifi()
{
    File file = LittleFS.open(F("/wifi.json"), "r");
    if (!file)
    {
        // Serial.println(F("Failed to read wifi.json. Using defaults."));
        return;
    }

    DynamicJsonDocument doc(1024);

    DeserializationError error = deserializeJson(doc, file);
    if (error)
    {
        // Serial.println(F("Failed to deserialize wifi.json"));
        file.close();
        return;
    }

    wifi_info->enableAp = doc[F("enableAp")];
    if(doc.containsKey(F("enableWM"))) wifi_info->enableWmApFallback = doc[F("enableWM")];
    wifi_info->apSsid = doc[F("apSsid")].as<String>();
    wifi_info->apPwd = doc[F("apPwd")].as<String>();

    wifi_info->enableStaticIp4 = doc[F("enableStaticIp4")];
    String s(30);
    wifi_info->ip4Address_str = doc[F("ip4Address")].as<String>();
    wifi_info->ip4Gateway_str = doc[F("ip4Gateway")].as<String>();
    wifi_info->ip4Subnet_str = doc[F("ip4Subnet")].as<String>();
    wifi_info->ip4DnsPrimary_str = doc[F("ip4DnsPrimary")].as<String>();
    wifi_info->ip4DnsSecondary_str = doc[F("ip4DnsSecondary")].as<String>();
    wifi_info->ip4NTP_str = doc[F("ip4NTP")].as<String>();

    BWC_YIELD;
}

/**
 * save WiFi json configuration to "wifi.json"
 */
void saveWifi()
{
    File file = LittleFS.open(F("/wifi.json"), "w");
    if (!file)
    {
        // Serial.println(F("Failed to save wifi.json"));
        return;
    }

    DynamicJsonDocument doc(1024);

    doc[F("enableAp")] = wifi_info->enableAp;
    doc[F("enableWM")] = wifi_info->enableWmApFallback;
    doc[F("apSsid")] = wifi_info->apSsid;
    doc[F("apPwd")] = wifi_info->apPwd;
    doc[F("enableStaticIp4")] = wifi_info->enableStaticIp4;
    doc[F("ip4Address")] = wifi_info->ip4Address_str;
    doc[F("ip4Gateway")] = wifi_info->ip4Gateway_str;
    doc[F("ip4Subnet")] = wifi_info->ip4Subnet_str;
    doc[F("ip4DnsPrimary")] = wifi_info->ip4DnsPrimary_str;
    doc[F("ip4DnsSecondary")] = wifi_info->ip4DnsSecondary_str;
    doc[F("ip4NTP")] = wifi_info->ip4NTP_str;

    if (serializeJson(doc, file) == 0)
    {
        // Serial.println(F("{\"error\": \"Failed to serialize file\"}"));
    }
    file.close();
    BWC_YIELD;
}

/**
 * response for /getwifi/
 * web server prints a json document
 */
void handleGetWifi()
{
    if (!checkHttpPost(server->method())) return;

    DynamicJsonDocument doc(1024);

    doc[F("enableAp")] = wifi_info->enableAp;
    doc[F("enableWM")] = wifi_info->enableWmApFallback;
    doc[F("apSsid")] = wifi_info->apSsid;
    doc[F("apPwd")] = F("<enter password>");
    if (!hidePasswords)
    {
        doc[F("apPwd")] = wifi_info->apPwd;
    }

    doc[F("enableStaticIp4")] = wifi_info->enableStaticIp4;
    doc[F("ip4Address")] = wifi_info->ip4Address_str;
    doc[F("ip4Gateway")] = wifi_info->ip4Gateway_str;
    doc[F("ip4Subnet")] = wifi_info->ip4Subnet_str;
    doc[F("ip4DnsPrimary")] = wifi_info->ip4DnsPrimary_str;
    doc[F("ip4DnsSecondary")] = wifi_info->ip4DnsSecondary_str;
    doc[F("ip4NTP")] = wifi_info->ip4NTP_str;
    String json;
    json.reserve(200);
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

    wifi_info->enableAp = doc[F("enableAp")];
    if(doc.containsKey("enableWM")) wifi_info->enableWmApFallback = doc[F("enableWM")];
    wifi_info->apSsid = doc[F("apSsid")].as<String>();
    wifi_info->apPwd = doc[F("apPwd")].as<String>();

    wifi_info->enableStaticIp4 = doc[F("enableStaticIp4")];
    wifi_info->ip4Address_str = doc[F("ip4Address")].as<String>();
    wifi_info->ip4Gateway_str = doc[F("ip4Gateway")].as<String>();
    wifi_info->ip4Subnet_str = doc[F("ip4Subnet")].as<String>();
    wifi_info->ip4DnsPrimary_str = doc[F("ip4DnsPrimary")].as<String>();
    wifi_info->ip4DnsSecondary_str = doc[F("ip4DnsSecondary")].as<String>();
    wifi_info->ip4NTP_str = doc[F("ip4NTP")].as<String>();

    saveWifi();

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
    wifi_info->enableAp = false;
    wifi_info->enableWmApFallback = true;
    wifi_info->apSsid = F("empty");
    wifi_info->apPwd = F("empty");
    saveWifi();
    delay(3000);
    periodicTimer->detach();
    updateMqttTimer->detach();
    updateWSTimer->detach();
    if(ntpCheck_ticker->active()) ntpCheck_ticker->detach();
    bwc->saveSettings();
    bwc->stop();
    delay(1000);
#if defined(ESP8266)
    ESP.eraseConfig();
#endif
    delay(1000);
    // ESP_WiFiManager wm;
    // wm.resetSettings();
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
        BWC_LOG_P(PSTR("MQTT > Failed to read mqtt.json. Using defaults.\n"),0);
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

    mqtt_info->useMqtt = doc[F("enableMqtt")];
    // enableMqtt = useMqtt; //will be set with start complete timer
    mqtt_info->mqttHost = doc[F("mqttHost")].as<String>();
    mqtt_info->mqttPort = doc[F("mqttPort")];
    mqtt_info->mqttUsername = doc[F("mqttUsername")].as<String>();
    mqtt_info->mqttPassword = doc[F("mqttPassword")].as<String>();
    mqtt_info->mqttClientId = doc[F("mqttClientId")].as<String>();
    mqtt_info->mqttBaseTopic = doc[F("mqttBaseTopic")].as<String>();
    mqtt_info->mqttTelemetryInterval = doc[F("mqttTelemetryInterval")];
    BWC_YIELD;
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

    doc[F("enableMqtt")] = mqtt_info->useMqtt;
    doc[F("mqttHost")] = mqtt_info->mqttHost;
    doc[F("mqttPort")] = mqtt_info->mqttPort;
    doc[F("mqttUsername")] = mqtt_info->mqttUsername;
    doc[F("mqttPassword")] = mqtt_info->mqttPassword;
    doc[F("mqttClientId")] = mqtt_info->mqttClientId;
    doc[F("mqttBaseTopic")] = mqtt_info->mqttBaseTopic;
    doc[F("mqttTelemetryInterval")] = mqtt_info->mqttTelemetryInterval;

    if (serializeJson(doc, file) == 0)
    {
        // Serial.println(F("{\"error\": \"Failed to serialize file\"}"));
    }
    file.close();
    BWC_YIELD;
}

/**
 * response for /getmqtt/
 * web server prints a json document
 */
void handleGetMqtt()
{
    if (!checkHttpPost(server->method())) return;

    DynamicJsonDocument doc(1024);

    doc[F("enableMqtt")] = mqtt_info->useMqtt;
    doc[F("mqttHost")] = mqtt_info->mqttHost;
    doc[F("mqttPort")] = mqtt_info->mqttPort;
    doc[F("mqttUsername")] = mqtt_info->mqttUsername;
    doc[F("mqttPassword")] = "<enter password>";
    if (!hidePasswords)
    {
        doc[F("mqttPassword")] = mqtt_info->mqttPassword;
    }
    doc[F("mqttClientId")] = mqtt_info->mqttClientId;
    doc[F("mqttBaseTopic")] = mqtt_info->mqttBaseTopic;
    doc[F("mqttTelemetryInterval")] = mqtt_info->mqttTelemetryInterval;

    String json;
    if (serializeJson(doc, json) == 0)
    {
        json = F("{\"error\": \"Failed to serialize message\"}");
    }
    server->send(200, F("text/plain"), json);
    BWC_YIELD;
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

    mqtt_info->useMqtt = doc[F("enableMqtt")];
    enableMqtt = mqtt_info->useMqtt;
    mqtt_info->mqttHost = doc[F("mqttHost")].as<String>();
    mqtt_info->mqttPort = doc[F("mqttPort")];
    mqtt_info->mqttUsername = doc[F("mqttUsername")].as<String>();
    mqtt_info->mqttPassword = doc[F("mqttPassword")].as<String>();
    mqtt_info->mqttClientId = doc[F("mqttClientId")].as<String>();
    mqtt_info->mqttBaseTopic = doc[F("mqttBaseTopic")].as<String>();
    mqtt_info->mqttTelemetryInterval = doc[F("mqttTelemetryInterval")];

    server->send(200, F("text/plain"), "");

    saveMqtt();
    startMqtt();
    BWC_YIELD;
}

/**
 * response for /dir/
 * web server prints a list of files
 */
void handleDir()
{
    // HeapSelectIram ephemeral;
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
    /** a file variable to temporarily store the received file */
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

        BWC_LOG_P(PSTR("FS > upload filename: %s\n"),path.c_str());

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
            BWC_LOG_P(PSTR("FS > upload size: %d\n"),upload.totalSize);
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
            BWC_LOG_P(PSTR("FA > error: %d\n"), upload.status);
            server->send(500, F("text/plain"), F("500: couldn't create file"));
        }
    }
    else
    {
        BWC_LOG_P(PSTR("FA > upload aborted: %d\n"), upload.status);
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
    BWC_LOG_P(PSTR("ESP restart ...\n"),0);
    ESP.restart();
    delay(3000);
}

void updateStart(){
    BWC_LOG_P(PSTR("OTA > update start\n"),0);
}
void updateEnd(){
    BWC_LOG_P(PSTR("OTA > update finish\n"),0);
}
void udpateProgress(int cur, int total){
    BWC_LOG_P(PSTR("OTA: update process at %d of %d bytes...\n"), cur, total);
}
void updateError(int err){
    BWC_LOG_P(PSTR("update fatal error code %d\n"), err);
}

/**
 * MQTT setup and connect
 * @author 877dev
 */
void startMqtt()
{
    {
        HeapSelectIram ephemeral;
        BWC_LOG_P(PSTR("MQTT > start. Iram heap: %d\n"), ESP.getFreeHeap());
        if(!aWifiClient) aWifiClient = new WiFiClient;
        if(!mqttClient) mqttClient = new PubSubClient(*aWifiClient);
    

        // load mqtt credential file if it exists, and update default strings
        loadMqtt();

        // disconnect in case we are already connected
        mqttClient->disconnect();

        // setup MQTT broker information as defined earlier
        mqttClient->setServer(mqtt_info->mqttHost.c_str(), mqtt_info->mqttPort);
        // set buffer for larger messages, new to library 2.8.0
        // if (mqttClient->setBufferSize(1536))
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
    BWC_YIELD;
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
    if (String(topic).equals(String(mqtt_info->mqttBaseTopic) + F("/command")))
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
    if (String(topic).equals(String(mqtt_info->mqttBaseTopic) + F("/command_batch")))
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

    if (String(topic).equals(String(mqtt_info->mqttBaseTopic) + F("/set_config")))
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
    BWC_LOG_P(PSTR("MQTT > connecting\n"),0);

    // Serial.print(F("MQTT > Connecting ... "));
    // We'll connect with a Retained Last Will that updates the 'Status' topic with "Dead" when the device goes offline...
    if (mqttClient->connect(
        mqtt_info->mqttClientId.c_str(), // client_id : the client ID to use when connecting to the server->
        mqtt_info->mqttUsername.c_str(), // username : the username to use. If NULL, no username or password is used (const char[])
        mqtt_info->mqttPassword.c_str(), // password : the password to use. If NULL, no password is used (const char[])setupHA
        (String(mqtt_info->mqttBaseTopic) + F("/Status")).c_str(), // willTopic : the topic to be used by the will message (const char[])
        0, // willQoS : the quality of service to be used by the will message (int : 0,1 or 2)
        1, // willRetain : whether the will should be published with the retain flag (int : 0 or 1)
        "Dead")) // willMessage : the payload of the will message (const char[])
    {
        // Serial.println(F("success!"));
        mqtt_connect_count++;

        // update MQTT every X seconds. (will also be updated on state changes)
        updateMqttTimer->attach(mqtt_info->mqttTelemetryInterval, []{ sendMQTTFlag = true; });

        // These all have the Retained flag set to true, so that the value is stored on the server and can be retrieved at any point
        // Check the 'Status' topic to see that the device is still online before relying on the data from these retained topics
        mqttClient->publish((String(mqtt_info->mqttBaseTopic) + F("/Status")).c_str(), "Alive", true);
        mqttClient->publish((String(mqtt_info->mqttBaseTopic) + F("/MAC_Address")).c_str(), WiFi.macAddress().c_str(), true);                 // Device MAC Address
        mqttClient->publish((String(mqtt_info->mqttBaseTopic) + F("/MQTT_Connect_Count")).c_str(), String(mqtt_connect_count).c_str(), true); // MQTT Connect Count
        mqttClient->loop();

        // Watch the 'command' topic for incoming MQTT messages
        mqttClient->subscribe((String(mqtt_info->mqttBaseTopic) + F("/command")).c_str());
        mqttClient->subscribe((String(mqtt_info->mqttBaseTopic) + F("/command_batch")).c_str());
        mqttClient->subscribe((String(mqtt_info->mqttBaseTopic) + F("/set_config")).c_str());
        mqttClient->loop();

        #ifdef ESP8266
        // mqttClient->publish((String(mqttBaseTopic) + "/reboot_time").c_str(), DateTime.format(DateFormatter::SIMPLE).c_str(), true);
        mqttClient->publish((String(mqtt_info->mqttBaseTopic) + F("/reboot_time")).c_str(), (bwc->reboot_time_str+'Z').c_str(), true);
        mqttClient->publish((String(mqtt_info->mqttBaseTopic) + F("/reboot_reason")).c_str(), ESP.getResetReason().c_str(), true);
        String buttonname;
        buttonname.reserve(32);
        bwc->getButtonName(buttonname);
        mqttClient->publish((String(mqtt_info->mqttBaseTopic) + F("/button")).c_str(), buttonname.c_str(), true);
        mqttClient->loop();
        sendMQTT();
        BWC_LOG_P(PSTR("MQTT > Sending HA discovery"),0);
        mqttClient->setBufferSize(1536);
        setupHA();
        mqttClient->setBufferSize(512);
        mqttClient->loop();
        // Serial.println(F("MQTT Sending config"));
        // sendMQTTConfig();    // Stack smashing if doing this here :-(
        send_mqtt_cfg_needed = true;
        BWC_LOG_P(PSTR("MQTT > connect done\n"),0);
        #endif
    }
    else
    {
        // Serial.print(F("failed, Return Code = "));
        // Serial.println(mqttClient->state()); // states explained in webSocket->js
    }
    BWC_YIELD;
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
    BWC_YIELD;
}

#include "ha.hpp"
#include "prometheus.hpp"