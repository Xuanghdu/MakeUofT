#define FORCE_SENSOR_PIN A0 // the FSR and 10K pulldown are connected to A0

// define mode
#define KBD_MODE 0
#define TRB_MODE 1

#define MODE TRB_MODE 

// defines pins numbers
const int trigPin = 2;
const int echoPin = 3;
const unsigned long timeout = 1000000l; // default is 1 second (1000000 us)

int calc_dist() {
  int duration, distance;
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  // If no pulse is received before the timeout, returns 0
  duration = pulseIn(echoPin, HIGH, timeout);
  // Calculating the distance in cm
  distance = duration * 0.34 / 2;
  return distance;
}

#if MODE==KBD_MODE
int dist_to_freq(int dist, int start_dist, int end_dist, int* freq_table) {
  if (dist < start_dist || dist > end_dist) {
    return -1;
  }

  // calculate bucket size and bucket number based on start and end distance
  int bucket_size = (end_dist - start_dist) / 12;
  int bucket_num = (dist - start_dist) / bucket_size;

  // prevent rounding errors from division causing array to go out of bound
  bucket_num = min(bucket_num, 11);
  return freq_table[bucket_num];
}
#elif MODE==TRB_MODE
int dist_to_freq(int dist, int start_dist, int end_dist, int min_freq, int max_freq) {
  if (dist < start_dist || dist > end_dist) {
    return -1;
  }

  int dist_range = end_dist - start_dist;
  int freq_range = max_freq - min_freq;
  return (dist - start_dist) / dist_range * freq_range + min_freq;
}
#endif

int calc_freq() {
  int dist = calc_dist();
  
  //Serial.print("Distance: ");
  //Serial.println(dist);

  int start_dist = 10, end_dist = 130;

  #if MODE==KBD_MODE
  int freq_table[12] = {131,139,147,156,165,175,185,196,208,220,233,247};
  return dist_to_freq(dist, start_dist, end_dist, freq_table);
  #elif MODE==TRB_MODE
  int min_freq = 131, max_freq = 262;
  return dist_to_freq(dist, start_dist, end_dist, min_freq, max_freq);
  #endif
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

void setup() {
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  Serial.begin(1200); // Starts the serial communication
}

void loop() {
  int base_freq = calc_freq();
  int octave = force2octave();
  //Serial.print(force2octave());
  int final_freq = base_freq << octave;
  Serial.print("Final freq: ");
  Serial.print(final_freq);

  delay(200);
}