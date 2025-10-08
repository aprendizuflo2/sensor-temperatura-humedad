#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>
#include "DHT.h"

#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// --- CONFIG WIFI ---
const char* ssid = "SAMU";
const char* password = "123456788";

// --- CONFIG SCRIPT ---
const char* GAS_URL = "https://script.google.com/macros/s/AKfycbx8jyJSkYYpzJvSoo0AbDedWIk1UwkpZFS4ayXx4Vg9AMMVhCfhiuDWChLSa0qglxRU/exec";
const char* DEVICE_NAME = "ESP1";

// --- Variables dinámicas ---
String ESPACIO = "";
unsigned long INTERVALO_MS = 60000UL;
unsigned long previousMillis = 0;

// --- Función para obtener parámetros desde hoja "Parametros" ---
void obtenerParametros() {
  Serial.println("📡 Solicitando configuración desde Google Sheets...");

  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;

  String url = String(GAS_URL) + "?action=getParams&device=" + DEVICE_NAME;

  if (https.begin(*client, url)) {
    int httpCode = https.GET();
    Serial.print("HTTP code: ");
    Serial.println(httpCode);

    if (httpCode > 0) {
      String payload = https.getString();
      Serial.println("Respuesta: " + payload);

      if (httpCode == HTTP_CODE_OK) {
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, payload);

        if (!error && doc["status"] == "OK") {
          ESPACIO = doc["espacio"].as<String>();
          INTERVALO_MS = doc["intervalo_min"].as<int>() * 60000UL;

          Serial.println("✅ Configuración cargada:");
          Serial.println("ESPACIO: " + ESPACIO);
          Serial.print("INTERVALO (min): ");
          Serial.println(INTERVALO_MS / 60000);
        } else {
          Serial.println("⚠️ No se encontraron parámetros válidos para este dispositivo.");
        }
      }
    } else {
      Serial.println("❌ Error HTTP al obtener parámetros: " + https.errorToString(httpCode));
    }

    https.end();
  } else {
    Serial.println("❌ Falló https.begin() al obtener parámetros");
  }
}

// --- Función para enviar lecturas a hoja "Lecturas" ---
void enviarDatos(float t, float h) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi desconectado - no se envía");
    return;
  }

  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  client->setInsecure();
  HTTPClient https;

  String url = String(GAS_URL)
    + "?action=sendData"
    + "&temperature=" + String(t, 2)
    + "&humidity=" + String(h, 2)
    + "&device=" + String(DEVICE_NAME)
    + "&espacio=" + String(ESPACIO);

  Serial.print("🚀 Enviando datos a: ");
  Serial.println(url);

  if (https.begin(*client, url)) {
    int httpCode = https.GET();
    Serial.print("HTTP code: ");
    Serial.println(httpCode);

    if (httpCode > 0) {
      String payload = https.getString();
      Serial.println("Respuesta: " + payload);
    }

    https.end();
  } else {
    Serial.println("❌ Falló https.begin() al enviar datos");
  }
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  dht.begin();
  WiFi.begin(ssid, password);

  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi conectado.");

  // Leer parámetros de la hoja
  obtenerParametros();

  // Primera lectura inmediata
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    enviarDatos(t, h);
  }

  previousMillis = millis();
}

// --- LOOP ---
void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= INTERVALO_MS) {
    previousMillis = currentMillis;

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (!isnan(t) && !isnan(h)) {
      enviarDatos(t, h);
    } else {
      Serial.println("⚠️ Error de lectura del DHT");
    }
  }
}
