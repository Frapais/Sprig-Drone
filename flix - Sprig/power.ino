// Copyright (c) 2026 Oleg Kalachev <okalachev@gmail.com>
// Repository: https://github.com/okalachev/flix

// Power management
//
// Supports two voltage/state-of-charge sources, chosen at runtime by the
// PWR_FG_ENABLE parameter:
//   - PWR_FG_ENABLE != 0 (default for the Sprig-C3 build): read voltage and
//     state-of-charge from the MAX17048 fuel-gauge IC over I2C (address 0x36),
//     which shares the IMU I2C bus on GPIO2 (SDA) / GPIO3 (SCL).
//   - PWR_FG_ENABLE == 0: fall back to the original analog-voltage reading on
//     the GPIO pin specified by PWR_VOLT_PIN, scaled by PWR_VOLT_SCALE. The
//     state-of-charge is then estimated by mapping voltage to [0..1] in
//     mavlink.ino (legacy 3.4 V .. 4.2 V linear approximation).
// If fuel-gauge mode is enabled but the IC does not respond (e.g. board powered
// from USB only with no battery connected to the IC), the code automatically
// falls back to the analog path so the firmware keeps working.

#include <Wire.h>
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h>
#include <soc/soc.h>
#include <soc/rtc_cntl_reg.h>
#include "lpf.h"
#include "util.h"

#define POWER_I2C_SDA_PIN 2
#define POWER_I2C_SCL_PIN 3

float voltage;
float batteryRemaining = NAN; // state of charge in [0..1], NAN if unknown
LowPassFilter<float> voltageFilter(0.2);
int voltagePin = -1;
float voltageScale = 2;
// Master switch: 1 = use the MAX17048 fuel gauge over I2C (Sprig-C3 default),
// 0 = use the analog voltage divider on PWR_VOLT_PIN.
int fuelGaugeEnabled = 1;

SFE_MAX1704X fuelGauge(MAX1704X_MAX17048);
bool fuelGaugePresent = false;

void setupPower() {
	// Disable reset on low voltage
	REG_CLR_BIT(RTC_CNTL_BROWN_OUT_REG, RTC_CNTL_BROWN_OUT_ENA);

	if (!fuelGaugeEnabled) return;

	// Ensure the I2C bus is up. Wire.begin is idempotent on the ESP32 Arduino
	// core, so it is safe to call here even though setupIMU() also calls it.
	Wire.begin(POWER_I2C_SDA_PIN, POWER_I2C_SCL_PIN);
	Wire.setClock(400000);

	print("Setup Power (MAX17048 fuel gauge)\n");
	if (fuelGauge.begin()) {
		fuelGaugePresent = true;
		// Quick-start re-initialises the IC's voltage-to-SOC tracking. Useful
		// after a power-cycle so the first readings are not stale.
		fuelGauge.quickStart();
	} else {
		fuelGaugePresent = false;
		print("MAX17048 not detected - falling back to analog voltage reading\n");
	}
}

void readVoltage() {
	static Rate rate(10);
	if (!rate) return;

	if (fuelGaugeEnabled && fuelGaugePresent) {
		// MAX17048: native voltage in volts, native SOC in percent.
		float v = fuelGauge.getVoltage();
		float soc = fuelGauge.getSOC(); // percent, 0..100 (can briefly exceed)
		if (isfinite(v)) voltage = voltageFilter.update(v);
		if (isfinite(soc)) batteryRemaining = constrain(soc / 100.0f, 0.0f, 1.0f);
		return;
	}

	// Analog fallback path - identical to the original behaviour.
	if (voltagePin < 0) return;
	float v = analogReadMilliVolts(voltagePin) * voltageScale / 1000.0f;
	voltage = voltageFilter.update(v);
	batteryRemaining = NAN; // let mavlink.ino derive remaining from voltage
}
