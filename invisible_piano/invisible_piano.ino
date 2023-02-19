#define FORCE_SENSOR_PIN A0  // the FSR and 10K pulldown are connected to A0

// define mode
#define KBD_MODE 0
#define TRB_MODE 1

//#define MODE TRB_MODE

#define TONE_TIMER_SELECTION TC_CMR_TCCLKS_TIMER_CLOCK4
#define TONE_TIMER_DIVISOR 128
#define TONE_PIN 8
#define WHOLE_NOTE_DURATION 1000

// defines pins numbers
const int trigPin = 6;
const int echoPin = 7;
const int buttonPin = 2;
const unsigned long timeout = 1000000l;  // default is 1 second (1000000 us)
int mode = 0, button_state, prev_button_state;

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

int dist_to_freq_kbd(int dist, int start_dist, int end_dist, int* freq_table) {
  //Serial.print("Distance: ");
  //Serial.println(dist);
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

int dist_to_freq_trb(int dist, int start_dist, int end_dist, int min_freq, int max_freq) {
  if (dist < start_dist || dist > end_dist) {
    return -1;
  }

  int dist_range = end_dist - start_dist;
  int freq_range = max_freq - min_freq;
  return int((float)(dist - start_dist) / float(dist_range) * freq_range + min_freq);
}

int calc_freq() {
  int dist = calc_dist();

  //Serial.print("Distance: ");
  //Serial.println(dist);

  int start_dist = 100, end_dist = 1300;

  if (mode == KBD_MODE) {
    int freq_table[12] = { 131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247 };
    return dist_to_freq_kbd(dist, start_dist, end_dist, freq_table);
  } else {  // MODE == TRB_MODE
    int min_freq = 131, max_freq = 262;
    return dist_to_freq_trb(dist, start_dist, end_dist, min_freq, max_freq);
  }
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
  pinMode(trigPin, OUTPUT);  // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);   // Sets the echoPin as an Input
  pinMode(buttonPin, INPUT);
  initialize_tone();
  Serial.begin(1200);  // Starts the serial communication
}

void initialize_tone() {
  pmc_set_writeprotect(false);
  pmc_enable_periph_clk((uint32_t)TC3_IRQn);
  TC_Configure(TC1, 0, TONE_TIMER_SELECTION | TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC);
  // TC_CMR_ACPA_SET: RA compare sets TIOA
  // TC_CMR_ACPC_CLEAR: RC compare clears TIOA
  TC1->TC_CHANNEL[0].TC_IER = TC_IER_CPCS;
  TC1->TC_CHANNEL[0].TC_IDR = ~TC_IER_CPCS;
  NVIC_EnableIRQ(TC3_IRQn);
  pinMode(TONE_PIN, OUTPUT);
}

void loop() {
  button_state = digitalRead(buttonPin);
  //mode = (buttonstate == HIGH) ? KBD_MODE : TRB_MODE;
  if (button_state == LOW && prev_button_state == HIGH) {
    mode = 1 - mode;
  }
  prev_button_state = button_state;

  int base_freq = calc_freq();
  int octave = force2octave();
  //Serial.print("Octave: ");
  //Serial.println(octave);
  //Serial.print(force2octave());
  int final_freq = base_freq << octave;
  // Serial.print("Final freq: ");
  // Serial.println(final_freq);
  tone(final_freq);
  delay(100);
}

void tone(uint32_t frequency) {
  TC_Stop(TC1, 0);
  if (frequency == 0) {
    digitalWrite(TONE_PIN, LOW);
    return;
  }
  TC_SetRC(TC1, 0, VARIANT_MCK / (TONE_TIMER_DIVISOR * 2) / frequency);
  TC_Start(TC1, 0);
}

void TC3_Handler() {
  static uint32_t pin_state = 0;
  TC_GetStatus(TC1, 0);
  pin_state = !pin_state;
  digitalWrite(TONE_PIN, pin_state);
}