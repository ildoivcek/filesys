#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>

const char* ssid = "ESP32-CAM-Stream";
const char* password = "12345678";

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

WebServer server(80);


void streamHandler() {
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t * _jpg_buf = NULL;
  char * part_buf[64];

  res = httpd_resp_set_type(NULL, "multipart/x-mixed-replace;boundary=frame");
  WiFiClient client = server.client();
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  client.println();

  while (client.connected()) {
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      continue;
    }
    if (fb->format != PIXFORMAT_JPEG) {
      bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
      esp_camera_fb_return(fb);
      fb = NULL;
      if (!jpeg_converted) {
        continue;
      }
    } else {
      _jpg_buf_len = fb->len;
      _jpg_buf = fb->buf;
    }

    client.println("--frame");
    client.println("Content-Type: image/jpeg");
    client.print("Content-Length: ");
    client.println(_jpg_buf_len);
    client.println();
    client.write(_jpg_buf, _jpg_buf_len);
    client.println();
    
    if (fb) {
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if (_jpg_buf) {
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
  }
}

String getContentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  if (filename.endsWith(".css")) return "text/css";
  if (filename.endsWith(".js")) return "application/javascript";
  return "text/plain";
}

void handleUpload() {
  HTTPUpload& upload = server.upload();
  static File fsUploadFile;
  if (upload.status == UPLOAD_FILE_START) {
    String filename = "/" + upload.filename;
    fsUploadFile = LittleFS.open(filename, "w");
    Serial.print("Uploading: "); Serial.println(filename);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile) fsUploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) fsUploadFile.close();
    Serial.println("Upload Size: " + String(upload.totalSize));
  }
}

void handleFileManager() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'></head><body><h1>File Manager</h1>";
  html += "<form method='POST' action='/upload' enctype='multipart/form-data'><input type='file' name='file'><button>Upload</button></form><hr>";
  
  File root = LittleFS.open("/");
  File file = root.openNextFile();
  while(file){
      html += "<a href='/" + String(file.name()) + "'>" + String(file.name()) + "</a><br>";
      file = root.openNextFile();
  }
  html += "<hr><a href='/meshai'>Go to Video Interface</a></body></html>";
  server.send(200, "text/html", html);
}

void handleFileRead(String path) {
  if (LittleFS.exists(path)) {
    File file = LittleFS.open(path, "r");
    server.streamFile(file, getContentType(path));
    file.close();
  } else {
    server.send(404, "text/plain", "File Not Found. Go to /filemanager to upload it.");
  }
}

void setup() {
  Serial.begin(115200);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 2;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera Init Failed");
    return;
  }

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS Init Failed");
    return;
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.print("AP IP Address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/stream", streamHandler); 
  server.on("/filemanager", handleFileManager);
  server.on("/upload", HTTP_POST, [](){
      server.sendHeader("Location", "/filemanager");
      server.send(302, "text/plain", ""); 
  }, handleUpload);

  server.on("/meshai", [](){ handleFileRead("/meshai.html"); });

  server.onNotFound([](){ handleFileRead(server.uri()); });

  server.begin();
  Serial.println("HTTP Server started");
}

void loop() {
  server.handleClient();
}
