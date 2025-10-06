#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include "DHT.h"

#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// --- CONFIGURA ESTO ---
const char* ssid = "SES 2.4G";
const char* password = "SESINGENIERIA";
const char* GAS_URL = "https://script.google.com/macros/s/AKfycbx8jyJSkYYpzJvSoo0AbDedWIk1UwkpZFS4ayXx4Vg9AMMVhCfhiuDWChLSa0qglxRU/exec";
const char* DEVICE_NAME = "ESP1";
const char* ESPACIO = "GALPON1";
// -------------------------

// intervalo en milisegundos (1 minuto = 60000)
const unsigned long INTERVALO_MS = 60000UL;
unsigned long previousMillis = 0;

void enviarDatos(float t, float h) {
  if (WiFi.status() == WL_CONNECTED) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    client->setInsecure();

    HTTPClient https;

    String url = String(GAS_URL)
      + "?temperature=" + String(t, 2)
      + "&humidity=" + String(h, 2)
      + "&device=" + String(DEVICE_NAME)
      + "&espacio=" + String(ESPACIO);

    Serial.print("Enviando a: ");
    Serial.println(url);

    if (https.begin(*client, url)) {
      int httpCode = https.GET();
      Serial.print("HTTP code: ");
      Serial.println(httpCode);
      if (httpCode > 0) {
        String payload = https.getString();
        Serial.println("Respuesta: " + payload);
      } else {
        Serial.println("Error en HTTP GET: " + https.errorToString(httpCode));
      }
      https.end();
    } else {
      Serial.println("No se pudo iniciar conexión HTTPS (https.begin falló)");
    }
  } else {
    Serial.println("WiFi no conectado - no se envía");
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  Serial.println();
  Serial.print("Conectando a WiFi ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(250);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Conectado. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("No conectado al inicio, seguirá intentando en background.");
  }

  // 🔹 Primera medición inmediata
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    Serial.println("Primera medición enviada inmediatamente.");
    enviarDatos(t, h);
  } else {
    Serial.println("Error: primera lectura DHT no válida.");
  }
  previousMillis = millis(); // Reinicia contador justo después de la primera lectura
}

void loop() {
  unsigned long currentMillis = millis();

  // Reintenta conexión WiFi si se pierde
  static unsigned long lastWifiAttempt = 0;
  if (WiFi.status() != WL_CONNECTED && currentMillis - lastWifiAttempt >= 10000UL) {
    lastWifiAttempt = currentMillis;
    Serial.println("WiFi desconectado, intentando reconectar...");
    WiFi.begin(ssid, password);
  }

  // 🔹 Cada INTERVALO_MS realiza una nueva lectura y envío
  if (currentMillis - previousMillis >= INTERVALO_MS) {
    previousMillis = currentMillis;

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (isnan(t) || isnan(h)) {
      Serial.println("Error: lectura DHT no válida");
      return;
    }

    Serial.print("Temp: ");
    Serial.print(t);
    Serial.print(" °C  Hum: ");
    Serial.print(h);
    Serial.println(" %");

    enviarDatos(t, h);
  }

  // Aquí puedes hacer otras tareas
}
