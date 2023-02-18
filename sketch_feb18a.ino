#define FORCE_SENSOR_PIN A0 // the FSR and 10K pulldown are connected to A0

void setup() {
  Serial.begin(1200);
}

// https://arduinogetstarted.com/tutorials/arduino-force-sensor
int force2octave() {
  int analogReading = analogRead(FORCE_SENSOR_PIN);
  // Serial.print(analogReading); // print the raw analog reading
  if (analogReading < 24)       
    return 0;
  else if (analogReading > 1000) 
    return 2;
  else
    return 1;
}

void loop() {
  delay(200);
}