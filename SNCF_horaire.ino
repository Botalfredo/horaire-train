#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>  // N'oublie pas d'installer cette bibliothèque
#include <time.h>
/*
stop_point:SNCF:87612002:Train Labège Innopole (Labège)

stop_point:SNCF:87611004:Train Toulouse Matabiau

https://ec6e6867-47ed-496e-9e4b-07630e8a757b@api.sncf.com/v1/coverage/sncf/journeys?from=stop_area:SNCF:87612002&to=stop_area:SNCF:87611004&count=5

https://api.sncf.com/v1/coverage/sncf/stop_areas/stop_area:SNCF:87612002/departures?direction=stop_area:SNCF:87611004&data_freshness=realtime


*/
// --- CONFIGURATION ---
const char* ssid = "WiFi-nistere";
const char* password = "mikHz5GDKdybDnifGp";
const char* TOKEN_API = "ec6e6867-47ed-496e-9e4b-07630e8a757b";  // Toujours garder ça secret en théorie !
const char* URL = "https://api.sncf.com/v1/coverage/sncf/stop_areas/stop_area:SNCF:87612002/departures?count=10&data_freshness=realtime";

// Délai entre chaque vérification (ici 60 secondes)
const int DELAI_RAFRAICHISSEMENT = 60000;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Connexion au Wi-Fi
  Serial.println("\n--- Démarrage ---");
  Serial.print("Connexion au WiFi : ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connecté !");
  Serial.print("Adresse IP : ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    recupererHoraires();
  } else {
    Serial.println("Erreur : Déconnecté du Wi-Fi.");
  }

  // Attendre avant la prochaine requête
  delay(DELAI_RAFRAICHISSEMENT);
}

void recupererHoraires() {
  Serial.println("\nRécupération des horaires en temps réel...");

  WiFiClientSecure client;
  client.setInsecure();  // On ignore la vérification du certificat SSL

  HTTPClient http;
  http.begin(client, URL);

  // Authentification
  http.setAuthorization(TOKEN_API, "");

  int httpResponseCode = http.GET();

  if (httpResponseCode == 200) {
    // 1. UTILISER LA MÉMOIRE : On télécharge l'intégralité des 23 Ko dans une variable String.
    // L'ESP32-C3 a largement la place, et ça évite les coupures réseau ("IncompleteInput") !
    String payload = http.getString();

    Serial.println(payload);
    afficherHorairesTrains(payload);

  } else {
    Serial.printf("Erreur %d : Impossible de joindre l'API.\n", httpResponseCode);
  }

  http.end();  // Libérer les ressources proprement
}

void afficherHorairesTrains(const String& jsonString) {
  
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, jsonString);

  if (error) {
    Serial.print("Erreur de lecture du JSON : ");
    Serial.println(error.c_str());
    return;
  }

  JsonArray departures = doc["departures"];
  
  Serial.println("=== Prochains départs ===");

  for (JsonObject departure : departures) {
    
    String direction = departure["display_informations"]["direction"].as<String>();
    String ligne = departure["display_informations"]["code"].as<String>();
    
    // Récupération des deux heures (théorique et réelle)
    String baseDateTime = departure["stop_date_time"]["base_departure_date_time"].as<String>();
    String realDateTime = departure["stop_date_time"]["departure_date_time"].as<String>();
    
    if (baseDateTime.length() >= 13 && realDateTime.length() >= 13) {
      // Extraction HH et MM pour l'heure prévue
      String heurePrevue = baseDateTime.substring(9, 11);
      String minutePrevue = baseDateTime.substring(11, 13);
      
      // Extraction HH et MM pour l'heure réelle
      String heureReelle = realDateTime.substring(9, 11);
      String minuteReelle = realDateTime.substring(11, 13);
      
      // Affichage de base
      Serial.print("Ligne ");
      Serial.print(ligne);
      Serial.print(" > Dest : ");
      Serial.print(direction);
      
      // Vérification du retard
      if (baseDateTime != realDateTime) {
        // Il y a une différence, donc un retard
        Serial.print(" | Prévu: ");
        Serial.print(heurePrevue);
        Serial.print(":");
        Serial.print(minutePrevue);
        Serial.print(" -> ⚠️ RETARD : Départ à ");
        Serial.print(heureReelle);
        Serial.print(":");
        Serial.println(minuteReelle);
      } else {
        // Le train est à l'heure
        Serial.print(" | À l'heure : ");
        Serial.print(heurePrevue);
        Serial.print(":");
        Serial.println(minutePrevue);
      }
    }
  }
  
  Serial.println("=========================");
}