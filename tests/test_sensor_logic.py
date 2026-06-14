#!/usr/bin/env python3
"""
Host-side verification for Part 3 sensor algorithms.
Mirrors filter.c, tilt_control direction mapping, and temperature difficulty.
"""

import math
import sys

TILT_DEAD_ZONE_DEG = 5.0
TILT_THRESHOLD_DEG = 12.0
TILT_LPF_ALPHA = 0.15
TILT_ACCEL_SENSITIVITY = 16384.0


class LowPassFilter:
    def __init__(self, alpha: float):
        self.alpha = alpha
        self.value = 0.0
        self.initialized = False

    def update(self, sample: float) -> float:
        if not self.initialized:
            self.value = sample
            self.initialized = True
            return self.value
        self.value = self.alpha * sample + (1.0 - self.alpha) * self.value
        return self.value

    def reset(self, value: float = 0.0):
        self.value = value
        self.initialized = True


def compute_angles(ax: int, ay: int, az: int) -> tuple[float, float]:
    ax_g = ax / TILT_ACCEL_SENSITIVITY
    ay_g = ay / TILT_ACCEL_SENSITIVITY
    az_g = az / TILT_ACCEL_SENSITIVITY
    roll = math.degrees(math.atan2(ay_g, az_g))
    pitch = math.degrees(math.atan2(-ax_g, math.sqrt(ay_g * ay_g + az_g * az_g)))
    return roll, pitch


def clamp_dead_zone(angle: float) -> float:
    if abs(angle) < TILT_DEAD_ZONE_DEG:
        return 0.0
    return angle


def get_direction(roll: float, pitch: float) -> str:
    roll = clamp_dead_zone(roll)
    pitch = clamp_dead_zone(pitch)
    abs_roll = abs(roll)
    abs_pitch = abs(pitch)

    if abs_roll < TILT_THRESHOLD_DEG and abs_pitch < TILT_THRESHOLD_DEG:
        return "NONE"

    if abs_roll >= abs_pitch:
        if roll > TILT_THRESHOLD_DEG:
            return "RIGHT"
        if roll < -TILT_THRESHOLD_DEG:
            return "LEFT"
    else:
        if pitch > TILT_THRESHOLD_DEG:
            return "DOWN"
        if pitch < -TILT_THRESHOLD_DEG:
            return "UP"
    return "NONE"


def apply_temperature_difficulty(base_ms: int, temp_x10: int) -> int:
    adjust = 0
    if temp_x10 > 300:
        excess = temp_x10 - 300
        adjust = -((excess * 2) // 5)
        if temp_x10 >= 350:
            adjust -= 10
    effective = base_ms + adjust
    return max(80, min(250, effective))


def accel_for_tilt(roll_deg: float, pitch_deg: float) -> tuple[int, int, int]:
    """Synthesize accelerometer readings for a given tilt."""
    roll = math.radians(roll_deg)
    pitch = math.radians(pitch_deg)
    ax_g = -math.sin(pitch)
    ay_g = math.sin(roll) * math.cos(pitch)
    az_g = math.cos(roll) * math.cos(pitch)
    return (
        int(ax_g * TILT_ACCEL_SENSITIVITY),
        int(ay_g * TILT_ACCEL_SENSITIVITY),
        int(az_g * TILT_ACCEL_SENSITIVITY),
    )


def test_low_pass_filter():
    f = LowPassFilter(0.15)
    assert f.update(10.0) == 10.0
    v = f.update(10.0)
    assert abs(v - 10.0) < 1e-6
    v2 = f.update(20.0)
    expected = 0.15 * 20.0 + 0.85 * 10.0
    assert abs(v2 - expected) < 1e-6, f"LPF expected {expected}, got {v2}"


def test_flat_orientation():
    roll, pitch = compute_angles(0, 0, 16384)
    assert abs(roll) < 1.0, roll
    assert abs(pitch) < 1.0, pitch


def test_roll_right():
    ax, ay, az = accel_for_tilt(20.0, 0.0)
    roll, pitch = compute_angles(ax, ay, az)
    assert roll > 15.0, roll
    assert get_direction(roll, pitch) == "RIGHT"


def test_pitch_up():
    ax, ay, az = accel_for_tilt(0.0, -20.0)
    roll, pitch = compute_angles(ax, ay, az)
    assert pitch < -15.0, pitch
    assert get_direction(roll, pitch) == "UP"


def test_dead_zone():
    assert get_direction(3.0, 0.0) == "NONE"
    assert get_direction(0.0, 4.0) == "NONE"


def test_calibration_offset():
    samples = [compute_angles(100, -50, 16300) for _ in range(50)]
    roll_off = sum(r for r, _ in samples) / len(samples)
    pitch_off = sum(p for _, p in samples) / len(samples)

    rf = LowPassFilter(TILT_LPF_ALPHA)
    pf = LowPassFilter(TILT_LPF_ALPHA)
    ax, ay, az = accel_for_tilt(0.0, 0.0)
    roll, pitch = compute_angles(ax, ay, az)
    roll -= roll_off
    pitch -= pitch_off
    for _ in range(20):
        roll_f = rf.update(roll)
        pitch_f = pf.update(pitch)
    assert abs(roll_f) < 3.0, roll_f
    assert abs(pitch_f) < 3.0, pitch_f


def test_temperature_difficulty():
    assert apply_temperature_difficulty(200, 250) == 200
    assert apply_temperature_difficulty(200, 320) == 192
    assert apply_temperature_difficulty(200, 350) == 170
    assert apply_temperature_difficulty(200, 500) == 110
    assert apply_temperature_difficulty(100, 400) == 80


def test_dominant_axis():
    assert get_direction(15.0, 8.0) == "RIGHT"
    assert get_direction(8.0, 15.0) == "DOWN"


def main():
    tests = [
        test_low_pass_filter,
        test_flat_orientation,
        test_roll_right,
        test_pitch_up,
        test_dead_zone,
        test_calibration_offset,
        test_temperature_difficulty,
        test_dominant_axis,
    ]
    passed = 0
    for test in tests:
        test()
        passed += 1
        print(f"PASS: {test.__name__}")
    print(f"\nAll {passed} sensor logic tests passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
