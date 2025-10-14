while (!client.connected()) {
    Serial.println("Reconectando a MQTT...");
    if (client.connect("ArduinoClient", mqtt_username, mqtt_password)) {
      Serial.println("Conectado");
      client.subscribe(mqtt_topic_door1);
    } else {
      Serial.print("Error al reconectar: ");
      Serial.print(client.state());
      delay(5000);
    }
  }