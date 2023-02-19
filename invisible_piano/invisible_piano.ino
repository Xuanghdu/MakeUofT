////////////////////////////////////////////////////////////////////////////////
// Tone
////////////////////////////////////////////////////////////////////////////////

#define TONE_HARMONY_CHANNELS 1

const uint32_t toneClockSelection = TC_CMR_TCCLKS_TIMER_CLOCK4;
const uint32_t toneClockDivisor = 128;
const uint32_t toneOutputPin = DAC1;

void setHarmonyChannel(uint32_t channel, uint32_t value) {
  static volatile uint32_t channelValues[TONE_HARMONY_CHANNELS] = {};
  channelValues[channel] = value;
  uint32_t valueSum = 0;
  for (uint32_t i = 0; i < TONE_HARMONY_CHANNELS; ++i) {
    valueSum += channelValues[i];
  }
  analogWrite(toneOutputPin, min(valueSum, 4095));
}

static volatile uint32_t harmonyChannelAmplitudes[TONE_HARMONY_CHANNELS] = {};

class Tone {
public:
  void init(uint32_t clockID, Tc* tcPtr, uint32_t tcChannel, uint32_t harmonyChannel) {
    pmc_enable_periph_clk(clockID);
    TC_Configure(tcPtr, tcChannel, toneClockSelection | TC_CMR_WAVE | TC_CMR_WAVSEL_UP_RC);
    tcPtr->TC_CHANNEL[tcChannel].TC_IER = TC_IER_CPCS;
    tcPtr->TC_CHANNEL[tcChannel].TC_IDR = ~TC_IER_CPCS;
    NVIC_EnableIRQ((IRQn_Type)clockID);
    tcPtr_ = tcPtr;
    tcChannel_ = tcChannel;
    harmonyChannel_ = harmonyChannel;
  }

  void set(uint32_t frequency, uint32_t amplitude) {
    TC_Stop(tcPtr_, tcChannel_);
    if (frequency == 0 || amplitude == 0) {
      setHarmonyChannel(harmonyChannel_, 0);
      return;
    }
    harmonyChannelAmplitudes[harmonyChannel_] = amplitude;
    TC_SetRC(tcPtr_, tcChannel_, VARIANT_MCK / toneClockDivisor / frequency / 2);
    TC_Start(tcPtr_, tcChannel_);
  }

private:
  Tc* tcPtr_;
  uint32_t tcChannel_;
  uint32_t harmonyChannel_;
};

#define DEFINE_TC_HANDLER(handlerName, tcPtr, toChannel, harmonyChannel) \
  extern void handlerName(); \
  void handlerName() { \
    static uint32_t pinState = 0; \
    TC_GetStatus(tcPtr, toChannel); \
    pinState = 1 - pinState; \
    setHarmonyChannel(harmonyChannel, pinState* harmonyChannelAmplitudes[harmonyChannel]); \
  }

DEFINE_TC_HANDLER(TC3_Handler, TC1, 0, 0)

static Tone harmonyTones[TONE_HARMONY_CHANNELS] = {};

void initializeTones() {
  pmc_set_writeprotect(false);
  analogWriteResolution(12);
  harmonyTones[0].init(ID_TC3, TC1, 0, 0);
}

////////////////////////////////////////////////////////////////////////////////
// Sensor
////////////////////////////////////////////////////////////////////////////////

// defines pins numbers
const uint32_t modeButtonPin = 2;
const uint32_t forceSensorPin = A0;  // the FSR and 10K pulldown are connected to A0
const uint32_t ultrasonicTrigPin = 6;
const uint32_t ultrasonicEchoPin = 7;
const uint32_t ultrasonicTimeout = 100000;  // default is 1 second (1000000 us)
const uint32_t minSoundDistance = 100;
const uint32_t maxSoundDistance = 1300;
const uint32_t baseNoteFrequencies[] = { 523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988 };
// Backup base frequencies: { 131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247 }
//                          { 262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494 }

#define NOTE_C5 523
#define NOTE_CS5 554
#define NOTE_D5 587
#define NOTE_DS5 622
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_FS5 740
#define NOTE_G5 784
#define NOTE_GS5 831
#define NOTE_A5 880
#define NOTE_AS5 932
#define NOTE_B5 988

enum SoundMode {
  DISCRETE_MODE = 0,
  CONTINUOUS_MODE = 1
} soundMode = DISCRETE_MODE;

// https://arduinogetstarted.com/tutorials/arduino-force-sensor
uint32_t getOctaveFromForce() {
  uint32_t forceReading = analogRead(forceSensorPin);
  // Serial.print("Force reading: ");
  // Serial.println(forceReading);
  if (forceReading < 24) {
    return 0;
  } else if (forceReading > 1000) {
    return 2;
  } else {
    return 1;
  }
}

uint32_t getBaseFrequencyFromDistance() {
  uint32_t distance = detectDistance();
  // Serial.print("Distance: ");
  // Serial.println(distance);
  switch (soundMode) {
    case DISCRETE_MODE:
      return distanceToBaseFrequencyDiscrete(distance);
    case CONTINUOUS_MODE:
      return distanceToBaseFrequencyContinuous(distance);
    default:
      return 0;
  }
}

uint32_t detectDistance() {
  // Clears the ultrasonicTrigPin
  digitalWrite(ultrasonicTrigPin, LOW);
  delayMicroseconds(2);
  // Sets the ultrasonicTrigPin on HIGH state for 10 microseconds
  digitalWrite(ultrasonicTrigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(ultrasonicTrigPin, LOW);
  // Reads the ultrasonicEchoPin, returns the sound wave travel time in microseconds
  // If no pulse is received before the timeout, returns 0
  uint32_t duration = pulseIn(ultrasonicEchoPin, HIGH, ultrasonicTimeout);
  // Calculating the distance in centimeters
  return (uint32_t)(duration * (0.34 / 2));
}

uint32_t distanceToBaseFrequencyDiscrete(uint32_t distance) {
  // Serial.print("Distance (discrete): ");
  // Serial.println(distance);
  if (distance < minSoundDistance || distance > maxSoundDistance) {
    return 0;
  }
  uint32_t noteIndex = 12 * (distance - minSoundDistance) / (maxSoundDistance - minSoundDistance);
  noteIndex = min(noteIndex, 12 - 1);
  return baseNoteFrequencies[noteIndex];
}

uint32_t distanceToBaseFrequencyContinuous(uint32_t distance) {
  // Serial.print("Distance (continuous): ");
  // Serial.println(distance);
  if (distance < minSoundDistance || distance > maxSoundDistance) {
    return 0;
  }
  uint32_t offsetFrequency = baseNoteFrequencies[0] * (distance - minSoundDistance) / (maxSoundDistance - minSoundDistance);
  offsetFrequency = min(offsetFrequency, baseNoteFrequencies[0] - 1);
  return baseNoteFrequencies[0] + offsetFrequency;
}

////////////////////////////////////////////////////////////////////////////////
// Main
////////////////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(1200);  // Starts the serial communication
  initializeTones();
  pinMode(modeButtonPin, INPUT);
  pinMode(ultrasonicTrigPin, OUTPUT);  // Sets the ultrasonicTrigPin as an output
  pinMode(ultrasonicEchoPin, INPUT);  // Sets the ultrasonicEchoPin as an input
}

void loop() {
  static uint32_t previousButtonState = LOW;
  uint32_t buttonState = digitalRead(modeButtonPin);
  if (previousButtonState == HIGH && buttonState == LOW) {
    soundMode = (SoundMode)(1 - soundMode);
  }
  previousButtonState = buttonState;
  uint32_t baseFrequency = getBaseFrequencyFromDistance();
  uint32_t octave = getOctaveFromForce();
  // Serial.print("Octave: ");
  // Serial.println(octave);
  // Serial.print(getOctaveFromForce());
  uint32_t finalFrequency = baseFrequency << octave;
  // Serial.print("Final frequency: ");
  // Serial.println(finalFrequency);
  harmonyTones[0].set(finalFrequency, 4096 - 1);
  delay(100);
}
