// Copyright (c) 2023 Oleg Kalachev <okalachev@gmail.com>
// Repository: https://github.com/okalachev/flix

// Work with the IMU sensor

#include <Wire.h>
#include <FlixPeriph.h>
#include "vector.h"
#include "lpf.h"
#include "util.h"

// MPU-6050 connected over I2C on the Sprig-C3 board.
// SDA -> GPIO2, SCL -> GPIO3 (these are also the board's fuel-gauge I2C pins).
#define IMU_SDA_PIN 2
#define IMU_SCL_PIN 3
// I2C address of the MPU-6050: 0x68 when AD0 is low, 0x69 when AD0 is high.
// Change this if the IMU is not detected (check with the `imu` CLI command,
// whoAmI should report 0x68 for the MPU-6050).
#define IMU_I2C_ADDR 0x68

MPU6050 imu(Wire, -1, IMU_I2C_ADDR); // MPU-6050 over I2C, no data-ready pin, explicit address
Vector imuRotation(0, 0, -1.571); // imu orientation as Euler angles

Vector gyro; // gyroscope output, rad/s
Vector gyroBias;
bool gyroCalibrated = false; // true once the gyro bias estimate has converged

Vector acc; // accelerometer output, m/s/s
Vector accBias;
Vector accScale(1, 1, 1);

// Gyro bias low-pass filter. Higher alpha converges faster but tracks noise more;
// 0.01 settles in a few seconds of being stationary instead of ~30 with 0.001.
LowPassFilter<Vector> gyroBiasFilter(0.01);

void setupIMU() {
	print("Setup IMU\n");
	Wire.begin(IMU_SDA_PIN, IMU_SCL_PIN);
	Wire.setClock(400000); // use 400 kHz I2C clock
	imu.begin();
	configureIMU();
}

void configureIMU() {
	imu.setAccelRange(imu.ACCEL_RANGE_4G);
	imu.setGyroRange(imu.GYRO_RANGE_2000DPS);
	imu.setDLPF(imu.DLPF_MAX);
	imu.setRate(imu.RATE_1KHZ_APPROX);
	imu.setupInterrupt();
}

void readIMU() {
	imu.waitForData();
	imu.getGyro(gyro.x, gyro.y, gyro.z);
	imu.getAccel(acc.x, acc.y, acc.z);
	calibrateGyroOnce();
	// apply scale and bias
	acc = (acc - accBias) / accScale;
	gyro = gyro - gyroBias;
	// rotate to body frame
	Quaternion rotation = Quaternion::fromEuler(imuRotation);
	acc = Quaternion::rotateVector(acc, rotation.inversed());
	gyro = Quaternion::rotateVector(gyro, rotation.inversed());
}

void calibrateGyroOnce() {
	static Delay landedDelay(2);
	if (!landedDelay.update(landed)) return; // calibrate only if definitely stationary
	gyroBias = gyroBiasFilter.update(gyro);

	// Mark the gyro as calibrated once it has been stationary long enough for the
	// bias filter to converge. landedDelay already required 2 s of being landed;
	// this adds a further settle window. While calibrated, keep refining the bias.
	static Delay calibratedDelay(3);
	gyroCalibrated = calibratedDelay.update(true);
}

void calibrateAccel() {
	print("Calibrating accelerometer\n");
	imu.setAccelRange(imu.ACCEL_RANGE_2G); // the most sensitive mode

	print("1/6 Place level [8 sec]\n");
	pause(8);
	calibrateAccelOnce();
	print("2/6 Place nose up [8 sec]\n");
	pause(8);
	calibrateAccelOnce();
	print("3/6 Place nose down [8 sec]\n");
	pause(8);
	calibrateAccelOnce();
	print("4/6 Place on right side [8 sec]\n");
	pause(8);
	calibrateAccelOnce();
	print("5/6 Place on left side [8 sec]\n");
	pause(8);
	calibrateAccelOnce();
	print("6/6 Place upside down [8 sec]\n");
	pause(8);
	calibrateAccelOnce();

	printIMUCalibration();
	print("✓ Calibration done!\n");
	configureIMU();
}

void calibrateAccelOnce() {
	const int samples = 1000;
	static Vector accMax(-INFINITY, -INFINITY, -INFINITY);
	static Vector accMin(INFINITY, INFINITY, INFINITY);

	// Compute the average of the accelerometer readings
	acc = Vector(0, 0, 0);
	for (int i = 0; i < samples; i++) {
		imu.waitForData();
		Vector sample;
		imu.getAccel(sample.x, sample.y, sample.z);
		acc = acc + sample;
	}
	acc = acc / samples;

	// Update the maximum and minimum values
	if (acc.x > accMax.x) accMax.x = acc.x;
	if (acc.y > accMax.y) accMax.y = acc.y;
	if (acc.z > accMax.z) accMax.z = acc.z;
	if (acc.x < accMin.x) accMin.x = acc.x;
	if (acc.y < accMin.y) accMin.y = acc.y;
	if (acc.z < accMin.z) accMin.z = acc.z;
	// Compute scale and bias
	accScale = (accMax - accMin) / 2 / ONE_G;
	accBias = (accMax + accMin) / 2;
}

void printIMUCalibration() {
	print("gyro bias: %f %f %f\n", gyroBias.x, gyroBias.y, gyroBias.z);
	print("gyro calibrated: %d\n", gyroCalibrated);
	print("accel bias: %f %f %f\n", accBias.x, accBias.y, accBias.z);
	print("accel scale: %f %f %f\n", accScale.x, accScale.y, accScale.z);
}

void printIMUInfo() {
	imu.status() ? print("status: ERROR %d\n", imu.status()) : print("status: OK\n");
	print("model: %s\n", imu.getModel());
	print("who am I: 0x%02X\n", imu.whoAmI());
	print("rate: %.0f\n", loopRate);
	print("gyro: %f %f %f\n", gyro.x, gyro.y, gyro.z);
	print("acc: %f %f %f\n", acc.x, acc.y, acc.z);
	imu.waitForData();
	Vector rawGyro, rawAcc;
	imu.getGyro(rawGyro.x, rawGyro.y, rawGyro.z);
	imu.getAccel(rawAcc.x, rawAcc.y, rawAcc.z);
	print("raw gyro: %f %f %f\n", rawGyro.x, rawGyro.y, rawGyro.z);
	print("raw acc: %f %f %f\n", rawAcc.x, rawAcc.y, rawAcc.z);
}
