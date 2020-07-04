/*
 Name:		Timer.ino
 Created:	2020/06/21 13:52:41
 Target: Arduino UNO
 Author:	kanta
*/

#define ON_DURATION_POD_PIN	A3
#define OFF_DURATION_POD_PIN	A2
#define	PWM_POD_PIN	A1
#define MODE_SW_PIN	5u
#define TEST_SW_PIN	3u
#define MOSFET_PIN	9u

boolean isLongPeriodMode = false;
boolean isTestMode = false;
uint32_t onDuration, offDuration; // ms
uint32_t lastSerialReportTime = 0, lastPodUpdateTime = 0, lastTrigTime = 0;
boolean outState = LOW;
uint8_t pwmVal = 0;

const uint32_t
onDurationMin[2] = { 20, 1000 }, // short mode, long mode.
onDurationMax[2] = { 2000, 60000 },
offDurationMin[2] = { 100, 5000 },
offDurationMax[2] = { 60000, 180000 };

void setup() {
	pinMode(MODE_SW_PIN, INPUT_PULLUP);
	pinMode(TEST_SW_PIN, INPUT_PULLUP);
	pinMode(MOSFET_PIN, OUTPUT);
	bitClear(TCCR1B, CS12);
	bitClear(TCCR1B, CS11);
	bitSet(TCCR1B, CS10);

	Serial.begin(57600);
}

void loop() {
	uint32_t currentTime = millis();

	if ( outState )
	{
		if ( (uint32_t)(currentTime - lastTrigTime) >= onDuration )
		{
			if ( !isTestMode ) analogWrite(MOSFET_PIN, 0);
			outState = LOW;
			lastTrigTime = currentTime;
		}
	}
	else {
		if ( (uint32_t)(currentTime - lastTrigTime) >= offDuration )
		{
			analogWrite(MOSFET_PIN, pwmVal);
			outState = HIGH;
			lastTrigTime = currentTime;
		}
	}

	if ((uint32_t)(currentTime - lastPodUpdateTime) >= 30) {
		isLongPeriodMode = digitalRead(MODE_SW_PIN);
		onDuration = map(analogRead(ON_DURATION_POD_PIN), 0, 1023, onDurationMin[isLongPeriodMode], onDurationMax[isLongPeriodMode]);
		offDuration = map(analogRead(OFF_DURATION_POD_PIN), 0, 1023, offDurationMin[isLongPeriodMode], offDurationMax[isLongPeriodMode]);
		pwmVal = map(analogRead(PWM_POD_PIN), 0, 1023, 0, 255);
		if ( outState || isTestMode )
		{
			analogWrite(MOSFET_PIN, pwmVal);
		}
		if ( isTestMode == digitalRead(TEST_SW_PIN))
		{
			isTestMode = !isTestMode;
			if ( isTestMode ) { analogWrite(MOSFET_PIN, pwmVal); }
			else if ( !outState ) { analogWrite(MOSFET_PIN, 0); }
		}
		lastPodUpdateTime = currentTime;
	}
	if ( (uint32_t)(currentTime-lastSerialReportTime) >= 1000)
	{
		Serial.print(F("On Duration: "));
		Serial.print(onDuration);
		Serial.print(F(",\tOff Duration: "));
		Serial.print(offDuration);
		Serial.print(",\tDimming value:");
		Serial.print(pwmVal);
		Serial.println("/255");
		lastSerialReportTime = currentTime;
	}
}
