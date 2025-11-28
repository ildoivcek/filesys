#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

const char* ssid = "ssid";      
const char* password = "pass";  

ESP8266WebServer server(80);

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\nESP8266 Starting...");

  if (!LittleFS.begin()) {
    Serial.println("Failed to mount LittleFS");
    return;
  }
  Serial.println("LittleFS mounted successfully");

  Dir dir = LittleFS.openDir("/");
  Serial.println("Files in LittleFS:");
  while (dir.next()) {
    Serial.print("  - ");
    Serial.println(dir.fileName());
  }

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
  Serial.print("Visit http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  server.handleClient();
}

void handleRoot() {
  Serial.println("Root page requested");
  
  File file = LittleFS.open("/meshai.html", "r");
  if (!file) {
    Serial.println("Failed to open meshai.html");
    server.send(404, "text/plain", "File not found");
    return;
  }
  
  server.streamFile(file, "text/html");
  file.close();
  Serial.println("HTML file sent");
}

void handleNotFound() {
  Serial.print("404: ");
  Serial.println(server.uri());
  server.send(404, "text/plain", "404: Not Found");
}