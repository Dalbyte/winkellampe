/*////////////////////////////////////////////////////////////////////////////////
,   . , .  . ,  , ,--. ,    ,     ,.  .   , ;-.  ,--. 
| . | | |\ | | /  |    |    |    /  \ |\ /| |  ) |    
| ) ) | | \| |<   |-   |    |    |--| | V | |-'  |-   
|/|/  | |  | | \  |    |    |    |  | |   | |    |    
' '   ' '  ' '  ` `--' `--' `--' '  ' '   ' '    `--'                                                       
*/////////////////////////////////////////////////////////////////////////////////
/*
* Design:     Wanhyun Ko
* Developing: Alexander Dalbert
*
* Made in Halle(Saale) 2020
*/


/*////////////////////////////////////////////////////////////////////////////////
   __   _ __                    
  / /  (_) /  _______ _______ __
 / /__/ / _ \/ __/ _ `/ __/ // /
/____/_/_.__/_/  \_,_/_/  \_, / 
                         /___/  
*/////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// LIB > WLAN WEBSOCKET

#define _WEBSOCKETS_LOGLEVEL_     3

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsServer_Generic.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Hash.h>


/*
* Wlan Conection und Websocket sind die Libs gut.
* ESPAsyncWebServer.h könnte man sich noch als Alternative Anschauen
*/

// WLAN > Variablen

      ESP8266WiFiMulti WiFiMulti;

      // Static IP funktion läuft noch nicht
      IPAddress static_ip(192,168,2,105);
      IPAddress static_gw(192,168,2,1);
      IPAddress static_sn(192,168,2,1);

      ESP8266WebServer server(80);
      WebSocketsServer webSocket = WebSocketsServer(81);

      // Alex Wlan
      const char* ssid = "Beesemaise";  // Enter SSID here
      const char* password = "iGalaniapice";  //Enter Password here

      // Wanhyun Wlan
      // const char* ssid = "halle";  // Enter SSID here
      // const char* password = "HallegutAllesgut!";  //Enter Password here

////////////////////////////////////////////////////////////////////////////////
// LIB > NeoPixel

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif

/*
* LGPL-3.0 License
* https://github.com/adafruit/Adafruit_NeoPixel
* 
*/

// NeoPixel > Variablen

      #define LED_PIN     0   // Which Pin NeoPixels
      #define LED_COUNT  12   // How many NeoPixels are attached to the Arduino?

      //Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGBW + NEO_KHZ800);
      Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

      // Farbwerte
      int Rot = 10;
      int Gruen = 80;
      int Blau = 50;
      int Weis = 100;

      // RGB vs RGBW
      //float ColorStepMap[4] = {0,0,0,0};
      float ColorStepMap[3] = {0,0,0};

      // RGB vs RGBW
      // uint32_t AktiveFarbe = strip.Color(0,0,0,0);
      uint32_t AktiveFarbe = strip.Color(0,0,0);




////////////////////////////////////////////////////////////////////////////////
// LIB > JSON

#include <ArduinoJson.h>
/*
* MIT License
* https://arduinojson.org/
* https://github.com/bblanchon/ArduinoJson
*
* Websocket gibt einen uint8_t Paket als Input. In den Paket befindet sich JSON formatiert Daten die durch die Lib entpackt werden.
* Variablen werden durch die Lib auch wieder als JSON verpackt.
*
*/

// JSON > Variablen
      StaticJsonDocument<300> SocketDataJSON;



////////////////////////////////////////////////////////////////////////////////
// LIB > MPU6050

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

/*
* BSD License
* https://github.com/adafruit/Adafruit_MPU6050
*
*/

Adafruit_MPU6050 mpu;


// MPU6050 > Variablen

      float gyroXoffset, gyroYoffset, gyroZoffset; // Um Sensor Positionierung im Gehäuse auszugleichen
      float tempS, accX, accY, accZ, gyroX, gyroY, gyroZ; // Sensor Output
      float angleAccX, angleAccY; // Winkle Beschleunigung
      float angleX, angleY, angleZ; // Winkle (Komplementäre Filter)
      long preInterval; // Zeit
      float accCoef, gyroCoef; // Alpha zwischen HighPass and LowPass

      #define RAD_2_DEG          57.29578 // [°/rad]

      // Alpha für Komplementäre Filter
      #define DEFAULT_GYRO_COEFF 0.98 // Alpha

      // MPU Sensor Smoothing

      float AverageX = 0;
      float AverageY = 0;
      float AverageZ = 0;
      int AverageRange = 10; // Zeitspanne
      long timer = 0;
      int WinkelInputLatenz = 10; //in ms

      int WinkelMax = 90; // WinkelMax ist die Schrägste zulässige Position

////////////////////////////////////////////////////////////////////////////////
// WLAN Setup



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

      ws_rx(payload);
       
    }break;

    default:
      break;
  }
}


/*////////////////////////////////////////////////////////////////////////////////
   ____    __          
  / __/__ / /___ _____ 
 _\ \/ -_) __/ // / _ \
/___/\__/\__/\_,_/ .__/
                /_/  
*/////////////////////////////////////////////////////////////////////////////////

void setup()
{ 
  Serial.begin(115200); // SERIELLE KOMMUNIKATION gestartet
  Serial.println("\nStart Winkellampe " + String(ARDUINO_BOARD));

  ///////////////////////////////////
  // Setup > Bootdelay
      for (uint8_t t = 4; t > 0; t--)
      {
        Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
        Serial.flush();
        delay(1000);
      }

  ///////////////////////////////////
  // Setup > Websetup

      // WiFi.config(static_ip, static_gw, static_sn);
      WiFiMulti.addAP(ssid, password);

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

  ///////////////////////////////////
  // Setup > Webrequest

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



  ///////////////////////////////////
  // Setup > NeoPixel
  
      strip.begin();
  
  ///////////////////////////////////
  // Setup > MPU6050
  
  MPU6050SensorSetup();
  Serial.println("");
  delay(100);
  
}

/*////////////////////////////////////////////////////////////////////////////////
   __                
  / /  ___  ___  ___ 
 / /__/ _ \/ _ \/ _ \
/____/\___/\___/ .__/
              /_/   
*/////////////////////////////////////////////////////////////////////////////////

void loop()
{
  check_status();
  webSocket.loop();
  server.handleClient();

  ///////////////////////////////////
  // Setup > MPU6050
  MPU6050SensorUpdate2();
  // MPU6050SensorPrint();
  winklelampe();
}



////////////////////////////////////////////////////////////////////////////////
// Check Wifi Connection

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
  /*
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
  */
}

////////////////////////////////////////////////////////////////////////////////
// JSON DATA Nest
/*
  // Add an array.
  //
  JsonArray data = doc.createNestedArray("data");
  data.add(48.756080);
  data.add(2.302038);
*/

////////////////////////////////////////////////////////////////////////////////
// Websocket Empfangen

void ws_rx(uint8_t * payload){

  // Read Data
  deserializeJson(SocketDataJSON, (char *)payload);
  Rot = SocketDataJSON["R"];
  Gruen = SocketDataJSON["G"];
  Blau = SocketDataJSON["B"];
  Weis = SocketDataJSON["W"];

  // RGB vs RGBW
  // AktiveFarbe = strip.Color(Rot,Gruen,Blau,Weis);
  AktiveFarbe = strip.Color(Rot, Gruen, Blau);

  // Serial.println(AktiveFarbe);

  // Show JSON Data GRB
  //serializeJsonPretty(SocketDataJSON, Serial);
  //serializeJson(SocketDataJSON, Serial); // unformatiert

  strip.fill(AktiveFarbe, 0, LED_COUNT);
  strip.show();
}


/*////////////////////////////////////////////////////////////////////////////////
    __  _______  __  _______ ____  __________ 
   /  |/  / __ \/ / / / ___// __ \/ ____/ __ \
  / /|_/ / /_/ / / / / __ \/ / / /___ \/ / / /
 / /  / / ____/ /_/ / /_/ / /_/ /___/ / /_/ / 
/_/  /_/_/    \____/\____/\____/_____/\____/  
                                              
*/////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// MPU6050Sensor Funktion


void MPU6050SensorAlpha(){ // Setzt Alpha
  accCoef = 1.0-DEFAULT_GYRO_COEFF;
  gyroCoef = DEFAULT_GYRO_COEFF;
}


void MPU6050SensorUpdate(){

// MPU6050SensorUpdate() Basiert auf [MPU6050_light](https://github.com/rfetick/MPU6050_light) *License:MIT*

  // Sensor Event
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  // Serial.println("MPU6050 Sensor EventUpdate: Run");


  // Sensor Data hollen
  accX = a.acceleration.x;
  accY = a.acceleration.y;
  accZ = a.acceleration.z;
  tempS = temp.temperature;
  gyroX = g.gyro.x; // - gyroXoffset;
  gyroY = g.gyro.y; // - gyroYoffset;
  gyroZ = g.gyro.z; // - gyroZoffset;
  

  //////////////////////
  // Sensor Komplementäre Filter

  float sgZ = (accZ>=0)-(accZ<0);
  angleAccX =   atan2(accY, sgZ*sqrt(accZ*accZ + accX*accX)) * RAD_2_DEG;
  angleAccY = - atan2(accX, sqrt(accZ*accZ + accY*accY)) * RAD_2_DEG;

  unsigned long Tnew = millis();
  float dt = (Tnew - preInterval) * 1e-3;
  preInterval = Tnew;

  angleX = (gyroCoef * (angleX + gyroX*dt)) + (accCoef * angleAccX);
  angleY = (gyroCoef * (angleY + gyroY*dt)) + (accCoef * angleAccY);
  angleZ += gyroZ*dt;

}

void MPU6050SensorUpdate2(){

  // Sensor Event
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  // Serial.println("MPU6050 Sensor EventUpdate: Run");

  // Sensor Data hollen
  accX = a.acceleration.x;
  accY = a.acceleration.y;
  accZ = a.acceleration.z;
  tempS = temp.temperature;
  gyroX = g.gyro.x; // - gyroXoffset;
  gyroY = g.gyro.y; // - gyroYoffset;
  gyroZ = g.gyro.z; // - gyroZoffset;

  //////////////////////
  // Sensor Komplementäre Filter

  angleAccX = atan( -1  * (accX)  /sqrt(pow(accY,2) + pow(accZ,2))) *  RAD_2_DEG;
  angleAccY = atan(       (accY)  /sqrt(pow(accX,2) + pow(accZ,2)))  * RAD_2_DEG;

  unsigned long Tnew = millis();
  float dt = (Tnew - preInterval) * 1e-3; // / 1000.0;
  preInterval = Tnew;

  angleX = gyroCoef *(angleX+gyroX*dt) + accCoef*angleAccX;
  angleY = gyroCoef *(angleY+gyroY*dt) + accCoef*angleAccY;
  angleZ = angleZ+gyroZ*dt* RAD_2_DEG;

}

void MPU6050SensorSetup(){ // MPU6050 Sensor Setup: Start

  Serial.println("MPU6050 Sensor Setup: Start");

  // Startet MPU6050 Sensor
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  //////////////////////
  // Sensor Einstellung

  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  MPU6050SensorGetAccRange(); // Feedback Serial AccRange

  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  MPU6050SensorGetGyroRange(); // Feedback Serial GyroRange

  mpu.setFilterBandwidth(MPU6050_BAND_44_HZ);
  MPU6050SensorGetFilterBandwidth(); // Feedback Serial FilterBandwidth

  MPU6050SensorAlpha();

  preInterval = millis(); // may cause issue if begin() is much before the first update()
}


void MPU6050SensorGetAccRange(){ 
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }
}

void MPU6050SensorGetGyroRange(){
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println("+- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println("+- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println("+- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println("+- 2000 deg/s");
    break;
  }
}

void MPU6050SensorGetFilterBandwidth(){
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }
}

void MPU6050SensorReading(){
  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  /* Print out the values */
  Serial.print("Acceleration X: ");
  Serial.print(a.acceleration.x);
  Serial.print(", Y: ");
  Serial.print(a.acceleration.y);
  Serial.print(", Z: ");
  Serial.print(a.acceleration.z);
  Serial.println(" m/s^2");

  Serial.print("Rotation X: ");
  Serial.print(g.gyro.x);
  Serial.print(", Y: ");
  Serial.print(g.gyro.y);
  Serial.print(", Z: ");
  Serial.print(g.gyro.z);
  Serial.println(" rad/s");

  Serial.print("Temperature: ");
  Serial.print(temp.temperature);
  Serial.println(" degC");

  Serial.println("");
  delay(500);
}

//////////////////////
// Plotter Sensor Data

void MPU6050SensorPrint(){
  Serial.print("90");
  Serial.print(",");
  Serial.print("180");
  Serial.print(",");
  Serial.print(angleX);
  Serial.print(",");
  Serial.print(angleY);
  Serial.print(",");
  Serial.print(angleZ);
  Serial.print(",");
  Serial.print("-90");
  Serial.print(",");
  Serial.print("-180");
  Serial.println("");
}

////////////////////////////////////////////////////////////////////////////////
// Winkel to Fade

/*
* Smoothing soll Position stabelesieren beim nach Schwingen.
* (Int und Float Problem)
*/

float SmoothingInput(float input, float AverageValue){
    float SmoothingOut = (( (float) AverageRange * (float) AverageValue) + (float) input)/( (float) AverageRange+1);
    return SmoothingOut;
}

void MPU6050_Glatt(){
    if((millis()-timer)>WinkelInputLatenz){ // print data every ms
	
    //////////////////////
    // X Rotation
        Serial.print("X : ");
        AverageX = SmoothingInput(angleX,AverageX);
        //Serial.print(angleX);
        //Serial.print("X Smoothing :"AverageX);
	
    //////////////////////
    // Y Rotation
        Serial.print("\tY : ");
        AverageX = SmoothingInput(angleY,AverageY);
        // Serial.print(angleY);
        // Serial.print("Y Smoothing :"AverageY);

    //////////////////////
    // Z Rotation
        Serial.print("\tZ : ");
        AverageX = SmoothingInput(angleZ,AverageZ);
        // Serial.println(angleZ);
        // Serial.print("Z Smoothing :"AverageZ);

	  timer = millis();  
  }
}


////////////////////////////////////////////////////////////////////////////////
// Winkel Lampe Kippen

float ColorMap(int offset,int channelluma){

    // int offset = 1 = 100%
    // int channelluma = den Aktuellen Wert

    // Nützliche Funktionen
    // map(value, fromLow, fromHigh, toLow, toHigh)
    // sensVal = constrain(sensVal, 10, 150);  // limits range of sensor values to between 10 and 150


    
    int WinkelLuma = channelluma - (offset*channelluma);


}




float AngularMap(int LumaMax,float angle){
  float StepLuma = (float) LumaMax / (float) LED_COUNT;
  float w;

  if(angle < 0){
    w = angle * (float) -1;
  }else
  {
    w = angle;
  }
  //AngularMapPrint(w);

  return map(w, 0, WinkelMax,StepLuma,0);
}

void AngularMapPrint(float w){
  
  Serial.print("Winkle:");
  Serial.print(w);
  //Serial.print(",");
  Serial.print(",");
  Serial.print("-90");
  Serial.print(",");
  Serial.print("90");
  Serial.println("");
}

void setColorSteps(){
  ColorStepMap[0] = AngularMap(Rot,angleX);
  ColorStepMap[1] = AngularMap(Gruen,angleX);
  ColorStepMap[2] = AngularMap(Blau,angleX);
  //ColorStepMap[3] = AngularMap(Weis,angleX);
}

void setColorStepsPrint(){
  /*
  Serial.print("90");
  Serial.print(",");
  Serial.print(ColorStepMap[0]);
  Serial.print(",");
  */
  Serial.print(ColorStepMap[1]);
  Serial.print(",");
  /*
  Serial.print(ColorStepMap[2]);
  Serial.print(",");
  Serial.print(angleX);
  Serial.print(",");
  Serial.print("-90");
  */
  Serial.println("");
}

int run = 3;
int r = 0;

void winklelampe(){

  if(r <= run){
    //r++;
  setColorSteps();
  setColorStepsPrint();

  for(int i=0;i<LED_COUNT;i++){
      float PixelColorStep[0];

      // RGB a=3 vs RGBW a=4
      for (int a = 0; a < 3; a++){
        PixelColorStep[a] = ColorStepMap[a]*i;
      }
      //strip.setPixelColor(i, Rot-PixelColorStep[0], Gruen-PixelColorStep[1], Blau-PixelColorStep[2], Weis-PixelColorStep[3]);
      strip.setPixelColor(i,strip.Color(Rot-PixelColorStep[0],Gruen-PixelColorStep[1],Blau-PixelColorStep[2]));
      PixelFarbePrint((int)Rot-(int)PixelColorStep[0],(int)Gruen-(int)PixelColorStep[1],(int)Blau-(int)PixelColorStep[2]);
    }
    strip.show();
  }
    
}

void PixelFarbePrint(float a,float b,float c){
        /*
        Serial.print("RotGrenze:");
        Serial.print(10);
        Serial.print(",");

        Serial.print("Rot:");
        Serial.print(a);
        Serial.print(",");
        
        Serial.print("GrünGrenze:");
        Serial.print(80);
        Serial.print(",");
        */
        Serial.print("Grün:");
        Serial.print(b);
        /*
        Serial.print(",");
        Serial.print("BlauGrenze:");
        Serial.print(50);
        Serial.print(",");
        Serial.print("Blau:");
        Serial.print(c);
        */
        Serial.println("");
}

/*
      // Farbwerte
      int Rot = 10;
      int Gruen = 80;
      int Blau = 50;
      int Weis = 100;
*/


// send message to client
//webSocket.sendTXT(num, "Connected");
