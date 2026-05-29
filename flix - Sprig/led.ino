// Copyright (c) 2023 Oleg Kalachev <okalachev@gmail.com>
// Repository: https://github.com/okalachev/flix

// Board's LED control

#define BLINK_PERIOD 500000

// On the Sprig-C3 (ESP32-C3) build, GPIO2 is used for I2C SDA and GPIOs 4-7
// for the motors, so the original default of GPIO2 cannot be used here.
// GPIO8 is the on-board LED on most ESP32-C3 dev boards; change if needed.
// If your board has no usable LED, this can point at any free GPIO harmlessly.
#ifndef LED_BUILTIN
#define LED_BUILTIN 0
#endif

void setupLED() {
	pinMode(LED_BUILTIN, OUTPUT);
}

void setLED(bool on) {
	static bool state = false;
	if (on == state) {
		return; // don't call digitalWrite if the state is the same
	}
	digitalWrite(LED_BUILTIN, on ? HIGH : LOW);
	state = on;
}

void blinkLED() {
	setLED(micros() / BLINK_PERIOD % 2);
}
