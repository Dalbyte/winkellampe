// WLAN WEBSOCKET

#define _WEBSOCKETS_LOGLEVEL_     3

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer_Generic.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Hash.h>


///////////////////////////
///Data Packet

#include <ArduinoJson.h>
StaticJsonDocument<300> SocketDataJSON;



///////////////////////////
///LedLib

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif
//////////////////////////////////////////////////////

///////////////////////////
////// LED

// Which Pin NeoPixels
#define LED_PIN     0

// How many NeoPixels are attached to the Arduino?
#define LED_COUNT  12 // Ledring Test
// #define LED_COUNT  144 // LedStrip

//Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGBW + NEO_KHZ800);
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Farbwerte
int Rot = 0;
int Gruen = 0;
int Blau = 0;
int Weis = 0;

// RGB vs RGBW
// uint32_t AktiveFarbe = strip.Color(0,0,0,0);
uint32_t AktiveFarbe = strip.Color(0,0,0);

//////////////////////////////////////////////////////


///////////////////////////
////// WLAN Setup

ESP8266WiFiMulti WiFiMulti;

// Static IP funktion läuft noch nicht
IPAddress static_ip(192,168,2,105);
IPAddress static_gw(192,168,2,1);
IPAddress static_sn(192,168,2,1);

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{
  switch (type)
  {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
      
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

        // send message to client
        webSocket.sendTXT(num, "Connected");
      }
      break;
      
    case WStype_TEXT:
    {
      // Serial.printf("[%u] get Text: %s\n", num, payload); // Printet Serial den Orginal empfangen Text

      // Read Data 
      deserializeJson(SocketDataJSON,(char*)payload);
      Rot = SocketDataJSON["R"];
      Gruen = SocketDataJSON["G"];
      Blau = SocketDataJSON["B"];
      Weis = SocketDataJSON["W"];

      // RGB vs RGBW
      // AktiveFarbe = strip.Color(Rot,Gruen,Blau,Weis);
      AktiveFarbe = strip.Color(Rot,Gruen,Blau);
      Serial.println(AktiveFarbe);


      // Show JSON Data GRB
      serializeJsonPretty(SocketDataJSON, Serial);
      //serializeJson(SocketDataJSON, Serial); // unformatiert
      
      
      strip.fill(AktiveFarbe,0,LED_COUNT);
      strip.show();

      
      // Simple Filter Methode

      /*
        if (payload[0] == '#')
        {
          strip.fill(rgb,0,LED_COUNT-1);
          strip.show();
        }
      */
    }break;

    default:
      break;
  }
}


///////////////////////////
////// Setup

void setup()
{ 
  Serial.begin(115200);
  
  Serial.println("\nStart ESP8266_WebSocketServer on " + String(ARDUINO_BOARD));

  for (uint8_t t = 4; t > 0; t--)
  {
    Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  // WiFi.config(static_ip, static_gw, static_sn);
  // WiFiMulti.addAP("halle", "HallegutAllesgut!");
  WiFiMulti.addAP("Beesemaise", "iGalaniapice");

  // WiFi.disconnect();
  while (WiFiMulti.run() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(100);
  }
  
  Serial.println();

  // start webSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  if (MDNS.begin("esp8266"))
  {
    Serial.println("MDNS responder started");
  }

  // handle index
  server.on("/", []()
  {
    // send index.html
    server.send(200, "text/html", "<html><head><script>var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']); connection.onopen = function () {  connection.send('Connect ' + new Date()); }; connection.onerror = function (error) {    console.log('WebSocket Error ', error);};connection.onmessage = function (e) {  console.log('Server: ', e.data);};function sendRGB() {  var r = parseInt(document.getElementById('r').value).toString(16);  var g = parseInt(document.getElementById('g').value).toString(16);  var b = parseInt(document.getElementById('b').value).toString(16);  if(r.length < 2) { r = '0' + r; }   if(g.length < 2) { g = '0' + g; }   if(b.length < 2) { b = '0' + b; }   var rgb = '#'+r+g+b;    console.log('RGB: ' + rgb); connection.send(rgb); }</script></head><body>LED Control:<br/><br/>R: <input id=\"r\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" oninput=\"sendRGB();\" /><br/>G: <input id=\"g\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" oninput=\"sendRGB();\" /><br/>B: <input id=\"b\" type=\"range\" min=\"0\" max=\"255\" step=\"1\" oninput=\"sendRGB();\" /><br/></body></html>");
  });

  server.begin();

  // Add service to MDNS
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("ws", "tcp", 81);

  // server address, port and URL
  Serial.print("WebSockets Server started @ IP address: ");
  Serial.println(WiFi.localIP());

  strip.begin();


  
}

///////////////////////////
/// Check Wifi Connection

void check_status()
{
  static unsigned long checkstatus_timeout = 0;

  //KH
#define HEARTBEAT_INTERVAL    20000L
  // Print hearbeat every HEARTBEAT_INTERVAL (20) seconds.
  if ((millis() > checkstatus_timeout) || (checkstatus_timeout == 0))
  {
    heartBeatPrint();
    checkstatus_timeout = millis() + HEARTBEAT_INTERVAL;
  }
}

void heartBeatPrint(void)
{
  static int num = 1;

  if (WiFi.status() == WL_CONNECTED)
    Serial.print("Connected to WiFi");        // H means connected to WiFi
  else
    Serial.print("Not Connected to WiFi");        // F means not connected to WiFi

  if (num == 80)
  {
    Serial.println();
    num = 1;
  }
  else if (num++ % 10 == 0)
  {
    Serial.print(" ");
  }
}

//////////////////////////////////////////////////////


///////////////////////////
///Loop

void loop()
{
  check_status();
  webSocket.loop();
  server.handleClient();
}

///////////////////////////
///JSON DATA Nest
/*
  // Add an array.
  //
  JsonArray data = doc.createNestedArray("data");
  data.add(48.756080);
  data.add(2.302038);
*/
