# Estação IoT para Monitoramento Agrícola

> Protótipo funcional de uma Estação IoT que monitora umidade do solo, temperatura local e dados meteorológicos via satélite (simulado), com envio de dados via MQTT e controle remoto de LEDs de alerta.

---

## Informações do Projeto

| Item | Descrição |
|------|-----------|
| **Título** | Estação IoT para Monitoramento de Solo e Previsão Meteorológica |
| **Data** | 2026 |
| **Plataforma** | ESP32 + Wokwi Simulator |

---

##  Objetivo

Desenvolver um protótipo funcional de uma **Estação IoT** que:

-  Monitora **umidade do solo** (simulada por potenciômetro)
-  Mede **temperatura local** (simulada por potenciômetro)
-  Recebe **dados meteorológicos via satélite** (simulado)
-  Envia todos os dados via **MQTT** para a nuvem
-  Exibe informações em **LCD 16x2**
-  Permite **controle remoto de LEDs** via MQTT

---

## 🛠️ Componentes Utilizados

| Tipo | Componente | Quantidade | Função |
|------|-----------|------------|--------|
| **Entrada 1** | Potenciômetro (Umidade) | 1 | Simula sensor de umidade do solo (0-100%) |
| **Entrada 2** | Potenciômetro (Temperatura) | 1 | Simula sensor de temperatura local (0-50°C) |
| **Saída 1** | LED Vermelho | 1 | Alerta de solo seco |
| **Saída 2** | LED Verde | 1 | Indica recepção de dados do satélite |
| **Interface** | LCD 16x2 I2C | 1 | Exibe dados em tempo real |
| **Comunicação** | Wi-Fi + MQTT | - | Envio de dados para a nuvem |

---

## 🔌 Pinagem do ESP32

| Componente | Pino ESP32 | Observação |
|------------|-----------|------------|
| Sensor Umidade | GPIO 34 | Entrada analógica |
| Sensor Temperatura | GPIO 35 | Entrada analógica |
| LED Vermelho | GPIO 26 | Saída digital |
| LED Verde | GPIO 27 | Saída digital |
| LCD I2C - SDA | GPIO 21 | Comunicação I2C |
| LCD I2C - SCL | GPIO 22 | Comunicação I2C |

---

## Arquitetura MQTT

### Broker Utilizado

| Configuração | Valor |
|--------------|-------|
| **Plataforma** | HiveMQ Cloud |
| **URL** | `f05b7d6dd3404ddfa906daa3d79de4b0.s1.eu.hivemq.cloud` |
| **Porta** | 8883 (TLS) |
| **Usuário** | `isismodd` |
| **Senha** | `Pipoca20` |

### 📨 Tópicos de Publicação (Envio de Dados)

| Tópico | Descrição | Frequência |
|--------|-----------|------------|
| `estacao/solo/sensores` | Dados de umidade e temperatura local | A cada 5 segundos |
| `estacao/satelite/dados` | Dados meteorológicos do satélite | A cada 5 segundos |
| `estacao/completa/status` | Resumo completo da estação | A cada 5 segundos |

### 📩 Tópicos de Inscrição (Recepção de Comandos)

| Tópico | Comandos | Função |
|--------|----------|--------|
| `estacao/comando/led_alerta` | `"ON"` / `"OFF"` | Controle remoto do LED vermelho |
| `estacao/comando/led_satelite` | `"ON"` / `"OFF"` | Controle remoto do LED verde |

---

## Formato dos Dados (JSON)

### 1. `estacao/solo/sensores`

```json
{
  "umidade_solo_percent": 45.2,
  "temperatura_local_celsius": 23.5,
  "alerta_seco": false,
  "alerta_critico": false,
  "timestamp_ms": 123456
}
