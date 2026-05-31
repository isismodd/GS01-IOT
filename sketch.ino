#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal.h>
#include <ArduinoJson.h>

// ========== CONFIGURAÇÕES WI-FI ==========
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ========== CONFIGURAÇÕES MQTT ==========
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_client_id = "ESP32_Estacao_Solo_001";

// Tópicos MQTT (3 obrigatórios para o trabalho)
const char* topico_sensores_local = "estacao/solo/sensores";
const char* topico_satelite = "estacao/satelite/dados";
const char* topico_estacao_completa = "estacao/completa/status";

// Tópicos para controle dos LEDs via MQTT
const char* topico_comando_led_alerta = "estacao/comando/led_alerta";
const char* topico_comando_led_satelite = "estacao/comando/led_satelite";

// ========== PINAGEM DOS COMPONENTES ==========
const int pinoSensorUmidade = 34;     // Potenciômetro 1 (simula umidade do solo)
const int pinoSensorTemperatura = 35; // Potenciômetro 2 (simula temperatura local)
const int ledAlertaUmidade = 26;      // LED vermelho - alerta solo seco
const int ledSateliteOk = 27;         // LED verde - recepção de satélite

// Configuração do LCD (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(19, 18, 17, 16, 15, 14);

// ========== VARIÁVEIS GLOBAIS ==========
WiFiClient espClient;
PubSubClient client(espClient);

float umidadeSolo = 0.0;      // Percentual de umidade (0 a 100%)
float temperaturaLocal = 0.0; // Temperatura em °C (0 a 50)
float tempSatelite = 0.0;     // Temperatura prevista por satélite
float pressaoSatelite = 0.0;  // Pressão atmosférica (hPa)
float chuvaSatelite = 0.0;    // Probabilidade de chuva (%)

unsigned long lastSatelliteUpdate = 0;
unsigned long lastMqttPublish = 0;
const unsigned long satelliteInterval = 10000; // Atualiza satélite a cada 10s
const unsigned long mqttInterval = 5000;       // Publica MQTT a cada 5s

// ========== FUNÇÕES DOS SENSORES ==========
float lerUmidadeSolo() {
  int valorAnalogico = analogRead(pinoSensorUmidade);
  // Mapeia 0-4095 para 0-100%
  float percentual = map(valorAnalogico, 0, 4095, 0, 100);
  return constrain(percentual, 0, 100);
}

float lerTemperaturaLocal() {
  int valorAnalogico = analogRead(pinoSensorTemperatura);
  // Mapeia 0-4095 para 0-50°C (multiplica por 10 para manter 1 casa decimal)
  float temperatura = map(valorAnalogico, 0, 4095, 0, 500) / 10.0;
  return temperatura;
}

// ========== SIMULAÇÃO DE DADOS DO SATÉLITE ==========
void atualizarDadosSatelite() {
  // Simula recepção de dados meteorológicos de uma constelação de satélites
  // (em um projeto real, isso viria via API ou downlink real)
  
  tempSatelite = random(-100, 450) / 10.0;  // -10°C a 45°C
  pressaoSatelite = random(9500, 10500) / 10.0; // 950 a 1050 hPa
  chuvaSatelite = random(0, 100);           // 0% a 100%
  
  Serial.println("=================================");
  Serial.println("📡 DADOS RECEBIDOS DOS SATÉLITES:");
  Serial.printf("   Temperatura prevista: %.1f °C\n", tempSatelite);
  Serial.printf("   Pressão atmosférica: %.1f hPa\n", pressaoSatelite);
  Serial.printf("   Probabilidade de chuva: %.0f %%\n", chuvaSatelite);
  Serial.println("=================================");
  
  // Pisca o LED verde para indicar recepção de dados do satélite
  digitalWrite(ledSateliteOk, HIGH);
  delay(150);
  digitalWrite(ledSateliteOk, LOW);
}

// ========== DISPLAY LCD ==========
void atualizarDisplay() {
  lcd.clear();
  
  // Linha 1: Dados do solo (umidade e temperatura local)
  lcd.setCursor(0, 0);
  lcd.print("U:" + String(umidadeSolo, 0) + "%");
  lcd.setCursor(7, 0);
  lcd.print("T:" + String(temperaturaLocal, 1) + "C");
  
  // Linha 2: Dados do satélite (temperatura e chuva)
  lcd.setCursor(0, 1);
  if (tempSatelite >= 0) {
    lcd.print("Sat:+" + String(tempSatelite, 0) + "C");
  } else {
    lcd.print("Sat:" + String(tempSatelite, 0) + "C");
  }
  
  lcd.setCursor(9, 1);
  lcd.print("Chu:" + String(chuvaSatelite, 0) + "%");
  
  // Se a umidade estiver baixa, adiciona alerta no canto
  if (umidadeSolo < 30) {
    lcd.setCursor(14, 0);
    lcd.print("!");
  }
}

// ========== CONTROLE LOCAL DO LED DE ALERTA ==========
void controlarAlertaLocal() {
  // Alerta automático baseado na umidade do solo
  // O LED só acende automaticamente em caso crítico
  if (umidadeSolo < 20.0) {
    digitalWrite(ledAlertaUmidade, HIGH);
    client.publish("estacao/eventos/alerta", "CRITICO: Solo muito seco!");
  } 
  // Entre 20% e 30% mantém o estado atual (não muda)
  // Acima de 50% e estava aceso, não desliga automaticamente (aguarda comando MQTT)
}

// ========== PUBLICAÇÕES MQTT (3 TÓPICOS OBRIGATÓRIOS) ==========

// Tópico 1: Dados dos sensores locais (umidade e temperatura do solo)
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

// Tópico 2: Dados recebidos do satélite
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

// Tópico 3: Estação completa (dados locais + satélite) - VERSÃO CORRIGIDA
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

// ========== CALLBACK PARA COMANDOS VIA MQTT ==========
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  Serial.println("=================================");
  Serial.printf("📩 COMANDO MQTT RECEBIDO:\n");
  Serial.printf("   Tópico: %s\n", topic);
  Serial.printf("   Mensagem: %s\n", message.c_str());
  
  // Controle do LED vermelho (alerta de solo seco)
  if (String(topic) == topico_comando_led_alerta) {
    if (message == "ON") {
      digitalWrite(ledAlertaUmidade, HIGH);
      Serial.println("   🔴 LED VERMELHO (alerta) LIGADO via MQTT");
      client.publish("estacao/resposta/led_alerta", "LIGADO");
    } 
    else if (message == "OFF") {
      digitalWrite(ledAlertaUmidade, LOW);
      Serial.println("   🔴 LED VERMELHO (alerta) DESLIGADO via MQTT");
      client.publish("estacao/resposta/led_alerta", "DESLIGADO");
    }
    else {
      Serial.println("   ⚠️ Comando inválido. Use ON ou OFF");
    }
  }
  
  // Controle do LED verde (comunicação com satélite)
  if (String(topic) == topico_comando_led_satelite) {
    if (message == "ON") {
      digitalWrite(ledSateliteOk, HIGH);
      Serial.println("   🟢 LED VERDE (satélite) LIGADO via MQTT");
      client.publish("estacao/resposta/led_satelite", "LIGADO");
    } 
    else if (message == "OFF") {
      digitalWrite(ledSateliteOk, LOW);
      Serial.println("   🟢 LED VERDE (satélite) DESLIGADO via MQTT");
      client.publish("estacao/resposta/led_satelite", "DESLIGADO");
    }
    else {
      Serial.println("   ⚠️ Comando inválido. Use ON ou OFF");
    }
  }
  Serial.println("=================================");
}

// ========== CONEXÃO WI-FI ==========
void setup_wifi() {
  delay(10);
  Serial.println("\n================================");
  Serial.println("📡 CONECTANDO AO WI-FI...");
  Serial.printf("   SSID: %s\n", ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\n✅ WI-FI CONECTADO!");
  Serial.printf("   IP Address: %s\n", WiFi.localIP().toString().c_str());
  Serial.println("================================");
}

// ========== RECONEXÃO MQTT ==========
void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("🔄 Conectando ao broker MQTT...");
    Serial.printf(" %s:%d\n", mqtt_server, mqtt_port);
    
    // Tenta conectar
    if (client.connect(mqtt_client_id)) {
      Serial.println("✅ CONECTADO AO BROKER MQTT!");
      
      // Inscreve nos tópicos de comando
      if (client.subscribe(topico_comando_led_alerta)) {
        Serial.printf("   Inscrito no tópico: %s\n", topico_comando_led_alerta);
      }
      if (client.subscribe(topico_comando_led_satelite)) {
        Serial.printf("   Inscrito no tópico: %s\n", topico_comando_led_satelite);
      }
      
      // Publica mensagem de status online
      client.publish("estacao/status", "online");
      Serial.println("   Status 'online' publicado");
    } 
    else {
      Serial.printf("❌ FALHA! rc=%d\n", client.state());
      Serial.println("   Tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  
  // Inicializa pinos
  pinMode(pinoSensorUmidade, INPUT);
  pinMode(pinoSensorTemperatura, INPUT);
  pinMode(ledAlertaUmidade, OUTPUT);
  pinMode(ledSateliteOk, OUTPUT);
  
  // Inicializa LCD
  lcd.begin(16, 2);
  lcd.print("Inicializando...");
  lcd.setCursor(0, 1);
  lcd.print("Sistema IoT");
  
  // Conecta Wi-Fi
  setup_wifi();
  
  // Configura MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  
  // Gera semente aleatória para dados do satélite
  randomSeed(analogRead(0));
  
  // Inicializa dados do satélite
  atualizarDadosSatelite();
  lastSatelliteUpdate = millis();
  
  // Limpa display e mostra prontidão
  lcd.clear();
  lcd.print("MQTT: Conect...");
  
  Serial.println("\n🚀 SISTEMA INICIALIZADO!");
  Serial.println("Aguardando dados...\n");
}

// ========== LOOP PRINCIPAL ==========
void loop() {
  // Garante conexão MQTT
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();
  
  // Leitura dos sensores locais
  umidadeSolo = lerUmidadeSolo();
  temperaturaLocal = lerTemperaturaLocal();
  
  // Controle automático do LED de alerta baseado na umidade
  controlarAlertaLocal();
  
  // Atualiza dados do satélite periodicamente
  if (millis() - lastSatelliteUpdate >= satelliteInterval) {
    atualizarDadosSatelite();
    lastSatelliteUpdate = millis();
  }
  
  // Publica dados via MQTT periodicamente
  if (millis() - lastMqttPublish >= mqttInterval) {
    publicarSensoresLocal();      // Tópico 1 obrigatório ✅
    publicarDadosSatelite();       // Tópico 2 obrigatório ✅
    publicarEstacaoCompleta();     // Tópico 3 obrigatório ✅
    lastMqttPublish = millis();
  }
  
  // Atualiza o display LCD
  atualizarDisplay();
  
  // Pequeno delay para estabilidade
  delay(500);
}