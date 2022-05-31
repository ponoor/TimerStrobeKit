#include <Arduino.h>

const uint8_t knobPin[3] = {A3, A2, A1}; // ON/OFF/PWM
#define MODE_SW_PIN 5u
#define TEST_SW_PIN 3u
#define MOSFET_PIN 9u
bool outState = false;

bool isTimerMode = true;

#define SW_POLL_PERIOD 20         // ms
#define SERIAL_REPORT_PERIOD 1000 // ms

// ============== Timer Mode ==============
uint32_t onDuration = 0, offDuration = 0; // ms
uint8_t pwmVal = 0;
const uint32_t
    onDurationRange[2] = {20, 1000},
    offDurationRange[2] = {500, 20000}; // {min, max} in [ms]

void setup_timer()
{
  cli();  // disable interrupts
  // Put timer 1 in 8-bit phase correct pwm mode
  bitSet(TCCR1A, WGM10);
  bitClear(TCCR1A, WGM11);
  bitClear(TCCR1B, WGM12);
  bitClear(TCCR1B, WGM13);

  // Set PWM frequency 31kHz
  bitClear(TCCR1B, CS12);
  bitClear(TCCR1B, CS11);
  bitSet(TCCR1B, CS10);

  // Disable overflow interrupt
  bitClear(TIMSK1, TOIE1);

  sei();  // enable interrupts
}

void updateKnob_timer()
{
  onDuration = map(analogRead(knobPin[0]), 0, 1023, onDurationRange[0], onDurationRange[1]);
  offDuration = map(analogRead(knobPin[1]), 0, 1023, offDurationRange[0], offDurationRange[1]);
  pwmVal = ((uint16_t)analogRead(knobPin[2])) >> 2;
  if (outState)
    analogWrite(MOSFET_PIN, pwmVal);
}

void loop_timer(uint32_t currentTime)
{
  static uint32_t trigStartTime = 0;
  if (outState)
  {
    if ((uint32_t)(currentTime - trigStartTime) >= onDuration)
    {
      analogWrite(MOSFET_PIN, 0);
      outState = false;
      trigStartTime = currentTime;
    }
  }
  else
  {
    if ((uint32_t)(currentTime - trigStartTime) >= offDuration)
    {
      analogWrite(MOSFET_PIN, pwmVal);
      outState = true;
      trigStartTime = currentTime;
    }
  }
}

// ============== Strobe Mode ==============
uint16_t 
  strobePeriod = 0,
  strobePulseLength = 0;

ISR( TIMER1_OVF_vect )
{
  ICR1 = strobePeriod;
  OCR1A = strobePulseLength;
}
void setup_strobe()
{
  cli();  // disable interrupts
  // Put timer1 in 16-bit fast pwm mode
  bitClear(TCCR1A, WGM10);
  bitSet(TCCR1A, WGM11);
  bitSet(TCCR1B, WGM12);
  bitSet(TCCR1B, WGM13);

  bitClear(TCCR1A, COM1A0);
  bitSet(TCCR1A, COM1A1);


  // Pre scaler
  bitClear(TCCR1B, CS12);
  bitSet(TCCR1B, CS11);
  bitSet(TCCR1B, CS10);

  // Enable overflow interrupt
  bitSet(TIMSK1, TOIE1);

  sei();  // enable interrupts
  
}

void updateKnob_strobe()
{
  // strobePeriod = (uint16_t)analogRead(knobPin[2]) << 5;
  float t = (float)analogRead(knobPin[2]) / 1023.0f;
  strobePeriod = powf(t, 2.7f) * 65535.0f;
  if (strobePeriod < 5) strobePeriod = 5;

  // strobePulseLength = (float)(analogRead(knobPin[0])/1023.0f) * (float)strobePeriod;
  t = (float)analogRead(knobPin[0]) / 1023.0f;
  strobePulseLength = powf(t, 2.7f) * (float)strobePeriod;
  
  // uint16_t offOffset = analogRead(knobPin[1]) << 4;
  t = (float)analogRead(knobPin[1]) / 1023.0f;
  uint16_t offOffset = powf(t, 2.7f) * 19000.0f;
  strobePeriod += offOffset;
}

void loop_strobe()
{
}

// ============== Common ==============
void check_mode_sw()
{
  bool modeSwState = digitalRead(MODE_SW_PIN);
  if (isTimerMode != modeSwState)
  {
    isTimerMode = !isTimerMode;
    if (isTimerMode)
    {
      setup_timer();
    }
    else
    {
      setup_strobe();
    }
  }
}

void serialReport()
{
  if (isTimerMode)
  {
    Serial.print(F("On Duration: "));
    Serial.print(onDuration);
    Serial.print(F(",\tOff Duration: "));
    Serial.print(offDuration);
    Serial.print(",\tDimming value:");
    Serial.print(pwmVal);
    Serial.println();
  }
  else{
    Serial.print(F("Strobe Period: "));
    Serial.print(strobePeriod);
    Serial.print(F(",\tPulse Length: "));
    Serial.print(strobePulseLength);
    Serial.println();
  }
}
// ============== setup ==============
void setup()
{
  pinMode(MODE_SW_PIN, INPUT_PULLUP);
  pinMode(TEST_SW_PIN, INPUT_PULLUP);
  pinMode(MOSFET_PIN, OUTPUT);

  setup_timer();
  Serial.begin(57600);
}

// ============== loop ==============
void loop()
{
  uint32_t currentTime = millis();
  static uint32_t lastSwPollTime = 0, lastSerialReportTime = 0;

  if ((uint32_t)(currentTime - lastSwPollTime) >= SW_POLL_PERIOD)
  {
    check_mode_sw();
    if (isTimerMode)
    {
      updateKnob_timer();
    }
    else
    {
      updateKnob_strobe();
    }
    lastSwPollTime = currentTime;
  }

  if (isTimerMode)
  {
    loop_timer(currentTime);
  }
  else
  {
    loop_strobe();
  }

  if ((uint32_t)(currentTime - lastSerialReportTime) >= SERIAL_REPORT_PERIOD)
  {
    serialReport();
    lastSerialReportTime = currentTime;
  }
}