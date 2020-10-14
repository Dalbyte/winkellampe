////////////////////////////////////////////////////////////////////////////////
// MPU6050 Sensor Libs

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;



////////////////////////////////////////////////////////////////////////////////
// Variablen

  //////////////////////
  // MPU6050 Sensor Variablen

  float gyroXoffset, gyroYoffset, gyroZoffset; // Um Sensor Positionierung im Gehäuse auszugleichen
  float tempS, accX, accY, accZ, gyroX, gyroY, gyroZ; // Sensor Output
  float angleAccX, angleAccY; // Winkle Beschleunigung
  float angleX, angleY, angleZ; // Winkle (Komplementäre Filter)
  long preInterval; // Zeit
  float accCoef, gyroCoef; // Alpha zwischen HighPass and LowPass

  #define RAD_2_DEG          57.29578 // [°/rad]

  // Alpha für Komplementäre Filter
  #define DEFAULT_GYRO_COEFF 0.98 // Alpha


////////////////////////////////////////////////////////////////////////////////
// Setup


void setup(void) {
  Serial.begin(115200); // Serial Kommunikation starten
  while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens

  MPU6050SensorSetup(); 

  Serial.println("");
  delay(100);
}


////////////////////////////////////////////////////////////////////////////////
// Loop


void loop() {

MPU6050SensorUpdate2();
MPU6050SensorPrint();


}

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
