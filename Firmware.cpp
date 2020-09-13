#include <Arduino.h>

/****************************************
 * Include Libraries
 ****************************************/
#include <WiFi.h>
#include <PubSubClient.h>
#include "SSD1306Wire.h"
// Include the UI lib
#include <OLEDDisplayUi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

// Space to store values to send
SSD1306Wire display(0x3c, 5, 4);
OLEDDisplayUi ui ( &display );

#define WIFISSID "*******"               // Put your WifiSSID here
#define PASSWORD "*******"               // Put your wifi password here
#define TOKEN "**************" // Put your Ubidots' TOKEN
#define MQTT_CLIENT_NAME "**************"



/****************************************
 * Define Constants
 ****************************************/
#define DEVICE_STATUS "status"
#define SUBSCRIBE_LED "led"     // Assing the variable label
#define SUBSCRIBE_FAN "fan" // Assing the variable label
#define SUBSCRIBE_SOCKET "socket" // Assing the variable label
#define SUBSCRIBE_RIGHT_LED "rightled"       // Assing the variable label
#define SUBSCRIBE_LEFT_LED "leftled"       // Assing the variable label
#define SUBSCRIBE_POP_LIGHTS "poplights" // Assing the variable label
#define SUBSCRIBE_GENERAL_SWITCH "gswitch"
#define DEVICE_LABEL "esp32"             // Assig the device label

#define led 0 // Set the GPIO26 as RELAY
#define fan 2
#define socket 14
#define rightled 12 // Set the GPIO26 as RELAY
#define leftled 13
#define poplights 26
#define gswitch 25

const char *host = "esp32";
char mqttBroker[] = "things.ubidots.com";
char payload[100];
char topic [150];
char topicSubscribeled[100];
char topicSubscribefan[100];
char topicSubscribesocket[100];
char topicSubscriberightled[100];
char topicSubscribeleftled[100];
char topicSubscribepoplights[100];
char topicSubscribegeneralswitch[100];
int status;
String ledstatus = "OFF";
String fanstatus = "OFF";
String socketstatus = "OFF";
String rightledstatus = "OFF";
String leftledstatus = "OFF";
String poplightsstatus = "OFF";
String gswitchstatus = "OFF";

WebServer server(80);

/*
 * Login page
 */

const char *loginIndex =
    "<form name='loginForm'>"
    "<table width='20%' bgcolor='A09F9F' align='center'>"
    "<tr>"
    "<td colspan=2>"
    "<center><font size=4><b>ESP32 Login Page</b></font></center>"
    "<br>"
    "</td>"
    "<br>"
    "<br>"
    "</tr>"
    "<td>Username:</td>"
    "<td><input type='text' size=25 name='userid'><br></td>"
    "</tr>"
    "<br>"
    "<br>"
    "<tr>"
    "<td>Password:</td>"
    "<td><input type='Password' size=25 name='pwd'><br></td>"
    "<br>"
    "<br>"
    "</tr>"
    "<tr>"
    "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
    "</tr>"
    "</table>"
    "</form>"
    "<script>"
    "function check(form)"
    "{"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{"
    "window.open('/serverIndex')"
    "}"
    "else"
    "{"
    " alert('Error Password or Username')/*displays error message*/"
    "}"
    "}"
    "</script>";

/*
 * Server Index Page
 */

const char *serverIndex =
    "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
    "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
    "<input type='file' name='update'>"
    "<input type='submit' value='Update'>"
    "</form>"
    "<div id='prg'>progress: 0%</div>"
    "<script>"
    "$('form').submit(function(e){"
    "e.preventDefault();"
    "var form = $('#upload_form')[0];"
    "var data = new FormData(form);"
    " $.ajax({"
    "url: '/update',"
    "type: 'POST',"
    "data: data,"
    "contentType: false,"
    "processData:false,"
    "xhr: function() {"
    "var xhr = new window.XMLHttpRequest();"
    "xhr.upload.addEventListener('progress', function(evt) {"
    "if (evt.lengthComputable) {"
    "var per = evt.loaded / evt.total;"
    "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
    "}"
    "}, false);"
    "return xhr;"
    "},"
    "success:function(d, s) {"
    "console.log('success!')"
    "},"
    "error: function (a, b, c) {"
    "}"
    "});"
    "});"
    "</script>";

/****************************************
 * Auxiliar Functions
 ****************************************/
WiFiClient ubidots;
PubSubClient client(ubidots);

void welcome_msg()
{
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_24);
  display.drawString(0, 26, "Welcome");
  display.display();
  delay(1000);
  display.clear();
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.println("Attempting MQTT connection...");
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 10, "Attempting MQTT connection...");
    display.display();
    delay(1000);
    // Attemp to connect
    if (client.connect(MQTT_CLIENT_NAME, TOKEN, ""))
    {
      Serial.println("Connected");
      display.clear();
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_16);
      display.drawString(0, 10, "Connected");
      display.display();
      delay(1000);
      
      client.subscribe(topicSubscribeled);
      client.subscribe(topicSubscribefan);
      client.subscribe(topicSubscribesocket);
      client.subscribe(topicSubscriberightled);
      client.subscribe(topicSubscribeleftled);
      client.subscribe(topicSubscribepoplights);
      client.subscribe(topicSubscribegeneralswitch);
    }
    else
    {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      // Wait 2 seconds before retrying
      display.clear();
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_16);
      display.drawString(0, 0, "Failed, rc=" + String(client.state()));
      display.setFont(ArialMT_Plain_16);
      display.drawString(0, 10, "try again in 2 seconds");
      display.display();
      delay(1000);
    }
  }
}

void drawFrame1()
{
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 10, "LED " + ledstatus);
  display.drawString(0, 20, "FAN " + fanstatus);
  display.drawString(0, 30, "SOCKET " + socketstatus);
  display.drawString(0, 40, "POPLIGHTS " + poplightsstatus);
}

void drawFrame2()
{
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 10, "RIGHTLED " + rightledstatus);
  display.drawString(0, 20, "LEFTLED " + leftledstatus);
  display.drawString(0, 30, "G.SWITCH " + gswitchstatus);
}

void callback(char *topic, byte *payload, char value)
{
  char p[value];
  memcpy(p, payload, value);
  p[value] = {};
  String message(p);
  if (strcmp(topic, "/v1.6/devices/esp32/led/lv") == 0)
  {
    if (message == "0")
    {
      digitalWrite(led, LOW);
      Serial.println(topic);
      Serial.print("OFF");
      ledstatus = "OFF";
      display.clear();
      drawFrame1();
      display.display();
      delay(1000);
    }
  else
    {
      digitalWrite(led, HIGH);
      Serial.println(topic);
      Serial.print("ON");
      ledstatus = "ON";
      display.clear();
      drawFrame1();
      display.display();
      delay(1000);
    }
  }
  else if (strcmp(topic, "/v1.6/devices/esp32/fan/lv") == 0)
  {
    if (message == "0")
    {
      digitalWrite(fan, LOW);
      Serial.println(topic);
      Serial.print("OFF");
      fanstatus = "OFF";
      display.clear();
      drawFrame1();
      display.display();
      delay(1000);
    }
    else
    {
      digitalWrite(fan, HIGH);
      Serial.println(topic);
      Serial.print("ON");
      fanstatus = "ON";
      display.clear();
      drawFrame1();
      display.display();
      delay(1000);
    }
  }
  else if (strcmp(topic, "/v1.6/devices/esp32/socket/lv") == 0)
  {
    if (message == "0")
    {
      digitalWrite(socket, LOW);
      Serial.println(topic);
      Serial.print("OFF");
      socketstatus = "OFF";
      display.clear();
      drawFrame1();
      display.display();
      delay(1000);
    }
    else
    {
      digitalWrite(socket, HIGH);
      Serial.println(topic);
      Serial.print("ON");
      socketstatus = "ON";
      display.clear();
      drawFrame1();
      display.display();
      delay(1000);
    }
  }
  else if (strcmp(topic, "/v1.6/devices/esp32/rightled/lv") == 0)
  {
    if (message == "0")
    {
      digitalWrite(rightled, LOW);
      Serial.println(topic);
      Serial.print("OFF");
      rightledstatus = "OFF";
      display.clear();
      drawFrame2();
      display.display();
      delay(1000);
    }
    else
    {
      digitalWrite(rightled, HIGH);
      Serial.println(topic);
      Serial.print("ON");
      rightledstatus = "ON";
      display.clear();
      drawFrame2();
      display.display();
      delay(1000);
    }
  }
  else if (strcmp(topic, "/v1.6/devices/esp32/leftled/lv") == 0)
  {
    if (message == "0")
    {
      digitalWrite(leftled, LOW);
      Serial.println(topic);
      Serial.print("OFF");
      leftledstatus = "OFF";
      display.clear();
      drawFrame2();
      display.display();
      delay(1000);
    }
    else
    {
      digitalWrite(leftled, HIGH);
      Serial.println(topic);
      Serial.print("ON");
      leftledstatus = "ON";
      display.clear();
      drawFrame2();
      display.display();
      delay(1000);
    }
  }
  else if (strcmp(topic, "/v1.6/devices/esp32/poplights/lv") == 0)
  {
    if (message == "0")
    {
      digitalWrite(poplights, LOW);
      Serial.println(topic);
      Serial.print("OFF");
      poplightsstatus = "OFF";
      display.clear();
      drawFrame1();
      display.display();
      delay(1000);
    }
    else
    {
      digitalWrite(poplights, HIGH);
      Serial.println(topic);
      Serial.print("ON");
      poplightsstatus = "ON";
      display.clear();
      drawFrame1();
      display.display();
      delay(1000);
    }
  }
  else if (strcmp(topic, "/v1.6/devices/esp32/gswitch/lv") == 0)
  {
    if (message == "0")
    {
      digitalWrite(gswitch, LOW);
      Serial.println(topic);
      Serial.print("OFF");
      gswitchstatus = "OFF";
      display.clear();
      drawFrame2();
      display.display();
      delay(1000);
    }
    else
    {
      digitalWrite(gswitch, HIGH);
      Serial.println(topic);
      Serial.print("ON");
      gswitchstatus = "ON";
      display.clear();
      drawFrame2();
      display.display();
      delay(1000);
    }
  }
  Serial.write(payload, value);
  Serial.println(value);
  Serial.println();
}




/****************************************
 * Main Functions
 ****************************************/
void setup()
{
  Serial.begin(115200);
  WiFi.begin(WIFISSID, PASSWORD);
  display.init();
  display.setFont(ArialMT_Plain_10);
  display.flipScreenVertically();
  /*use mdns for host name resolution*/
  if (!MDNS.begin(host))
  { //http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1)
    {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  /*return index page which is stored in serverIndex */
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginIndex);
  });
  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart(); }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    } });
  server.begin();

  
  // Assign the pin as INPUT
  pinMode(led, OUTPUT);
  pinMode(fan, OUTPUT);
  pinMode(socket, OUTPUT);
  pinMode(rightled, OUTPUT);
  pinMode(leftled, OUTPUT);
  pinMode(poplights, OUTPUT);
  pinMode(gswitch, OUTPUT);

  Serial.println();
  Serial.print("Wait for WiFi...");
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 10, "Wait for WiFi..");
  display.display();
  delay(1000);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 10, ".....");
    display.display();
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi Connected");
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 10, "WiFi Connected");
  display.display();
  delay(1000);
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  client.setServer(mqttBroker, 1883);
  client.setCallback(callback);

  sprintf(topicSubscribeled, "/v1.6/devices/%s/%s/lv", DEVICE_LABEL, SUBSCRIBE_LED);
  sprintf(topicSubscribefan, "/v1.6/devices/%s/%s/lv", DEVICE_LABEL, SUBSCRIBE_FAN);
  sprintf(topicSubscribesocket, "/v1.6/devices/%s/%s/lv", DEVICE_LABEL, SUBSCRIBE_SOCKET);
  sprintf(topicSubscriberightled, "/v1.6/devices/%s/%s/lv", DEVICE_LABEL, SUBSCRIBE_RIGHT_LED);
  sprintf(topicSubscribeleftled, "/v1.6/devices/%s/%s/lv", DEVICE_LABEL, SUBSCRIBE_LEFT_LED);
  sprintf(topicSubscribepoplights, "/v1.6/devices/%s/%s/lv", DEVICE_LABEL, SUBSCRIBE_POP_LIGHTS);
  sprintf(topicSubscribegeneralswitch, "/v1.6/devices/%s/%s/lv", DEVICE_LABEL, SUBSCRIBE_GENERAL_SWITCH);

  client.subscribe(topicSubscribeled);
  client.subscribe(topicSubscribefan);
  client.subscribe(topicSubscribesocket);
  client.subscribe(topicSubscriberightled);
  client.subscribe(topicSubscribeleftled);
  client.subscribe(topicSubscribepoplights);
  client.subscribe(topicSubscribegeneralswitch);
  welcome_msg();
}//end of setup()


void loop()
{
  server.handleClient();
  delay(1);
  if (!client.connected())
  {
    status = 1;
    client.subscribe(topicSubscribeled);
    client.subscribe(topicSubscribefan);
    client.subscribe(topicSubscribesocket);
    client.subscribe(topicSubscriberightled);
    client.subscribe(topicSubscribeleftled);
    client.subscribe(topicSubscribepoplights);
    client.subscribe(topicSubscribegeneralswitch);
    Serial.println("Subscribing data from Ubidots Cloud");
    reconnect();
  }

  // /* 4 is mininum width, 2 is precision; float value is copied onto str_sensor*/
  // dtostrf(sensor, 4, 2, str_sensor);
  sprintf(topic, "%s%s", "/v1.6/devices/", DEVICE_LABEL);
  sprintf(payload, "%s", "");                             // Cleans the payload
  sprintf(payload, "{\"%s\":", DEVICE_STATUS);            // Adds the variable label
  sprintf(payload, "%s {\"value\": %d", payload, status); // Adds the value
  sprintf(payload, "%s } }", payload);                    // Closes the dictionary brackets                      // Closes the dictionary brackets
  client.publish(topic, payload);
  Serial.println(status);
  client.loop();

}