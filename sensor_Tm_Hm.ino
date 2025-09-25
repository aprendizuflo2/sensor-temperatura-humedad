#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h> // permite setInsecure()
#include "DHT.h"

#define DHTPIN 4        // GPIO4 = D2 en NodeMCU
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

// --- CONFIGURA ESTO ---
const char* ssid = "SES 2.4G";         // <- tu nombre WiFi
const char* password = "SESINGENIERIA"; // <- tu contraseña WiFi
const char* GAS_URL = "https://script.google.com/macros/s/AKfycbx8jyJSkYYpzJvSoo0AbDedWIk1UwkpZFS4ayXx4Vg9AMMVhCfhiuDWChLSa0qglxRU/exec"; // <- pega la URL de tu Apps Script
const char* DEVICE_NAME = "ESP1";     // nombre del dispositivo (opcional)
// -------------------------

void setup() {
  Serial.begin(115200);
  delay(10);
  dht.begin();
  Serial.println();
  Serial.print("Conectando a WiFi ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  // Espera a conexión
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Conectado. IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Leer sensor
  float t = dht.readTemperature(); // °C
  float h = dht.readHumidity();

  if (isnan(t) || isnan(h)) {
    Serial.println("Error: lectura DHT no válida");
  } else {
    Serial.print("Temp: ");
    Serial.print(t);
    Serial.print(" °C  Hum: ");
    Serial.print(h);
    Serial.println(" %");

    // Enviar a Google Apps Script (vía HTTPS GET)
    if (WiFi.status() == WL_CONNECTED) {
      // crear cliente seguro sin validar certificado (útil para pruebas)
      std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
      client->setInsecure(); // <- evita errores de certificado TLS. Aceptable para pruebas.

      HTTPClient https;
      // crear URL con parámetros GET (escape básico)
      String url = String(GAS_URL) + "?temperature=" + String(t) + "&humidity=" + String(h) + "&source=" + String(DEVICE_NAME);

      Serial.print("Enviando a: ");
      Serial.println(url);

      if (https.begin(*client, url)) {
        int httpCode = https.GET(); // hacemos GET
        if (httpCode > 0) {
          Serial.printf("HTTP code: %d\n", httpCode);
          String payload = https.getString();
          Serial.println("Respuesta: " + payload);
        } else {
          Serial.printf("Error en HTTP GET: %s\n", https.errorToString(httpCode).c_str());
        }
        https.end();
      } else {
        Serial.println("No se pudo iniciar conexión HTTPS");
      }
    } else {
      Serial.println("WiFi no conectado");
    }
  }

  // Espera antes de la próxima lectura (ajusta según necesites)
  delay(60000); // 60 segundos
}