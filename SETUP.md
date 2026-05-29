# Setup, tuning & troubleshooting

## Prerequisites

- Arduino IDE with the **ESP32 board package** (Espressif, core 3.x).
- Board selection: **ESP32C3 Dev Module**.
- Libraries (Library Manager):
  - **FlixPeriph** (latest) — IMU + SBUS drivers.
  - **SparkFun MAX1704x Fuel Gauge** — battery telemetry.
  - **MAVLink** — required by upstream Flix.
- **QGroundControl** app on a phone or computer.

## Choosing the IMU

The firmware defaults to the **ICM-20948**. To use an **MPU-6050**, change the
IMU declaration at the top of `flix/imu.ino` (see the comments there). Both use
I²C address `0x68` by default — if your breakout uses `0x69`, change
`IMU_I2C_ADDR`. Confirm detection with the `imu` CLI command:
- ICM-20948 → `who am I: 0xEA`
- MPU-6050 → `who am I: 0x68`

## First boot

1. Flash the firmware.
2. Connect to the **`flix`** Wi-Fi network (password `flixwifi`).
3. Open the CLI (wireless, or the Arduino Serial Monitor) and run **`preset`** to
   load the Sprig-C3 default parameters. *Do this once on a fresh board* — the
   firmware persists parameters in flash, and `preset` ensures the source-code
   defaults (pins, PWM frequency, fuel-gauge enable) are applied.
4. Run the accelerometer calibration: `ca` in the CLI, or QGroundControl's
   level-horizon calibration. Re-run this if you swap the IMU.

## Verifying motors

With **props removed**, use the CLI motor-test commands and confirm the correct
motor spins:
- `mfl` → front left, `mfr` → front right, `mrl` → rear left, `mrr` → rear right.

If the wrong motor spins, the pin-to-motor mapping is off. Set it live without
reflashing:

```
par MOT_PIN_FL 5
par MOT_PIN_FR 6
par MOT_PIN_RL 7
par MOT_PIN_RR 10
```

Then reboot. Also confirm spin **direction**: FL and RR turn one way, FR and RL
the other. Reverse a brushed motor by swapping its two leads.

## Flying

1. Power from the battery.
2. Join the `flix` Wi-Fi; open QGroundControl (connects automatically).
3. Enable **Virtual Joystick**; disable **Auto-Center Throttle**.
4. Let the drone sit still ~5 s — the gyro must calibrate before it will arm
   (the status LED is solid off until calibrated, then slow-blinks when ready).
5. Arm and fly.

## Tuning (if it oscillates)

The controller is a two-level cascade PID (outer attitude loop → inner rate
loop). All gains are live parameters — adjust in QGroundControl or via `par`,
no reflash needed. Roll and pitch share gains; change them together.

- **Fast oscillation / buzz, worse on stick input** → inner rate loop too hot.
  Try `CTL_R_RATE_D = 0` / `CTL_P_RATE_D = 0` first (the MPU-6050 is noisy and
  often flies better with no D-term), then lower `CTL_R_RATE_P` / `CTL_P_RATE_P`
  in ~20–25 % steps.
- **Slow wobble that builds** → lower rate I (`CTL_R_RATE_I` / `CTL_P_RATE_I`).
- **Sluggish, won't hold angle** → raise the outer loop P (`CTL_R_P` / `CTL_P_P`).

Change one gain at a time, in calm conditions. Distinguish **oscillation** (PID)
from **vibration** (mechanical): if the attitude readout shakes when you tap the
frame with motors off, soft-mount the IMU and balance the props — no PID value
fixes vibration.

## Troubleshooting

**IMU not detected (`who am I: 0x00`, `ERROR`).**
- Check the I²C address (`0x68` vs `0x69`).
- Make sure nothing is wired to **GPIO8/GPIO9** with a pull-down — a MOSFET gate
  pull-down on a strapping pin holds it low at boot and kills the I²C bus.
- Inspect for solder bridges between SDA/SCL (GPIO2/3) and adjacent pins; reflow
  the I²C and IMU power/ground joints. Measure SDA-to-GND and SCL-to-GND — both
  should idle near 3.3 V thanks to the pull-ups.

**Fuel gauge not reporting.**
- The MAX17048 is powered from the battery cell, not 3.3 V — connect a battery.
- Disable it with `par PWR_FG_ENABLE 0` to fall back to analog if needed.

**LED doesn't light (MOSFET-driven).**
- Confirm the MOSFET gate has a pull-down and the LED bank has current-limiting
  resistors. Measure the GPIO while running: disarm so the LED slow-blinks
  (1 Hz), which a multimeter can track.
- GPIO0 is usable but is also ADC1_CH0; if it misbehaves, move the gate to a
  free pin (e.g. GPIO1) and update `LED_BUILTIN`.

**Gyro drifts right after arming.**
- Let the drone sit still longer before arming; the bias filter must converge.
  The arm gate should prevent this, but a very quick arm on a cold board can
  still catch the tail of convergence.
