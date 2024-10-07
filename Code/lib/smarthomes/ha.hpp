#include <Arduino.h>
#include "ArduinoJson.h"
#include "config.h"
#include "main.h"
/*
    Note, following does not work!
    doc[F("key")] = F("Value")
    String s; doc[s] = ...
    Throws all kind of errors. String throws stack smashing.
*/

void setupHA()
{
    //HeapSelectIram ephemeral;
    //HeapSelectDram ephemeral;


    
    /* Save RAM by storing strings in FLASH */

    PGM_P fs_dev = PSTR("dev");
    PGM_P fs_name = PSTR("name");
    PGM_P fs_uniq_id = PSTR("uniq_id");
    PGM_P fs_stat_t = PSTR("stat_t");
    PGM_P fs_avty_t = PSTR("avty_t");
    PGM_P fs_pl_avail = PSTR("pl_avail");
    PGM_P fs_pl_not_avail = PSTR("pl_not_avail");
    PGM_P fs_val_tpl = PSTR("val_tpl");
    PGM_P fs_cmd_t = PSTR("cmd_t");
    PGM_P fs_cmd_tpl = PSTR("cmd_tpl");
    PGM_P fs_conf_url = PSTR("configuration_url");
    PGM_P fs_connections = PSTR("connections");
    PGM_P fs_identifiers = PSTR("identifiers");
    PGM_P fs_manufacturer = PSTR("manufacturer");
    PGM_P fs_model = PSTR("model");
    PGM_P fs_sw_v = PSTR("sw_version");
    PGM_P fs_alive = PSTR("Alive");
    PGM_P fs_dead = PSTR("Dead");
    PGM_P fs_min = PSTR("min");
    PGM_P fs_max = PSTR("max");
    PGM_P fs_icon = PSTR("icon");
    

    PGM_P fs_max_temp = PSTR("max_temp");
    PGM_P fs_min_temp = PSTR("min_temp");
    PGM_P fs_precision = PSTR("precision");
    PGM_P fs_temp_unit = PSTR("temp_unit");
    PGM_P fs_modes = PSTR("modes");
    PGM_P fs_mode_cmd_t = PSTR("mode_cmd_t");
    PGM_P fs_mode_cmd_tpl = PSTR("mode_cmd_tpl");
    PGM_P fs_mode_stat_t = PSTR("mode_stat_t");
    PGM_P fs_mode_stat_tpl = PSTR("mode_stat_tpl");
    PGM_P fs_act_t = PSTR("act_t");
    PGM_P fs_act_tpl = PSTR("act_tpl");
    PGM_P fs_temp_stat_t = PSTR("temp_stat_t");
    PGM_P fs_temp_stat_tpl = PSTR("temp_stat_tpl");
    PGM_P fs_curr_temp_t = PSTR("curr_temp_t");
    PGM_P fs_curr_temp_tpl = PSTR("curr_temp_tpl");
    PGM_P fs_temp_cmd_t = PSTR("temp_cmd_t");
    PGM_P fs_temp_cmd_tpl = PSTR("temp_cmd_tpl");

    PGM_P fs_expire_after = PSTR("expire_after");
    PGM_P fs_unit_of_meas = PSTR("unit_of_meas");
    PGM_P fs_dev_cla = PSTR("dev_cla");


    /* ArduinoJson crashes when using flash strings on both sides
        so here we pull them back to a String in RAM temporarily. */

    char _dev[4];
    char _name[5];
    char _uniq_id[8];
    char _stat_t[7]; 
    char _avty_t[7];
    char _pl_avail[9]; 
    char _pl_not_avail[13];
    char _val_tpl[8]; 
    char _cmd_t[6];
    char _cmd_tpl[8]; 
    char _conf_url[18];
    char _connections[12];
    char _identifiers[12];
    char _manufacturer[13];
    char _model[6];
    char _sw_v[11];
    char _alive[6];
    char _dead[5]; 
    char _mymin[4]; 
    char _mymax[4]; 
    char _icon[5]; 

    char _max_temp[9];
    char _min_temp[9];
    char _precision[10];
    char _temp_unit[10];
    char _modes[6];
    char _mode_cmd_t[11];
    char _mode_cmd_tpl[13];
    char _mode_stat_t[12];
    char _mode_stat_tpl[14];
    char _act_t[6];
    char _act_tpl[8];
    char _temp_stat_t[12];
    char _temp_stat_tpl[14];
    char _curr_temp_t[12];
    char _curr_temp_tpl[14];
    char _temp_cmd_t[11];
    char _temp_cmd_tpl[13];

    char _expire_after[13];
    char _unit_of_meas[13];
    char _dev_cla[8];


    strcpy_P(_dev, fs_dev);
    strcpy_P(_name, fs_name);
    strcpy_P(_uniq_id, fs_uniq_id);
    strcpy_P(_stat_t, fs_stat_t);
    strcpy_P(_avty_t, fs_avty_t);
    strcpy_P(_pl_avail, fs_pl_avail);
    strcpy_P(_pl_not_avail, fs_pl_not_avail);
    strcpy_P(_val_tpl, fs_val_tpl);
    strcpy_P(_cmd_t, fs_cmd_t);
    strcpy_P(_cmd_tpl, fs_cmd_tpl);
    strcpy_P(_conf_url, fs_conf_url);
    strcpy_P(_connections, fs_connections);
    strcpy_P(_identifiers, fs_identifiers);
    strcpy_P(_manufacturer, fs_manufacturer);
    strcpy_P(_model, fs_model);
    strcpy_P(_sw_v, fs_sw_v);
    strcpy_P(_alive, fs_alive);
    strcpy_P(_dead, fs_dead);
    strcpy_P(_mymin, fs_min);
    strcpy_P(_mymax, fs_max);
    strcpy_P(_icon, fs_icon);

    strcpy_P(_max_temp, fs_max_temp);
    strcpy_P(_min_temp, fs_min_temp);
    strcpy_P(_precision, fs_precision);
    strcpy_P(_temp_unit, fs_temp_unit);
    strcpy_P(_modes, fs_modes);
    strcpy_P(_mode_cmd_t, fs_mode_cmd_t);
    strcpy_P(_mode_cmd_tpl, fs_mode_cmd_tpl);
    strcpy_P(_mode_stat_t, fs_mode_stat_t);
    strcpy_P(_mode_stat_tpl, fs_mode_stat_tpl);
    strcpy_P(_act_t, fs_act_t);
    strcpy_P(_act_tpl, fs_act_tpl);
    strcpy_P(_temp_stat_t, fs_temp_stat_t);
    strcpy_P(_temp_stat_tpl, fs_temp_stat_tpl);
    strcpy_P(_curr_temp_t, fs_curr_temp_t);
    strcpy_P(_curr_temp_tpl, fs_curr_temp_tpl);
    strcpy_P(_temp_cmd_t, fs_temp_cmd_t);
    strcpy_P(_temp_cmd_tpl, fs_temp_cmd_tpl);

    strcpy_P(_expire_after, fs_expire_after);
    strcpy_P(_unit_of_meas, fs_unit_of_meas);
    strcpy_P(_dev_cla, fs_dev_cla);

    /* DEVICE */
    /*
    if(!mqttClient->connected()){
        String s = F("HA setup failed. MQTT not connected");
        bwc->saveDebugInfo(s);
        return;
    }
    */
    String topic;
    String payload;
    payload.reserve(1300);
    #if defined(ESP8266)
    String mychipid = String((unsigned int)ESP.getChipId());
    #elif defined(ESP32)
    String mychipid = String((unsigned int)ESP.getChipModel());
    #endif
    int maxtemp, mintemp;
    maxtemp = 104;
    mintemp = 68;
    DynamicJsonDocument devicedoc(512);
    devicedoc[_dev][_conf_url] = F("http://") + WiFi.localIP().toString();
    devicedoc[(_dev)][(_connections)].add(serialized("[\"mac\",\"" + WiFi.macAddress()+"\"]" ));
    devicedoc[(_dev)][(_identifiers)] = mychipid;
    devicedoc[(_dev)][(_manufacturer)] = F("Visualapproach");
    devicedoc[(_dev)][(_model)] = bwc->getModel();
    devicedoc[(_dev)][(_name)] = mqtt_info->mqttBaseTopic;
    devicedoc[(_dev)][(_sw_v)] = FW_VERSION;


    DynamicJsonDocument doc(1536);
    /************/
    /* NUMBER   */
    /************/

                                    /* brightness slider */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    String unique_id = mqtt_info->mqttBaseTopic + F("_brightness")+mychipid;
    topic = HA_PREFIX_F;               //HA discovery prefix
    topic += F("/number/");             //component
    topic += unique_id;                 //object id = unique id as recommended in HA docs
    topic += F("/config");
    doc[(_uniq_id)] = unique_id;
    doc[(_name)] = F("Brightness");     //Friendly name will be "devicename Brightness" ie "layzspa Brightness"
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/message");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[(_val_tpl)] = F("{{ value_json.BRT }}");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/command");
    doc[(_cmd_tpl)] = F("{CMD:12,VALUE:{{ value | int }},XTIME:0,INTERVAL:0}");
    doc[_mymin] = 0;
    doc[_mymax] = 8;
    doc["mode"] = F("slider");
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();


                                        /* spa ambient temperature number °C */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_amb_temp_c")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/number/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Amb temp C");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/message");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[(_val_tpl)] = F("{{ value_json.AMBC }}");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/command");
    doc[(_cmd_tpl)] = F("{CMD:15,VALUE:{{ value | int }},XTIME:0,INTERVAL:0}");
    doc[_mymin] = -50;
    doc[_mymax] = 60;
    doc["mode"] = F("slider");
    doc[_unit_of_meas] = "°C";
    doc[_expire_after] = 700;
    doc[_dev_cla] = F("temperature");
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize amb!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();

                                        /* spa ambient temperature number °F */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_amb_temp_f")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/number/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Amb temp F");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/message");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[(_val_tpl)] = F("{{ value_json.AMBF }}");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/command");
    doc[(_cmd_tpl)] = F("{CMD:14,VALUE:{{ value | int }},XTIME:0,INTERVAL:0}");
    doc[_mymin] = -58;
    doc[_mymax] = 140;
    doc["mode"] = F("slider");
    doc[_unit_of_meas] = "°C";
    doc[_expire_after] = 700;
    doc[_dev_cla] = F("temperature");
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize amb!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();




        /************/
        /* SENSORS  */
        /************/

    Serial.println(F("sensors"));
                                        /* pressed button sensor */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_pressed_button")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Pressed button");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/button");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;

    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }

    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();




                                        /* reboot time sensor */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_reboot_time")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Reboot time");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/reboot_time");
    doc[(_val_tpl)] = F("{{as_timestamp(as_datetime(value).isoformat()) | timestamp_custom('%F %T')}}");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();



                                        /* reboot reason sensor */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_reboot_reason")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Reboot reason");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/reboot_reason");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();



                                        /* WiFi SSID sensor */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_ssid")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("SSID");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/other");
    doc[(_val_tpl)] = F("{{ value_json.SSID }}");
    doc[_expire_after] = 700;
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();


                                        /* WiFi RSSI sensor */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_rssi")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("RSSI");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/other");
    doc[_unit_of_meas] = F("dBm");
    doc[(_val_tpl)] = F("{{ value_json.RSSI }}");
    doc[_expire_after] = 700;
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();



                                        /* WiFi local ip sensor */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_ip")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("IP");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/other");
    doc[(_val_tpl)] = F("{{ value_json.IP }}");
    doc[_expire_after] = 700;
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();


                                        /* connect count sensor */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_connect_count")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Connect count");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/MQTT_Connect_Count");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();

                                        /* error sensor */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_error")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Error");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/message");
    doc[(_val_tpl)] = F("{{ value_json.ERR }}");
    doc[_expire_after] = 700;
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();




                                        /* spa time to ready sensor */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_time_to_ready")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Time to ready");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/times");
    doc[_unit_of_meas] = F("hours");
    doc[(_val_tpl)] = F("{{ value_json.T2R }}");
    doc[_expire_after] = 700;
    doc[_icon] = F("mdi:clock");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();



                                        /* Ready State sensor (Never, Ready, Not Ready) */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_time_to_ready")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Ready state");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/times");
    doc[(_val_tpl)] = F("{{ value_json.RS }}");
    doc[_expire_after] = 700;
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();




                                        /* spa energy sensor */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_energy")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Energy");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/times");
    doc[_unit_of_meas] = F("kWh");
    doc[(_val_tpl)] = F("{{ value_json.KWH | round(3) }}");
    doc[_dev_cla] = F("energy");
    doc["state_class"] = F("total_increasing");
    doc[_expire_after] = 700;
    doc[_icon] = F("mdi:flash");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();

                                        /* spa energy cost sensor */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_cost")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Energy cost");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/times");
    doc[(_val_tpl)] = F("{{ value_json.COST | round(3) }}");
    doc[_dev_cla] = F("monetary");
    doc[_expire_after] = 700;
    doc[_icon] = F("mdi:currency-usd");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();


                                        /* spa daily energy sensor */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_today")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Today");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/times");
    doc[_unit_of_meas] = F("kWh");
    doc[(_val_tpl)] = F("{{ value_json.KWHD | round(3) }}");
    doc[_dev_cla] = F("energy");
    doc["state_class"] = F("total_increasing");
    doc[_expire_after] = 700;
    doc[_icon] = F("mdi:flash");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();



                                        /* spa power sensor */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_power")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Power");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/times");
    doc[_unit_of_meas] = F("W");
    doc[(_val_tpl)] = F("{{ value_json.WATT | int }}");
    doc[_dev_cla] = F("power");
    doc["state_class"] = F("measurement");
    doc[_expire_after] = 700;
    doc[_icon] = F("mdi:flash");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();



                                        /* spa chlorine age sensor */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_chlorine_age")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Chlorine age");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/times");
    doc[_unit_of_meas] = F("days");
    doc[(_val_tpl)] = F("{{ ( ( (now().timestamp()|int) - value_json.CLTIME|int)/3600/24) | round(2) }}");
    doc[_expire_after] = 700;
    doc[_icon] = F("hass:hand-coin-outline");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();



                                        /* spa filter age sensor */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_filter_age")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Filter age");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/times");
    doc[_unit_of_meas] = F("days");
    doc[(_val_tpl)] = F("{{ ( ( (now().timestamp()|int) - value_json.FREP|int)/3600/24) | round(2) }}");
    doc[_expire_after] = 700;
    doc[_icon] = F("hass:air-filter");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();

                                        /* spa filter clean sensor */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_filter_clean")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Filter clean");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/times");
    doc[_unit_of_meas] = F("days");
    doc[(_val_tpl)] = F("{{ ( ( (now().timestamp()|int) - value_json.FCLE|int)/3600/24) | round(2) }}");
    doc[_expire_after] = 700;
    doc[_icon] = F("hass:spray-bottle");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();

                                        /* spa filter rinse sensor */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_filter_rinse")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Filter rinse");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/times");
    doc[_unit_of_meas] = F("days");
    doc[(_val_tpl)] = F("{{ ( ( (now().timestamp()|int) - value_json.FRIN|int)/3600/24) | round(2) }}");
    doc[_expire_after] = 700;
    doc[_icon] = F("hass:water-pump");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();


                                        /* spa uptime sensor */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_uptime")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Uptime");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/times");
    doc[_unit_of_meas] = F("days");
    doc[(_val_tpl)] = F("{{ ( (value_json.UPTIME|int)/3600/24) | round(2) }}");
    doc[_expire_after] = 700;
    doc[_icon] = F("mdi:clock-outline");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();



                                        /* spa pump time sensor */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_pumptime")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Pump time");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/times");
    doc[_unit_of_meas] = F("hours");
    doc[(_val_tpl)] = F("{{ ( (value_json.PUMPTIME|int)/3600) | round(2) }}");
    doc[_expire_after] = 700;
    doc[_icon] = F("mdi:clock-outline");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();



                                        /* spa heater time sensor */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_heatertime")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Heater time");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/times");
    doc[_unit_of_meas] = F("hours");
    doc[(_val_tpl)] = F("{{ ( (value_json.HEATINGTIME|int)/3600) | round(2) }}");
    doc[_expire_after] = 700;
    doc[_icon] = F("mdi:clock-outline");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();



                                        /* spa air time sensor */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_airtime")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Air time");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/times");
    doc[_unit_of_meas] = F("hours");
    doc[(_val_tpl)] = F("{{ ( (value_json.AIRTIME|int)/3600) | round(2) }}");
    doc[_expire_after] = 700;
    doc[_icon] = F("mdi:clock-outline");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();



                                        /* spa temperature sensor f */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_temp_f")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Temp (F)");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/message");
    doc[_unit_of_meas] = "°F";
    doc[(_val_tpl)] = F("{{ value_json.TMPF }}");
    doc[_expire_after] = 700;
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[_dev_cla] = F("temperature");
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();



                                        /* spa temperature sensor c */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_temp_c")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Temp (C)");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/message");
    doc[_unit_of_meas] = "°C";
    doc[(_val_tpl)] = F("{{ value_json.TMPC }}");
    doc[_expire_after] = 700;
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[_dev_cla] = F("temperature");
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();



                                        /* spa virtual temperature sensor f */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_virtual_temp_f")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Virtual temp (F)");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/message");
    doc[_unit_of_meas] = "°F";
    doc[(_val_tpl)] = F("{{ value_json.VTMF | round(2) }}");
    doc[_expire_after] = 700;
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[_dev_cla] = F("temperature");
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize vtf"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();



                                        /* spa virtual temperature sensor c */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_virtual_temp_c")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Virtual temp (C)");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/message");
    doc[_unit_of_meas] = "°C";
    doc[(_val_tpl)] = F("{{ value_json.VTMC | round(2) }}");
    doc[_expire_after] = 700;
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[_dev_cla] = F("temperature");
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize vtc"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();



                                        /* spa target temperature sensor f */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_target_temp_f")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Target temp (F)");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/message");
    doc[_unit_of_meas] = "°F";
    doc[(_val_tpl)] = F("{{ value_json.TGTF }}");
    doc[_expire_after] = 700;
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[_dev_cla] = F("temperature");
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize tgtf!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();

    


                                        /* spa target temperature sensor c */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_target_temp_c")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Target temp (C)");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/message");
    doc[_unit_of_meas] = "°C";
    doc[(_val_tpl)] = F("{{ value_json.TGTC }}");
    doc[_expire_after] = 700;
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[_dev_cla] = F("temperature");
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize tgt!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();



        /******************/
        /* BINARY SENSORS */
        /******************/
    Serial.println(F("binary_sensors"));

                                        /* spa lock binary_sensor */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_lock")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/binary_sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[_name] = F("Lock");
    doc[_uniq_id] = unique_id;
    doc[_stat_t] = mqtt_info->mqttBaseTopic+F("/message");
    doc[_val_tpl] = F("{% if value_json.LCK == 1 %}OFF{% else %}ON{% endif %}");
    // doc[_dev_cla] = F("lock");
    doc[_expire_after] = 700;
    doc[_avty_t] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[_pl_avail] = _alive;
    doc[_pl_not_avail] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();



                                        /* spa heater binary_sensor */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_heater")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/binary_sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Heater");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/message");
    doc[(_val_tpl)] = F("{% if value_json.RED == 1 %}ON{% else %}OFF{% endif %}");
    doc[_dev_cla] = F("heat");
    doc[_expire_after] = 700;
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();



                                        /* spa ready binary_sensor */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_ready")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/binary_sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Ready");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/message");
    doc[(_val_tpl)] = F("{% if value_json.TMP > 30 %}{% if value_json.TMP >= value_json.TGT-1 %}ON{% else %}OFF{% endif %}{% else %}OFF{% endif %}");
    doc[_expire_after] = 700;
    doc[_icon] = F("mdi:hot-tub");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();



                                        /* spa connection status binary_sensor */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_connection")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/binary_sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Connection");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[_dev_cla] = F("connectivity");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc["pl_on"] = _alive;
    doc["pl_off"] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();


        /******************/
        /* SWITCHES       */
        /******************/

    Serial.println(F("switch"));


                                        /* spa power switch */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_powerswitch")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/switch/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Power switch");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/message");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/command");
    doc[(_val_tpl)] = F("{{ value_json.PWR }}");
    doc[_expire_after] = 700;
    doc[_icon] = F("mdi:power");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc["pl_on"] = F("{CMD:24,VALUE:true,XTIME:0,INTERVAL:0}");
    doc["pl_off"] = F("{CMD:24,VALUE:false,XTIME:0,INTERVAL:0}");
    doc["state_on"] = 1;
    doc["state_off"] = 0;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();



                                        /* spa heat regulation switch */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_heat_regulation")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/switch/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Heat regulation");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/message");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/command");
    doc[(_val_tpl)] = F("{% if value_json.RED == 1 %}1{% elif value_json.GRN == 1 %}1{% else %}0{% endif %}");
    doc[_expire_after] = 700;
    doc[_icon] = F("mdi:radiator");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc["pl_on"] = F("{CMD:3,VALUE:true,XTIME:0,INTERVAL:0}");
    doc["pl_off"] = F("{CMD:3,VALUE:false,XTIME:0,INTERVAL:0}");
    doc["state_on"] = 1;
    doc["state_off"] = 0;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();



                                    /* spa waterjets switch */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_jets")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/switch/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Jets");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/message");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/command");
    doc[(_val_tpl)] = F("{{ value_json.HJT }}");
    doc[_expire_after] = 700;
    doc[_icon] = F("mdi:hydro-power");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc["pl_on"] = F("{CMD:11,VALUE:true,XTIME:0,INTERVAL:0}");
    doc["pl_off"] = F("{CMD:11,VALUE:false,XTIME:0,INTERVAL:0}");
    doc["state_on"] = 1;
    doc["state_off"] = 0;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();



                                        /* spa airbubbles switch */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_airbubbles")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/switch/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Airbubbles");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/message");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/command");
    doc[(_val_tpl)] = F("{{ value_json.AIR }}");
    doc[_expire_after] = 700;
    doc[_icon] = F("mdi:chart-bubble");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc["pl_on"] = F("{CMD:2,VALUE:true,XTIME:0,INTERVAL:0}");
    doc["pl_off"] = F("{CMD:2,VALUE:false,XTIME:0,INTERVAL:0}");
    doc["state_on"] = 1;
    doc["state_off"] = 0;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();



                                        /* spa pump switch */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_pump")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/switch/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Pump");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/message");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/command");
    doc[(_val_tpl)] = F("{{ value_json.FLT }}");
    doc[_expire_after] = 700;
    doc[_icon] = F("mdi:pump");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc["pl_on"] = F("{CMD:4,VALUE:true,XTIME:0,INTERVAL:0}");
    doc["pl_off"] = F("{CMD:4,VALUE:false,XTIME:0,INTERVAL:0}");
    doc["state_on"] = 1;
    doc["state_off"] = 0;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();



                                        /* spa temperature unit switch */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_unit")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/switch/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Temperature unit F-C");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/message");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/command");
    doc[(_val_tpl)] = F("{{ value_json.UNT }}");
    doc[_expire_after] = 700;
    doc[_icon] = F("mdi:circle-outline");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc["pl_on"] = F("{CMD:1,VALUE:true,XTIME:0,INTERVAL:0}");
    doc["pl_off"] = F("{CMD:1,VALUE:false,XTIME:0,INTERVAL:0}");
    doc["state_on"] = 1;
    doc["state_off"] = 0;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();



                                        /* spa take control switch */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_ctrl")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/switch/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Take control");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/message");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/command");
    doc[(_val_tpl)] = F("{{ value_json.GOD }}");
    doc[_expire_after] = 700;
    doc[_icon] = F("mdi:steering");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc["pl_on"] = F("{CMD:17,VALUE:true,XTIME:0,INTERVAL:0}");
    doc["pl_off"] = F("{CMD:17,VALUE:false,XTIME:0,INTERVAL:0}");
    doc["state_on"] = 1;
    doc["state_off"] = 0;
    doc["assumed_state"] = 0; //get rid of lightning buttons
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();


        /******************/
        /* BUTTONS        */
        /******************/

    Serial.println(F("buttons"));


                                        /* spa reset chlorine timer button */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_reset_chlorine")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/button/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Reset chlorine timer");
    doc[(_uniq_id)] = unique_id;
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/command");
    doc["payload_press"] = F("{CMD:9,VALUE:true,XTIME:0,INTERVAL:0}");
    doc[_icon] = F("mdi:restart");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();



                                        /* spa reset filter change timer button */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_reset_filterchange")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/button/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Reset filter change timer");
    doc[(_uniq_id)] = unique_id;
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/command");
    doc["payload_press"] = F("{CMD:10,VALUE:true,XTIME:0,INTERVAL:0}");
    doc[_icon] = F("mdi:restart");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();

                                        /* spa reset filter clean timer button */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_reset_filterclean")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/button/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Reset filter clean timer");
    doc[(_uniq_id)] = unique_id;
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/command");
    doc["payload_press"] = F("{CMD:23,VALUE:true,XTIME:0,INTERVAL:0}");
    doc[_icon] = F("mdi:restart");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();

                                        /* spa reset filter rinse timer button */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_reset_filterrinse")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/button/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Reset filter rinse timer");
    doc[(_uniq_id)] = unique_id;
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/command");
    doc["payload_press"] = F("{CMD:22,VALUE:true,XTIME:0,INTERVAL:0}");
    doc[_icon] = F("mdi:restart");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();



                                        /* spa restart esp button */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_restart_esp")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/button/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Restart esp");
    doc[(_uniq_id)] = unique_id;
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/command");
    doc["payload_press"] = F("{CMD:6,VALUE:true,XTIME:0,INTERVAL:0}");
    doc[_icon] = F("mdi:restart");
    doc[_dev_cla] = F("restart");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize HA message!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();


        /****************/
        /* CLIMATE      */
        /****************/

    Serial.println(F("climate"));

                                        /* spa climate control */

    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_climate")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/climate/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Temperature control");
    doc[(_uniq_id)] = unique_id;
    doc[_max_temp] = maxtemp;
    doc[_min_temp] = mintemp;
    doc[_precision] = 1.0;
    doc[_temp_unit] = "F";
    doc[_modes].add(serialized("\"fan_only\", \"off\", \"heat\""));
    doc[_mode_cmd_t] = mqtt_info->mqttBaseTopic+F("/command_batch");
    doc[_mode_cmd_tpl] = F("[{CMD:3,VALUE:{%if value == \"heat\" %}1{% else %}0{% endif %},XTIME:0,INTERVAL:0},{CMD:4,VALUE:{%if value == \"fan_only\" %}1{% elif value == \"heat\" %}1{% else %}0{% endif %},XTIME:0,INTERVAL:0}]");
    doc[_mode_stat_t] = mqtt_info->mqttBaseTopic+F("/message");
    doc[_mode_stat_tpl] = F("{% if value_json.RED == 1 %}heat{% elif value_json.GRN == 1 %}heat{% else %}off{% endif %}");
    doc[_act_t] = mqtt_info->mqttBaseTopic+F("/message");
    doc[_act_tpl] = F("{% if value_json.RED == 1 %}heating{% elif value_json.GRN == 1 %}idle{% elif value_json.FLT == 1 %}fan{% else %}off{% endif %}");
    doc[_temp_stat_t] = mqtt_info->mqttBaseTopic+F("/message");
    doc[_temp_stat_tpl] = F("{{ value_json.TGTF }}");
    doc[_curr_temp_t] = mqtt_info->mqttBaseTopic+F("/message");
    doc[_curr_temp_tpl] = F("{{ value_json.TMPF }}");
    doc[_temp_cmd_t] = mqtt_info->mqttBaseTopic+F("/command");
    doc[_temp_cmd_tpl] = F("{CMD:0,VALUE:{{ value|int }},XTIME:0,INTERVAL:0}");
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize climate"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    doc.clear();
    doc.garbageCollect();
    //   Serial.println(payload);


        /****************/
        /* CONFIG       */
        /****************/

                                        /* Electricity price */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_price")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/number/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Electricity price");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/get_config");    // CONFIG TOPIC
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[(_val_tpl)] = F("{{ value_json.PRICE }}");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/set_config");     // CONFIG TOPIC
    doc[(_cmd_tpl)] = F("{PRICE:{{ value | float }}}");
    doc[_mymin] = -1000.0f;
    doc[_mymax] = 65535.0f;
    doc["step"] = 0.01f;
    doc["mode"] = F("box");
    // doc[_unit_of_meas] = "";
    // doc[_expire_after] = 700;
    doc[_dev_cla] = F("monetary");
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize amb!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();

                                        /* Filter renewal reminder interval */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_filter_replace_interval")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/number/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Filter change interval");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/get_config");    // CONFIG TOPIC
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[(_val_tpl)] = F("{{ value_json.FREPI }}");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/set_config");     // CONFIG TOPIC
    doc[(_cmd_tpl)] = F("{FREPI:{{ value | int }}}");
    doc[_mymin] = 1;
    doc[_mymax] = 1000;
    doc["step"] = 1;
    doc["mode"] = F("box");
    doc[_unit_of_meas] = "d";
    // doc[_expire_after] = 700;
    doc[_dev_cla] = F("duration");
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize amb!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();

                                        /* Filter clean reminder interval */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_filter_clean_interval")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/number/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Filter clean interval");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/get_config");    // CONFIG TOPIC
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[(_val_tpl)] = F("{{ value_json.FCLEI }}");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/set_config");     // CONFIG TOPIC
    doc[(_cmd_tpl)] = F("{FCLEI:{{ value | int }}}");
    doc[_mymin] = 1;
    doc[_mymax] = 1000;
    doc["step"] = 1;
    doc["mode"] = F("box");
    doc[_unit_of_meas] = "d";
    // doc[_expire_after] = 700;
    doc[_dev_cla] = F("duration");
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize amb!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();

                                        /* Filter rinse reminder interval */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_filter_rinse_interval")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/number/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Filter rinse interval");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/get_config");    // CONFIG TOPIC
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[(_val_tpl)] = F("{{ value_json.FRINI }}");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/set_config");     // CONFIG TOPIC
    doc[(_cmd_tpl)] = F("{FRINI:{{ value | int }}}");
    doc[_mymin] = 1;
    doc[_mymax] = 1000;
    doc["step"] = 1;
    doc["mode"] = F("box");
    doc[_unit_of_meas] = "d";
    // doc[_expire_after] = 700;
    doc[_dev_cla] = F("duration");
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize amb!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();

                                        /* Chlorine renewal reminder interval */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_cl_interval")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/number/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Chlorine interval");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/get_config");    // CONFIG TOPIC
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[(_val_tpl)] = F("{{ value_json.CLINT }}");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/set_config");     // CONFIG TOPIC
    doc[(_cmd_tpl)] = F("{CLINT:{{ value | int }}}");
    doc[_mymin] = 1;
    doc[_mymax] = 1000;
    doc["step"] = 1;
    doc["mode"] = F("box");
    doc[_unit_of_meas] = "d";
    // doc[_expire_after] = 700;
    doc[_dev_cla] = F("duration");
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize amb!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();


                                        /* Audio enabled */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_audio")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/switch/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Audio");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/get_config");    // CONFIG TOPIC
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[(_val_tpl)] = F("{{ value_json.AUDIO }}");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/set_config");     // CONFIG TOPIC
    doc["pl_on"] = F("{AUDIO:true}");
    doc["pl_off"] = F("{AUDIO:false}");
    doc["state_on"] = true;
    doc["state_off"] = false;
    // doc[_expire_after] = 700;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize amb!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();

                                        /* Restore states on start */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_restore")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/switch/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Restore on start");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/get_config");    // CONFIG TOPIC
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[(_val_tpl)] = F("{{ value_json.RESTORE }}");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/set_config");     // CONFIG TOPIC
    doc["pl_on"] = F("{RESTORE:true}");
    doc["pl_off"] = F("{RESTORE:false}");
    doc["state_on"] = true;
    doc["state_off"] = false;
    // doc[_expire_after] = 700;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize amb!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();

                                        /* Notify */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_notify")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/switch/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Notify");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/get_config");    // CONFIG TOPIC
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[(_val_tpl)] = F("{{ value_json.NOTIFY }}");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/set_config");     // CONFIG TOPIC
    doc["pl_on"] = F("{NOTIFY:true}");
    doc["pl_off"] = F("{NOTIFY:false}");
    doc["state_on"] = true;
    doc["state_off"] = false;
    // doc[_expire_after] = 700;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize amb!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();

                                        /* Notification time */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_notiftime")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/number/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Notification time");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/get_config");    // CONFIG TOPIC
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[(_val_tpl)] = F("{{ value_json.NOTIFTIME }}");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/set_config");     // CONFIG TOPIC
    doc[(_cmd_tpl)] = F("{NOTIFTIME:{{ value | int }}}");
    doc[_mymin] = 1;
    doc[_mymax] = 1000;
    doc["step"] = 1;
    doc["mode"] = F("box");
    doc[_unit_of_meas] = "s";
    // doc[_expire_after] = 700;
    doc[_dev_cla] = F("duration");
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize amb!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();

                                        /* Virtual temperature calibrated */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_vtcal")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/binary_sensor/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("VTCAL");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/get_config");    // CONFIG TOPIC
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[(_val_tpl)] = F("{% if value_json.VTCAL == 1 %}OFF{% else %}ON{% endif %}");
    // doc[_expire_after] = 700;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize amb!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();

                                        /* Enable LOCK button */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_lck")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/switch/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Lock btn enabled");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/get_config");    // CONFIG TOPIC
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[(_val_tpl)] = F("{{ value_json.LCK }}");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/set_config");     // CONFIG TOPIC
    doc["pl_on"] = F("{LCK:true}");
    doc["pl_off"] = F("{LCK:false}");
    doc["state_on"] = true;
    doc["state_off"] = false;
    // doc[_expire_after] = 700;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize amb!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();

                                        /* Enable TIMER button */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_tmr")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/switch/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Timer btn enabled");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/get_config");    // CONFIG TOPIC
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[(_val_tpl)] = F("{{ value_json.TMR }}");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/set_config");     // CONFIG TOPIC
    doc["pl_on"] = F("{TMR:true}");
    doc["pl_off"] = F("{TMR:false}");
    doc["state_on"] = true;
    doc["state_off"] = false;
    // doc[_expire_after] = 700;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize amb!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();

                                        /* Enable AIR button */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_air")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/switch/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Airbubbles btn enabled");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/get_config");    // CONFIG TOPIC
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[(_val_tpl)] = F("{{ value_json.AIR }}");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/set_config");     // CONFIG TOPIC
    doc["pl_on"] = F("{AIR:true}");
    doc["pl_off"] = F("{AIR:false}");
    doc["state_on"] = true;
    doc["state_off"] = false;
    // doc[_expire_after] = 700;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize amb!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();

                                        /* Enable UNIT button */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_unt")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/switch/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Unit btn enabled");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/get_config");    // CONFIG TOPIC
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[(_val_tpl)] = F("{{ value_json.UNT }}");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/set_config");     // CONFIG TOPIC
    doc["pl_on"] = F("{UNT:true}");
    doc["pl_off"] = F("{UNT:false}");
    doc["state_on"] = true;
    doc["state_off"] = false;
    // doc[_expire_after] = 700;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize amb!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();

                                        /* Enable HEAT button */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_htr")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/switch/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Heater btn enabled");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/get_config");    // CONFIG TOPIC
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[(_val_tpl)] = F("{{ value_json.HTR }}");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/set_config");     // CONFIG TOPIC
    doc["pl_on"] = F("{HTR:true}");
    doc["pl_off"] = F("{HTR:false}");
    doc["state_on"] = true;
    doc["state_off"] = false;
    // doc[_expire_after] = 700;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize amb!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();

                                        /* Enable FILTER button */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_flt")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/switch/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Filter btn enabled");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/get_config");    // CONFIG TOPIC
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[(_val_tpl)] = F("{{ value_json.FLT }}");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/set_config");     // CONFIG TOPIC
    doc["pl_on"] = F("{FLT:true}");
    doc["pl_off"] = F("{FLT:false}");
    doc["state_on"] = true;
    doc["state_off"] = false;
    // doc[_expire_after] = 700;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize amb!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();

                                        /* Enable DOWN button */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_dn")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/switch/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Down btn enabled");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/get_config");    // CONFIG TOPIC
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[(_val_tpl)] = F("{{ value_json.DN }}");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/set_config");     // CONFIG TOPIC
    doc["pl_on"] = F("{DN:true}");
    doc["pl_off"] = F("{DN:false}");
    doc["state_on"] = true;
    doc["state_off"] = false;
    // doc[_expire_after] = 700;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize amb!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();

                                        /* Enable UP button */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_up")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/switch/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Up btn enabled");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/get_config");    // CONFIG TOPIC
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[(_val_tpl)] = F("{{ value_json.UP }}");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/set_config");     // CONFIG TOPIC
    doc["pl_on"] = F("{UP:true}");
    doc["pl_off"] = F("{UP:false}");
    doc["state_on"] = true;
    doc["state_off"] = false;
    // doc[_expire_after] = 700;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize amb!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();

                                        /* Enable POWER button */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_pwr")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/switch/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Power btn enabled");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/get_config");    // CONFIG TOPIC
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[(_val_tpl)] = F("{{ value_json.PWR }}");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/set_config");     // CONFIG TOPIC
    doc["pl_on"] = F("{PWR:true}");
    doc["pl_off"] = F("{PWR:false}");
    doc["state_on"] = true;
    doc["state_off"] = false;
    // doc[_expire_after] = 700;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize amb!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();

                                        /* Enable HYDROJETS button */
    doc[(_dev)] = devicedoc[(_dev)];
    payload.clear();
    unique_id = mqtt_info->mqttBaseTopic + F("_hjt")+mychipid;
    topic = HA_PREFIX_F;
    topic += F("/switch/");
    topic += unique_id;
    topic += F("/config");
    doc[(_name)] = F("Hydrojets btn enabled");
    doc[(_uniq_id)] = unique_id;
    doc[(_stat_t)] = mqtt_info->mqttBaseTopic+F("/get_config");    // CONFIG TOPIC
    doc[(_avty_t)] = mqtt_info->mqttBaseTopic+F("/Status");
    doc[(_pl_avail)] = _alive;
    doc[(_pl_not_avail)] = _dead;
    doc[(_val_tpl)] = F("{{ value_json.HJT }}");
    doc[(_cmd_t)] = mqtt_info->mqttBaseTopic+F("/set_config");     // CONFIG TOPIC
    doc["pl_on"] = F("{HJT:true}");
    doc["pl_off"] = F("{HJT:false}");
    doc["state_on"] = true;
    doc["state_off"] = false;
    // doc[_expire_after] = 700;
    if (serializeJson(doc, payload) == 0)
    {
        // Serial.println(F("Failed to serialize amb!"));
        return;
    }
    mqttClient->publish(topic.c_str(), payload.c_str(), true);
    mqttClient->loop();
    // Serial.println(payload);
    doc.clear();
    doc.garbageCollect();
}

