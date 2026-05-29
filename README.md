# Sprig-Drone — a single-board Flix drone for the Sprig-C3

A compact, single-board build of the open source [**Flix**](https://github.com/okalachev/flix)
quadcopter, designed around the [**Sprig-C3**](https://github.com/Frapais/Sprig-C3)
(ESP32-C3) microcontroller. The PCB integrates the IMU, motor drivers, LEDs and
battery connector, and doubles as the drone's frame — so the whole aircraft
(minus the microcontroller) is a single board.

This repository contains the **firmware** (a modified version of the Flix
Arduino firmware) and the **hardware** documentation needed to build or buy the
drone.

> **Credit & lineage.** This project is based on the Flix open source quadcopter
> by **Oleg Kalachev** — <https://github.com/okalachev/flix> — used and modified
> under the MIT License. Flix provides the flight control firmware, the
> attitude estimator, the MAVLink/QGroundControl integration and the overall
> architecture. This repo adapts that work to the Sprig-C3 board and adds a few
> hardware-specific features (see [What's different](CHANGES.md)).
> Huge thanks to Oleg and the Flix community.

<!--
| 1 | 2 | 3 |
|---|---|---|
<!-- ![Sprig-Drone drone](docs/images/drone.jpg) -->

---

## Features

- **Single-board airframe** — IMU, MOSFET motor drivers, status LEDs and battery
  connector integrated into one PCB that is also the chassis.
- **ESP32-C3** flight controller via the Sprig-C3 module.
- **Wi-Fi + MAVLink** control with the QGroundControl app (no RC receiver
  required).
- **I²C IMU** support: ICM-20948 or MPU-6050 (both from the Flix-compatible list).
- **On-board fuel-gauge** battery telemetry via the Sprig-C3's MAX17048.
- **Status LED indicator** driven through a MOSFET (calibration / armed / low
  battery states).
- Brushed **8520** coreless motors, same geometry as the reference Flix.

## What's different from upstream Flix

This firmware tracks upstream Flix and changes only what the Sprig-C3 hardware
requires. The notable modifications:

| Area | Change |
|------|--------|
| IMU bus | Switched from SPI (MPU-9250) to **I²C**; supports **ICM-20948** and **MPU-6050** on `Wire` (SDA = GPIO2, SCL = GPIO3). |
| Motor PWM | Frequency lowered to **39 kHz** (ESP32-C3 LEDC max at 10-bit) and pins remapped to the Sprig-C3 layout. |
| Strapping pins | Motor pins chosen to avoid GPIO8/GPIO9 (ESP32-C3 boot strapping pins). |
| Gyro calibration | Faster bias convergence and an **arm gate** that blocks arming until the gyro bias has settled. |
| Power | New **MAX17048 fuel-gauge** driver reporting true voltage and state-of-charge to QGroundControl, with automatic fallback to the analog path. |
| LED | Status-indicator logic on a MOSFET-driven LED bank (GPIO0). |

A full list of changed files and rationale is in [`docs/CHANGES.md`](docs/CHANGES.md).

## Hardware

| Component | Choice |
|-----------|--------|
| Flight controller | Sprig-C3 (ESP32-C3) |
| IMU | ICM-20948 or MPU-6050 (I²C, address `0x68`) |
| Motors | 8520 coreless brushed, 3.7 V |
| Motor drivers | logic-level N-channel MOSFETs (e.g. 100N03A) with gate pull-downs |
| Battery | 1S Li-Ion / LiPo (e.g. 18650 for bench testing) |
| Fuel gauge | MAX17048 (on the Sprig-C3, I²C `0x36`) |

### Pin mapping (Sprig-C3)

| Function | GPIO |
|----------|------|
| I²C SDA (IMU + fuel gauge) | 2 |
| I²C SCL (IMU + fuel gauge) | 3 |
| Motor — rear left  | 7 |
| Motor — rear right | 10 |
| Motor — front right | 6 |
| Motor — front left | 5 |
| Status LED (MOSFET gate) | 0 |

> ⚠️ **Do not** put motor or LED MOSFET gates on **GPIO8** or **GPIO9** — they are
> ESP32-C3 boot strapping pins and a gate pull-down on them holds the pin low at
> boot, preventing the chip from initialising (the I²C/IMU bus comes up dead).

Hardware design files and the schematic are in [`hardware/`](hardware/).
**Buy the assembled board:** <https://sprig-labs.com> <!-- update with the exact product link -->

## Firmware setup

> The build process follows upstream Flix closely. See the
> [Flix build guide](https://github.com/okalachev/flix/blob/master/docs/build.md)
> for the general workflow; the Sprig-C3 specifics are below.

1. **Install the Arduino IDE** and the **ESP32 board package** (esp32 by Espressif,
   core 3.x). Select **"ESP32C3 Dev Module"** as the board.
2. **Install libraries** (Library Manager):
   - `FlixPeriph` (latest) — IMU and SBUS drivers.
   - `SparkFun MAX1704x Fuel Gauge` — battery fuel gauge.
   - `MAVLink` — as required by upstream Flix.
3. **Open the sketch:** open the [`flix/`](flix) folder (the sketch is `flix/flix.ino`).
4. **Select the IMU** in `flix/imu.ino` if you are not using the default
   (ICM-20948). For MPU-6050, the declaration is already provided — see comments.
5. **Flash** the board.
6. **First boot:** the firmware creates a Wi-Fi network. On a fresh board, run
   `preset` in the CLI to load the Sprig-C3 default parameters.
7. **Calibrate** the accelerometer (QGroundControl level-horizon calibration, or
   the `ca` CLI command).

### Flying with QGroundControl

1. Power the drone from the battery.
2. Connect your phone/computer to the **`flix`** Wi-Fi network (password
   `flixwifi`).
3. Open **QGroundControl** — it connects automatically and shows telemetry.
4. Enable the **Virtual Joystick** in settings (disable Auto-Center Throttle).
5. Let the drone sit still for ~5 s so the gyro calibrates, then arm and fly.

See [`docs/SETUP.md`](docs/SETUP.md) for detailed setup, tuning and troubleshooting.

## Repository layout

```
Sprig-Drone/
├── flix - Sprig/              # Arduino firmware (sketch folder; open flix/flix.ino)
├── hardware/          # PCB schematic, design files, BOM
├── docs/              # Setup, changes, troubleshooting, images
├── LICENSE            # MIT (upstream Flix + Sprig Labs modifications)
└── README.md
```

## Video

<!-- Add your YouTube build/showcase video link here -->
A full build and flight video is on YouTube: _coming soon_.

## License

This project is released under the **MIT License** — see [`LICENSE`](LICENSE).
The firmware is a derivative of [Flix](https://github.com/okalachev/flix) by Oleg
Kalachev, also MIT-licensed; the original copyright notices are retained in the
source files. You are free to use, modify, and sell — please keep the attribution.

Hardware design files (if/where included) are licensed separately; see
[`hardware/README.md`](hardware/README.md).

## Acknowledgements

- **Oleg Kalachev** and the [Flix project](https://github.com/okalachev/flix) — the
  firmware, flight controller and the whole foundation this builds on.
- The [FlixPeriph](https://github.com/okalachev/flixperiph) peripheral library.
- The open source quadcopter community.
