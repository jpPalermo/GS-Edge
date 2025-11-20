

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ---------- CONFIGURAÇÕES DE WIFI E MQTT ----------
const char* WIFI_SSID     = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

// Pode usar esse broker público para teste ou trocar pelo IP do seu Node-RED/Mosquitto
const char* MQTT_BROKER   = "test.mosquitto.org";
const uint16_t MQTT_PORT  = 1883;
const char* MQTT_CLIENT_ID = "WorkSafeIoT_ESP32";
const char* TOPIC_STATUS   = "worksafe/status";
const char* TOPIC_TEMP     = "worksafe/temperatura";
const char* TOPIC_HUM      = "worksafe/umidade";
const char* TOPIC_LDR      = "worksafe/luminosidade";
const char* TOPIC_DIST     = "worksafe/postura";
const char* TOPIC_ALERTA   = "worksafe/alertas";

WiFiClient espClient;
PubSubClient client(espClient);

// ---------- PINOS ----------
#define DHTPIN        15
#define DHTTYPE       DHT22
#define LDR_PIN       34    // Entrada analógica
#define TRIG_PIN      5
#define ECHO_PIN      18
#define LED_ALERTA    2     // LED onboard ESP32
#define BUTTON_PAUSA  19    // Botão para simular levantar e resetar tempo sentado

DHT dht(DHTPIN, DHTTYPE);

// ---------- CONTROLE DE TEMPO ----------
unsigned long lastSensorRead = 0;
const unsigned long SENSOR_INTERVAL = 2000; // 2 segundos entre leituras

// Postura
unsigned long posturaRuimStart = 0;
bool posturaRuim = false;

// Pausa / tempo sentado
unsigned long sentadoStart = 0;
const unsigned long TEMPO_PAUSA_MIN = 50; // em minutos (ex.: 50 min)
bool pausaRecomendada = false;

// ---------- FUNÇÕES AUXILIARES ----------

void conectaWiFi() {
  Serial.print("Conectando ao WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void reconectaMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando ao broker MQTT...");
    if (client.connect(MQTT_CLIENT_ID)) {
      Serial.println(" conectado!");
      // Se quiser assinar algum tópico, faça aqui
      // client.subscribe("worksafe/comandos");
    } else {
      Serial.print(" falhou, rc=");
      Serial.print(client.state());
      Serial.println(" tentando novamente em 3 segundos...");
      delay(3000);
    }
  }
}

float medeDistanciaCM() {
  // Pulso no TRIG
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Leitura do ECHO
  long duracao = pulseIn(ECHO_PIN, HIGH, 30000); // timeout 30ms
  if (duracao == 0) {
    return -1.0; // erro ou fora de alcance
  }

  float distancia = (duracao / 2.0) * 0.0343; // velocidade do som ~ 343 m/s
  return distancia;
}

int calculaFatigueScore(float temp, float hum, int ldrValue, float distCM, bool posturaAlerta, bool pausaAlerta) {
  // Score simples de 0 a 100 (quanto maior, mais fadiga)
  int score = 0;

  // Temperatura (ideal ~22-26°C)
  if (temp > 26 && temp <= 30) score += 20;
  else if (temp > 30)          score += 35;

  // Umidade (ideal ~40-60%)
  if (hum < 35 || hum > 65) score += 10;

  // Luminosidade (depende do divisor, aqui só tratamos extremos)
  if (ldrValue < 1000) score += 10;  // muito escuro
  if (ldrValue > 3500) score += 10;  // luz muito forte

  // Distância/postura
  if (distCM > 0) {
    if (distCM < 25 || distCM > 70) score += 20;
  }

  // Alertas diretos
  if (posturaAlerta) score += 15;
  if (pausaAlerta)   score += 20;

  if (score > 100) score = 100;
  return score;
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  pinMode(LDR_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_ALERTA, OUTPUT);
  pinMode(BUTTON_PAUSA, INPUT_PULLUP);

  digitalWrite(LED_ALERTA, LOW);

  dht.begin();

  conectaWiFi();
  client.setServer(MQTT_BROKER, MQTT_PORT);

  sentadoStart = millis(); // começa "sentado" no momento em que liga
}

void loop() {
  if (!client.connected()) {
    reconectaMQTT();
  }
  client.loop();

  unsigned long agora = millis();

  // Verifica se o botão foi pressionado (simula levantar para pausa)
  if (digitalRead(BUTTON_PAUSA) == LOW) {
    // Reset do tempo sentado
    sentadoStart = agora;
    pausaRecomendada = false;
    Serial.println("Botão pressionado: tempo sentado resetado (pausa feita).");
    delay(300); // debounce simples
  }

  if (agora - lastSensorRead >= SENSOR_INTERVAL) {
    lastSensorRead = agora;

    // ----- LEITURA DO DHT22 -----
    float temp = dht.readTemperature();
    float hum  = dht.readHumidity();

    if (isnan(temp) || isnan(hum)) {
      Serial.println("Falha ao ler DHT22!");
      temp = -1;
      hum  = -1;
    }

    // ----- LEITURA DO LDR -----
    int ldrValue = analogRead(LDR_PIN); // 0–4095 no ESP32

    // ----- LEITURA DO HC-SR04 (DISTÂNCIA) -----
    float distCM = medeDistanciaCM();

    // ----- LÓGICA DE POSTURA -----
    // Consideramos postura ruim se muito perto (<25cm) ou muito longe (>70cm)
    bool posturaAtualRuim = false;
    if (distCM > 0) { // leitura válida
      if (distCM < 25.0 || distCM > 70.0) {
        posturaAtualRuim = true;
      }
    }

    if (posturaAtualRuim) {
      if (!posturaRuim) {
        // Começou agora a ficar ruim
        posturaRuimStart = agora;
      }
      posturaRuim = true;
    } else {
      // Postura normal
      posturaRuim = false;
      posturaRuimStart = 0;
    }

    bool alertaPostura = false;
    if (posturaRuim && posturaRuimStart > 0 && (agora - posturaRuimStart >= 120000)) {
      // Postura ruim há mais de 120 segundos (2 min)
      alertaPostura = true;
    }

    // ----- LÓGICA DE PAUSA (TEMPO SENTADO) -----
    unsigned long tempoSentadoMs = agora - sentadoStart;
    float tempoSentadoMin = tempoSentadoMs / 60000.0; // em minutos

    if (tempoSentadoMin >= TEMPO_PAUSA_MIN) {
      pausaRecomendada = true;
    } else {
      // Se ainda não chegou no tempo mínimo, mantém false
      // pausaRecomendada já é resetada ao apertar o botão
    }

    // ----- CÁLCULO DO SCORE DE FADIGA -----
    int fatigueScore = calculaFatigueScore(temp, hum, ldrValue, distCM, alertaPostura, pausaRecomendada);

    // ----- CONTROLE DO LED DE ALERTA -----
    if (alertaPostura || pausaRecomendada || fatigueScore >= 70) {
      digitalWrite(LED_ALERTA, HIGH);
    } else {
      digitalWrite(LED_ALERTA, LOW);
    }

    // ----- LOG NO SERIAL -----
    Serial.println("---- STATUS ----");
    Serial.print("Temp: "); Serial.print(temp); Serial.println(" *C");
    Serial.print("Umid: "); Serial.print(hum); Serial.println(" %");
    Serial.print("LDR : "); Serial.println(ldrValue);
    Serial.print("Dist: "); Serial.print(distCM); Serial.println(" cm");
    Serial.print("Tempo sentado (min): "); Serial.println(tempoSentadoMin);
    Serial.print("Postura alerta: "); Serial.println(alertaPostura ? "SIM" : "NAO");
    Serial.print("Pausa recomendada: "); Serial.println(pausaRecomendada ? "SIM" : "NAO");
    Serial.print("Fatigue score: "); Serial.println(fatigueScore);
    Serial.println("----------------\n");

    // ----- PUBLICAÇÃO MQTT -----
    // Publica JSON compacto no tópico principal
    String payload = "{";
    payload += "\"temp\":" + String(temp, 1) + ",";
    payload += "\"hum\":" + String(hum, 1) + ",";
    payload += "\"ldr\":" + String(ldrValue) + ",";
    payload += "\"dist_cm\":" + String(distCM, 1) + ",";
    payload += "\"tempo_sentado_min\":" + String(tempoSentadoMin, 1) + ",";
    payload += "\"postura_alerta\":" + String(alertaPostura ? "true" : "false") + ",";
    payload += "\"pausa_alerta\":" + String(pausaRecomendada ? "true" : "false") + ",";
    payload += "\"fatigue_score\":" + String(fatigueScore);
    payload += "}";

    client.publish(TOPIC_STATUS, payload.c_str());

    // Publicações individuais (opcionais, mas ajudam no Node-RED)
    client.publish(TOPIC_TEMP, String(temp, 1).c_str());
    client.publish(TOPIC_HUM,  String(hum, 1).c_str());
    client.publish(TOPIC_LDR,  String(ldrValue).c_str());
    client.publish(TOPIC_DIST, String(distCM, 1).c_str());

    // Publica alerta textual
    String alertaMsg = "";
    if (alertaPostura) {
      alertaMsg += "Postura inadequada por mais de 2 minutos. ";
    }
    if (pausaRecomendada) {
      alertaMsg += "Tempo de pausa atingido. Levante-se e alongue. ";
    }
    if (alertaMsg.length() > 0) {
      client.publish(TOPIC_ALERTA, alertaMsg.c_str());
    }
  }
}
