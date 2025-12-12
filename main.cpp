#include <Arduino.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

ESP8266WebServer server(80);

String getContentType(String filename) {
    if (filename.endsWith(".html")) return "text/html";
    if (filename.endsWith(".htm")) return "text/html";
    if (filename.endsWith(".css")) return "text/css";
    if (filename.endsWith(".js")) return "application/javascript";
    if (filename.endsWith(".ico")) return "image/x-icon";
    if (filename.endsWith(".txt")) return "text/plain";
    if (filename.endsWith(".jpg")) return "image/jpeg";
    if (filename.endsWith(".png")) return "image/png";
    return "application/octet-stream";
}

void sendFile(String path) {
    if (!LittleFS.exists(path)) {
        server.send(404, "text/plain", "Not found: " + path);
        return;
    }
    File file = LittleFS.open(path, "r");
    server.streamFile(file, getContentType(path));
    file.close();
}

void handleFileManager() {
    String html = "<!DOCTYPE html><html lang='uk'><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>------</title>";
    html += "<style>";
    html += "body { font-family: Arial; padding: 20px; background: #2f2f2f; color: white; }";
    html += "#files { margin-top: 15px; }";
    html += "button { padding: 5px 10px; margin: 5px; cursor: pointer; }";
    html += ".file { padding: 6px; border-bottom: 1px solid #444; display: flex; justify-content: space-between; align-items: center; }";
    html += ".file:hover { background: #444; }";
    html += ".file a { color: #4CAF50; text-decoration: none; }";
    html += ".delete-btn { color: #ff4444; cursor: pointer; padding: 5px; }";
    html += ".upload-form { margin: 20px 0; padding: 15px; background: #444; border-radius: 5px; }";
    html += "</style></head><body>";
    
    html += "<h1>📁File system6</h1>";
    
    html += "<div class='upload-form'>";
    html += "<form method='POST' action='/upload' enctype='multipart/form-data'>";
    html += "<input type='file' name='file' required>";
    html += "<button type='submit'>Upload file</button>";
    html += "</form>";
    html += "</div>";
    
    html += "<h2>Files:</h2>";
    html += "<div id='files'>";
    
    Dir dir = LittleFS.openDir("/");
    int count = 0;
    while (dir.next()) {
        String fileName = dir.fileName();
        size_t fileSize = dir.fileSize();
        html += "<div class='file'>";
        html += "<a href='" + fileName + "'>" + fileName + " (" + String(fileSize) + " bytes)</a>";
        html += "<span class='delete-btn' onclick=\"if(confirm('Delete " + fileName + "?')) window.location='/delete?file=" + fileName + "'\"> Delete</span>";
        html += "</div>";
        count++;
    }
    
    if (count == 0) {
        html += "<p>File system is empty</p>";
    }
    
    html += "</div>";
    html += "<br><a href='/' style='color: #4CAF50;'>← Back</a>";
    html += "</body></html>";
    
    server.send(200, "text/html", html);
}

void handleList() {
    String json = "[";
    bool first = true;
    Dir dir = LittleFS.openDir("/");
    while (dir.next()) {
        if (!first) json += ",";
        json += "\"" + dir.fileName() + "\"";
        first = false;
    }
    json += "]";
    server.send(200, "application/json", json);
}

void handleUpload() {
    HTTPUpload& upload = server.upload();
    static File uploadFile;
    
    if (upload.status == UPLOAD_FILE_START) {
        String filename = "/" + upload.filename;
        uploadFile = LittleFS.open(filename, "w");
        Serial.print("Upload Start: ");
        Serial.println(filename);
    } 
    else if (upload.status == UPLOAD_FILE_WRITE) {
        if (uploadFile) {
            uploadFile.write(upload.buf, upload.currentSize);
        }
    } 
    else if (upload.status == UPLOAD_FILE_END) {
        if (uploadFile) {
            uploadFile.close();
            Serial.print("Upload Complete: ");
            Serial.println(upload.totalSize);
        }
    }
}

void handleUploadResult() {
    server.sendHeader("Location", "/filemanager");
    server.send(302);
}

void handleDelete() {
    if (!server.hasArg("file")) {
        server.send(400, "text/plain", "No file specified");
        return;
    }
    String filename = server.arg("file");
    if (LittleFS.exists(filename)) {
        LittleFS.remove(filename);
        Serial.print("Deleted: ");
        Serial.println(filename);
    }
    server.sendHeader("Location", "/filemanager");
    server.send(302);
}

void setup() {
    Serial.begin(115200);
    Serial.println("\nStarting ESP8266...");
    
    if (!LittleFS.begin()) {
        Serial.println("LittleFS mount failed!");
    } else {
        Serial.println("LittleFS mounted successfully");
    }
    
    WiFi.mode(WIFI_AP);
    WiFi.softAP("MyESP", "12345678");
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
    
    server.on("/", [](){ 
        if (LittleFS.exists("/meshai.html")) {
            sendFile("/meshai.html");
        } else {
            server.send(200, "text/html", "<h1>Завантажте meshai.html через <a href='/filemanager'>файловий менеджер</a></h1>");
        }
    });
    
    server.on("/filemanager", handleFileManager);  
    server.on("/list", handleList);                 
    server.on("/upload", HTTP_POST, handleUploadResult, handleUpload);
    server.on("/delete", handleDelete);
    

    server.onNotFound([]() {
        String path = server.uri();
        if (LittleFS.exists(path)) {
            sendFile(path);
        } else {
            server.send(404, "text/plain", "File not found");
        }
    });
    
    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
    server.handleClient();
}
