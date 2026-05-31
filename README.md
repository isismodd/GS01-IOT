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

```
{
  "umidade_solo_percent": 45.2,
  "temperatura_local_celsius": 23.5,
  "alerta_seco": false,
  "alerta_critico": false,
  "timestamp_ms": 123456
}
```
### 2. `estacao/satelite/dados`

```
{
  "temperatura_prevista_celsius": 22.3,
  "pressao_atmosferica_hPa": 1012.5,
  "probabilidade_chuva_percent": 45,
  "fonte": "constelacao_satelites_meteorologicos",
  "timestamp_ms": 123456
}
```
### 4. `estacao/completa/status`
```
{
  "umidade": 45.2,
  "temp_local": 23.5,
  "alerta": false,
  "temp_sat": 22.3,
  "pressao": 1012.5,
  "chuva": 45,
  "status": "A",
  "timestamp": 123456
}
```

## 📌 Legenda do Campo `status`

| Código | Significado | Condição |
|--------|-------------|----------|
| `"C"` | Crítico | Umidade < 20% |
| `"A"` | Atenção | Umidade entre 20% e 50% |
| `"N"` | Normal | Umidade > 50% |

---

## 📺 Display LCD

O LCD 16x2 exibe em tempo real:
Linha 1: U:45% T:23.5C
Linha 2: Sat:+22C Chu:45%


### Símbolos do Display

| Símbolo | Significado |
|---------|-------------|
| `U:` | Umidade do solo |
| `T:` | Temperatura local |
| `Sat:` | Temperatura prevista por satélite |
| `Chu:` | Probabilidade de chuva |
| `!` | Alerta de solo seco (aparece quando umidade < 30%) |

---

## 🚀 Como Executar

### No Wokwi (Simulação)

1. Copie o código para o editor do Wokwi
2. Cole o `diagram.json` fornecido
3. Clique em **"Start Simulation"**
4. Acompanhe os dados no **Serial Monitor** e no **LCD**

## Dashboard para Visualização
Para visualizar os dados enviados pelo ESP32 em tempo real:

HiveMQ WebSocket Client (Navegador)
- Acesse: https://f05b7d6dd3404ddfa906daa3d79de4b0.s1.eu.hivemq.cloud:8888/websocket

- Faça login com usuário isismodd e senha Pipoca20

- Clique em "Connect"

- Na seção "Subscriptions", inscreva-se no tópico: estacao/#

- As mensagens aparecerão automaticamente
