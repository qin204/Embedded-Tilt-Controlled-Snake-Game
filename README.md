# Embedded Tilt-Controlled Snake Game

STM32F401RE Nucleo embedded Snake game with MPU6500 tilt control, DS18S20 temperature sensing, OLED + LCD displays.

## Part 3 — Sensor Processing (倾角控制与传感器处理)

This branch implements the complete sensor-processing layer required by the EBU5477 specification.

### Modules

| File | Responsibility |
|------|----------------|
| `src/filter.c` | First-order low-pass filter (`filtered = α·new + (1-α)·prev`) |
| `src/tilt_control.c` | MPU6500 accelerometer → roll/pitch, calibration, dead zone, direction mapping |
| `src/temp_sensor.c` | Non-blocking DS18S20 reading at 1 Hz |
| `src/sensor_processing.c` | 50 Hz / 1 Hz scheduler and game integration |
| `src/sensor_test_main.c` | Hardware verification entry point |

### Public API (for teammates)

```c
#include "sensor_processing.h"

SensorProcessing_Init();              /* call after hardware + SnakeGame_Init() */
SensorProcessing_Update();            /* every main-loop iteration */

/* Calibration (menu STATE_CALIBRATION) */
SensorProcessing_BeginCalibration();
SensorProcessing_RunCalibration();    /* or rely on Update() while calibrating */
SensorProcessing_EndCalibration();

/* Game integration */
SensorProcessing_ApplyTiltInput();    /* when CONTROL_TILT + STATE_RUNNING */
TiltControl_GetDirection();           /* DIR_NONE / DIR_UP / ... */
TempSensor_GetTemp_x10();
TempSensor_GetWarningLevel();         /* TEMP_WARN_HIGH >= 30 C */
SnakeGame_GetGameSpeed();             /* includes temperature difficulty */
```

### Update Rates (specification)

| Task | Rate | Function |
|------|------|----------|
| MPU6500 read + filter | 50 Hz | `TiltControl_Update()` |
| DS18S20 temperature | 1 Hz | `TempSensor_Update()` |
| Tilt → Snake direction | game tick | `SensorProcessing_ApplyTiltInput()` |

### Configuration (`tilt_control.h`)

- `TILT_LPF_ALPHA` = 0.15
- `TILT_DEAD_ZONE_DEG` = 5°
- `TILT_THRESHOLD_DEG` = 12°
- `TILT_CALIB_SAMPLE_COUNT` = 50 (≈1 s at 50 Hz)

### Optional Features Implemented

- Temperature-based difficulty: above 30 °C the Snake tick interval decreases (faster/harder)
- High-temperature warning at 35 °C (`TempSensor_GetWarningLevel()`, `SnakeGame_IsHighTemperatureWarning()`)
- Non-blocking DS18S20 with timer tick (`DS18S20_Tick_1ms` in TIM2 ISR)

### Build & Test

1. Open `MiniProjectDemo.uvprojx` in Keil µVision
2. Main file: `src/sensor_test_main.c` (replaces demo_main for Part 3 verification)
3. Build and flash to STM32F401RE Nucleo

**Hardware test controls (OLED buttons):**

| Button | Action |
|--------|--------|
| AS | Start MPU6500 calibration (keep board flat) |
| UP | Enable tilt control mode |
| # (Sharp) | Enable button control mode |
| DOWN | Start Snake game test with tilt input |

**Host-side logic tests:**

```bash
python tests/test_sensor_logic.py
```

### Integration Notes

- Part 1: uses `MPU6500_ReadAccelRaw()`, `DS18S20_*` from supplied driver library
- Part 2: calls `SnakeGame_SetDirection()`, `SnakeGame_SetTemperature()`, `SnakeGame_ApplyTemperatureDifficulty()`
- Part 4: use `UI_ShowSensorStatus()`, `UI_ShowCalibrationScreen()` for HMI
- Part 5: replace `sensor_test_main.c` with unified main; keep calling `SensorProcessing_Update()`
