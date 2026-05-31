#include <WiFi.h>
#include <WiFiClientSecure.h>  
#include <PubSubClient.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

// ========== CONFIGURAÇÕES WI-FI ==========
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ========== CONFIGURAÇÕES MQTT  ==========
const char* mqtt_server = "f05b7d6dd3404ddfa906daa3d79de4b0.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "isismodd";
const char* mqtt_password = "Pipoca20";
const char* mqtt_client_id = "ESP32_Estacao_Solo_001";

// Tópicos MQTT
const char* topico_sensores_local = "estacao/solo/sensores";
const char* topico_satelite = "estacao/satelite/dados";
const char* topico_estacao_completa = "estacao/completa/status";
const char* topico_comando_led_alerta = "estacao/comando/led_alerta";
const char* topico_comando_led_satelite = "estacao/comando/led_satelite";

// ========== PINAGEM ==========
const int pinoSensorUmidade = 34;
const int pinoSensorTemperatura = 35;
const int ledAlertaUmidade = 26;
const int ledSateliteOk = 27;

// ========== LCD I2C ==========
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ========== VARIÁVEIS ==========
WiFiClientSecure espClient;  
PubSubClient client(espClient);

float umidadeSolo = 0.0;
float temperaturaLocal = 0.0;
float tempSatelite = 0.0;
float pressaoSatelite = 0.0;
float chuvaSatelite = 0.0;

unsigned long lastSatelliteUpdate = 0;
unsigned long lastMqttPublish = 0;
const unsigned long satelliteInterval = 10000;
const unsigned long mqttInterval = 5000;

// ========== FUNÇÕES ==========
float lerUmidadeSolo() {
  int valorAnalogico = analogRead(pinoSensorUmidade);
  float percentual = map(valorAnalogico, 0, 4095, 0, 100);
  return constrain(percentual, 0, 100);
}

float lerTemperaturaLocal() {
  int valorAnalogico = analogRead(pinoSensorTemperatura);
  float temperatura = map(valorAnalogico, 0, 4095, 0, 500) / 10.0;
  return temperatura;
}

void atualizarDadosSatelite() {
  tempSatelite = random(-100, 450) / 10.0;
  pressaoSatelite = random(9500, 10500) / 10.0;
  chuvaSatelite = random(0, 100);
  
  Serial.println("=================================");
  Serial.println("📡 DADOS RECEBIDOS DOS SATÉLITES:");
  Serial.printf("   Temperatura prevista: %.1f °C\n", tempSatelite);
  Serial.printf("   Pressão atmosférica: %.1f hPa\n", pressaoSatelite);
  Serial.printf("   Probabilidade de chuva: %.0f %%\n", chuvaSatelite);
  Serial.println("=================================");
  
  digitalWrite(ledSateliteOk, HIGH);
  delay(150);
  digitalWrite(ledSateliteOk, LOW);
}

void atualizarDisplay() {
  lcd.clear();
  
  lcd.setCursor(0, 0);
  lcd.print("U:" + String(umidadeSolo, 0) + "%");
  lcd.setCursor(7, 0);
  lcd.print("T:" + String(temperaturaLocal, 1) + "C");
  
  lcd.setCursor(0, 1);
  if (tempSatelite >= 0) {
    lcd.print("Sat:+" + String(tempSatelite, 0) + "C");
  } else {
    lcd.print("Sat:" + String(tempSatelite, 0) + "C");
  }
  
  lcd.setCursor(9, 1);
  lcd.print("Chu:" + String(chuvaSatelite, 0) + "%");
  
  if (umidadeSolo < 30) {
    lcd.setCursor(14, 0);
    lcd.print("!");
  }
}

void controlarAlertaLocal() {
  if (umidadeSolo < 20.0) {
    digitalWrite(ledAlertaUmidade, HIGH);
    client.publish("estacao/eventos/alerta", "CRITICO: Solo muito seco!");
  } else if (umidadeSolo > 30.0) {
    digitalWrite(ledAlertaUmidade, LOW);
  }
}

void publicarSensoresLocal() {
  StaticJsonDocument<256> doc;
  doc["umidade_solo_percent"] = umidadeSolo;
  doc["temperatura_local_celsius"] = temperaturaLocal;
  doc["alerta_seco"] = (umidadeSolo < 30.0);
  doc["alerta_critico"] = (umidadeSolo < 20.0);
  doc["timestamp_ms"] = millis();
  
  char buffer[256];
  serializeJson(doc, buffer);
  
  if (client.publish(topico_sensores_local, buffer)) {
    Serial.println("✅ [MQTT] Tópico 1 publicado: " + String(topico_sensores_local));
    Serial.println("   Conteúdo: " + String(buffer));
  } else {
    Serial.println("❌ [MQTT] Falha ao publicar tópico 1");
  }
}

void publicarDadosSatelite() {
  StaticJsonDocument<256> doc;
  doc["temperatura_prevista_celsius"] = tempSatelite;
  doc["pressao_atmosferica_hPa"] = pressaoSatelite;
  doc["probabilidade_chuva_percent"] = chuvaSatelite;
  doc["fonte"] = "constelacao_satelites_meteorologicos";
  doc["timestamp_ms"] = millis();
  
  char buffer[256];
  serializeJson(doc, buffer);
  
  if (client.publish(topico_satelite, buffer)) {
    Serial.println("✅ [MQTT] Tópico 2 publicado: " + String(topico_satelite));
    Serial.println("   Conteúdo: " + String(buffer));
  } else {
    Serial.println("❌ [MQTT] Falha ao publicar tópico 2");
  }
}

void publicarEstacaoCompleta() {
  StaticJsonDocument<256> doc;
  
  doc["umidade"] = umidadeSolo;
  doc["temp_local"] = temperaturaLocal;
  doc["alerta"] = (umidadeSolo < 30.0);
  doc["temp_sat"] = tempSatelite;
  doc["pressao"] = pressaoSatelite;
  doc["chuva"] = chuvaSatelite;
  
  if (umidadeSolo < 20.0) {
    doc["status"] = "C";
  } else if (umidadeSolo < 50.0) {
    doc["status"] = "A";
  } else {
    doc["status"] = "N";
  }
  
  doc["timestamp"] = millis();
  
  char buffer[256];
  serializeJson(doc, buffer);
  
  if (client.publish(topico_estacao_completa, buffer)) {
    Serial.println("✅ [MQTT] Tópico 3 publicado: " + String(topico_estacao_completa));
    Serial.println("   Conteúdo: " + String(buffer));
  } else {
    Serial.println("❌ [MQTT] Falha ao publicar tópico 3");
  }
  Serial.println("");
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.println("=================================");
  Serial.printf("📩 COMANDO MQTT RECEBIDO: %s -> %s\n", topic, message.c_str());
  
  if (String(topic) == topico_comando_led_alerta) {
    if (message == "ON") {
      digitalWrite(ledAlertaUmidade, HIGH);
      client.publish("estacao/resposta/led_alerta", "LIGADO");
      Serial.println("   🔴 LED VERMELHO LIGADO");
    } else if (message == "OFF") {
      digitalWrite(ledAlertaUmidade, LOW);
      client.publish("estacao/resposta/led_alerta", "DESLIGADO");
      Serial.println("   🔴 LED VERMELHO DESLIGADO");
    }
  }
  
  if (String(topic) == topico_comando_led_satelite) {
    if (message == "ON") {
      digitalWrite(ledSateliteOk, HIGH);
      client.publish("estacao/resposta/led_satelite", "LIGADO");
      Serial.println("   🟢 LED VERDE LIGADO");
    } else if (message == "OFF") {
      digitalWrite(ledSateliteOk, LOW);
      client.publish("estacao/resposta/led_satelite", "DESLIGADO");
      Serial.println("   🟢 LED VERDE DESLIGADO");
    }
  }
  Serial.println("=================================");
}

void setup_wifi() {
  delay(10);
  Serial.println("\n=================================");
  Serial.println("📡 CONECTANDO AO WI-FI...");
  Serial.printf("   SSID: %s\n", ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\n✅ WI-FI CONECTADO!");
  Serial.printf("   IP Address: %s\n", WiFi.localIP().toString().c_str());
  Serial.println("=================================");
}

void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("🔄 Conectando ao broker MQTT...");
    Serial.printf(" %s:%d\n", mqtt_server, mqtt_port);
    
    // Conecta com usuário e senha
    if (client.connect(mqtt_client_id, mqtt_user, mqtt_password)) {
      Serial.println("✅ CONECTADO AO BROKER MQTT!");
      
      client.subscribe(topico_comando_led_alerta);
      client.subscribe(topico_comando_led_satelite);
      client.publish("estacao/status", "online");
      Serial.println("   Status 'online' publicado");
    } else {
      Serial.printf("❌ FALHA! rc=%d\n", client.state());
      Serial.println("   Tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(pinoSensorUmidade, INPUT);
  pinMode(pinoSensorTemperatura, INPUT);
  pinMode(ledAlertaUmidade, OUTPUT);
  pinMode(ledSateliteOk, OUTPUT);
  
  // Inicializa LCD I2C
  lcd.init();
  lcd.backlight();
  lcd.print("Inicializando...");
  lcd.setCursor(0, 1);
  lcd.print("Sistema IoT");
  
  setup_wifi();
  
  
  espClient.setInsecure();
  
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  randomSeed(analogRead(0));
  atualizarDadosSatelite();
  lastSatelliteUpdate = millis();
  
  lcd.clear();
  lcd.print("MQTT: Conect...");
  
  Serial.println("\n🚀 SISTEMA INICIALIZADO!\n");
}

void loop() {
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();
  
  umidadeSolo = lerUmidadeSolo();
  temperaturaLocal = lerTemperaturaLocal();
  
  controlarAlertaLocal();
  
  if (millis() - lastSatelliteUpdate >= satelliteInterval) {
    atualizarDadosSatelite();
    lastSatelliteUpdate = millis();
  }
  
  if (millis() - lastMqttPublish >= mqttInterval) {
    publicarSensoresLocal();
    publicarDadosSatelite();
    publicarEstacaoCompleta();
    lastMqttPublish = millis();
  }
  
  atualizarDisplay();
  delay(500);
}
