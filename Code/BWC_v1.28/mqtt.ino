#ifdef USE_MQTT

void MQTTcallback(char* topic, byte* payload, unsigned int length) {  //877dev

  Serial.print(F("Message arrived ["));
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  if (String(topic).equals(String(base_mqtt_topic) + "/command")) {
    uint32_t cmd = strtol((const char *) &payload[0], NULL, 16);
    commandAction(cmd);
  }
} // End of void MQTTcallback

void MQTT_Connect()
{
  Serial.print(F("Connecting to MQTT...  "));
  // We'll connect with a Retained Last Will that updates the '.../Status' topic with "Dead" when the device goes offline...
  // Attempt to connect...
  /*
    MQTT Connection syntax:
    boolean connect (client_id, username, password, willTopic, willQoS, willRetain, willMessage)
    Connects the client with a Will message, username and password specified.
    Parameters
    client_id : the client ID to use when connecting to the server.
    username : the username to use. If NULL, no username or password is used (const char[])
    password : the password to use. If NULL, no password is used (const char[])
    willTopic : the topic to be used by the will message (const char[])
    willQoS : the quality of service to be used by the will message (int : 0,1 or 2)
    willRetain : whether the will should be published with the retain flag (int : 0 or 1)
    willMessage : the payload of the will message (const char[])
    Returns
    false - connection failed.
    true - connection succeeded
  */
  if (MQTTclient.connect(mqtt_client_id, mqtt_username, mqtt_password, (String(base_mqtt_topic) + "/Status").c_str(), 0, 1, "Dead"))
  {
    // We get here if the connection was successful...
    mqtt_connect_count++;
    Serial.println(F("CONNECTED!"));
    // Once connected, publish some announcements...
    // These all have the Retained flag set to true, so that the value is stored on the server and can be retrieved at any point
    // Check the .../Status topic to see that the device is still online before relying on the data from these retained topics
    MQTTclient.publish((String(base_mqtt_topic) + "/Status").c_str(), "Alive", true);
    MQTTclient.publish((String(base_mqtt_topic) + "/MAC_Address").c_str(), WiFi.macAddress().c_str(), true);                 // Device MAC Address
    MQTTclient.publish((String(base_mqtt_topic) + "/MQTT_Connect_Count").c_str(), String(mqtt_connect_count).c_str(), true); // MQTT Connect Count
    MQTTclient.loop();


    // ... and then re/subscribe to the watched topics
    MQTTclient.subscribe((String(base_mqtt_topic) + "/command").c_str());   // Watch the .../command topic for incoming MQTT messages
    MQTTclient.loop();
    // Add other watched topics in here...
  }
  else
  {
    // We get here if the connection failed...
    Serial.print(F("MQTT Connection FAILED, Return Code = "));
    Serial.println(MQTTclient.state());
    Serial.println();
    /*
      MQTTclient.state return code meanings...
      -4 : MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time
      -3 : MQTT_CONNECTION_LOST - the network connection was broken
      -2 : MQTT_CONNECT_FAILED - the network connection failed
      -1 : MQTT_DISCONNECTED - the client is disconnected cleanly
      0 : MQTT_CONNECTED - the client is connected
      1 : MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT
      2 : MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier
      3 : MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection
      4 : MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected
      5 : MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect *
    */
  }
} // End of void MQTT_Connect

#endif
