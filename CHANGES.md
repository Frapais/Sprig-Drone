# Changes from upstream Flix

This firmware is a derivative of [Flix](https://github.com/okalachev/flix) by
Oleg Kalachev. It aims to stay close to upstream and only change what the
Sprig-C3 single-board hardware requires. This document lists the modifications
so the diff against upstream is easy to follow.

## `imu.ino`
- Switched the IMU interface from **SPI (MPU-9250)** to **I²C**.
- Declares the IMU as **ICM-20948** (default) or **MPU-6050** on `Wire`, address
  `0x68`, no data-ready pin (timer-interrupt mode).
- `setupIMU()` initialises the I²C bus on **GPIO2 (SDA) / GPIO3 (SCL)** at 400 kHz
  before starting the sensor.
- Faster gyro-bias low-pass filter and a `gyroCalibrated` convergence flag.

## `motors.ino`
- Motor pins remapped to the Sprig-C3 layout (rear-left 7, rear-right 10,
  front-right 6, front-left 5).
- PWM frequency lowered from 78 kHz to **39 kHz** (ESP32-C3 LEDC limit at 10-bit).
- Motors kept off **GPIO8/GPIO9** (boot strapping pins).

## `control.ino`
- Added an **arm gate**: arming and motor output are blocked until the gyro bias
  has converged (`gyroCalibrated`). Guards every arm path (CLI, MAVLink, RC).

## `power.ino`
- New **MAX17048 fuel-gauge** driver (SparkFun MAX1704x library) reporting true
  battery voltage and state-of-charge over I²C (`0x36`).
- Exposes `batteryRemaining` (0–1) for MAVLink telemetry.
- Automatic fallback to the original analog-voltage path if the fuel gauge is
  not present, selectable via the `PWR_FG_ENABLE` parameter.

## `mavlink.ino`
- `BATTERY_STATUS` uses the fuel-gauge state-of-charge when available, falling
  back to the legacy voltage-to-percentage approximation otherwise.

## `led.ino`
- Status-indicator logic for a **MOSFET-driven LED bank on GPIO0**:
  solid off = gyro calibrating, slow blink = disarmed, fast blink = armed,
  very fast = low battery. `LED_ACTIVE_LOW` selectable for wiring polarity.

## `flix.ino`
- Calls `updateLED()` from the main loop.

## `parameters.ino`
- Added the `PWR_FG_ENABLE` parameter (and its `extern`) for the fuel gauge.

---

All original `// Copyright (c) 2023 Oleg Kalachev` headers are retained; a
`// Modifications (c) 2026 Sprig Labs` line is added to the files changed here.
