# PID Controller Simulation

A simulation program written in C that validates the PID controller library against a first-order plant model. Each cycle the plant output feeds back into the PID controller, and all signals are logged to CSV files for analysis.

---

## Table of Contents

- [Overview](#overview)
- [Dependencies](#dependencies)
- [File Structure](#file-structure)
- [Configuration Constants](#configuration-constants)
- [Simulation Parameters](#simulation-parameters)
- [Data Logging](#data-logging)
- [Control Loop](#control-loop)
- [Initialization Functions](#initialization-functions)
- [Output Files](#output-files)
- [How to Run](#how-to-run)

---

## Overview

The simulation runs a closed-loop PID control scenario for `ITERATIONS` cycles at a fixed sample period. On each cycle:

1. The PID output is applied to the plant together with a constant disturbance.
2. The plant returns a measured output.
3. The measured output is fed back into the PID controller.
4. All signals are written to a CSV data log.

The plant is modeled as a first-order system with configurable gain, time constant, saturation, noise, and input/output delay.

```
              disturbance
                  │
setPoint ──► [PID] ──► u ──► [Plant] ──► outMeasured ──┐
               ▲                                       │
               └───────────────────────────────────────┘
```

---

## Dependencies

| Module | Header | Description |
|---|---|---|
| PID controller | `PID.h` | Closed-loop PID controller. |
| Data logger | `Logger.h` | Logs float arrays and string events to CSV files. |
| Plant model | `Plant.h` | First-order plant simulation with noise and delay. |

---

## File Structure

```
main.h    — Compile-time constants and the dataInfoLogger enum.
main.c    — Entry point, simulation loop, and initialization helpers.
```

---

## Configuration Constants

Defined in `main.h`.

| Constant | Value | Description |
|---|---|---|
| `ITERATIONS` | `1000` | Number of simulation cycles to run. |
| `OUTPUT_PID_MAX` | `100` | Upper saturation limit of the PID output. |
| `OUTPUT_PID_MIN` | `0` | Lower saturation limit of the PID output. |
| `OUTPUT_PID_INIT` | `100` | Initial PID output value (also the plant input initial condition). |
| `INPUT_PID_INIT` | `12` | Initial plant output / PID input value. |
| `PERIOD` | `1` | Sample period in seconds. |
| `DISTURBANCE` | `20` | Constant disturbance added to the plant input each cycle. |

---

## Simulation Parameters

### Plant (`TEST_Plant_Init`)

| Field | Value | Description |
|---|---|---|
| `K` | `-0.2` | Plant input gain. |
| `tau` | `60` | Time constant (seconds). |
| `initInput` | `OUTPUT_PID_INIT` | Initial condition for the plant input. |
| `initOutput` | `INPUT_PID_INIT` | Initial condition for the plant output. |
| `minOut / maxOut` | `-5 / 25` | Output saturation limits. |
| `minIn / maxIn` | `OUTPUT_PID_MIN / OUTPUT_PID_MAX` | Input saturation limits. |
| `noiseAmpIN` | `0` | Input noise amplitude (disabled). |
| `noiseAmpOUT` | `0` | Output noise amplitude (disabled). |
| `delaySamplesIN` | `1` | Input delay in samples. |
| `delaySamplesOUT` | `1` | Output delay in samples. |

### PID Controller (`TEST_PID_Init`)

| Field | Value | Description |
|---|---|---|
| `setPoint` | `10` | Target process value. |
| `Kp / Ki / Kd` | `-60 / -2 / -10` | Negative gains (plant has negative gain K). |
| `alpha` | `0.7` | EMA derivative filter coefficient. |
| `rateLimit` | `20` | Max output change per cycle. |
| `minOutput / maxOutput` | `0 / 100` | Output saturation limits. |
| `initOutput` | `OUTPUT_PID_INIT` | Initial output after reset. |
| `initInput` | `INPUT_PID_INIT` | Initial input (prevents derivative spike at cycle 0). |
| `clampIntMin / clampIntMax` | `-35 / 35` | Integral accumulator clamp limits. |
| `deadband` | `0` | No deadband applied. |
| `dt` | `PERIOD` | Default sample period. |

> The PID gains are negative because the plant gain `K` is negative. A positive error requires a positive output increase, but since the plant inverts the sign, the controller gains must compensate.

---

## Data Logging

Logged signals are stored in a `float data[5]` array. The index mapping is defined by the `dataInfoLogger` enum in `main.h`:

| Index | Enum value | Signal logged |
|---|---|---|
| `0` | `inputPID` | Current PID input (plant measured output). |
| `1` | `outputPID` | Current PID output (plant input command). |
| `2` | `Error` | Tracking error (`setPoint - input`). |
| `3` | `Error_int` | Integral error accumulator. |
| `4` | `Error_der` | Filtered derivative error. |

> Signals are captured **before** `plant_Update()` and `PID_Update()` are called, so each row in the log corresponds to the state at the **start** of that cycle.

Two CSV files are produced:

| File | Content |
|---|---|
| `dataLogger.csv` | Float signal array (5 columns, one row per cycle). |
| `eventLogger.csv` | String event log (reserved for event descriptions). |

---

## Control Loop

```c
for (size_t i = 0; i < ITERATIONS; i++)
{
    // 1. Capture signals at the start of the cycle
    data[inputPID]  = hPID.input;
    data[outputPID] = hPID.output;
    data[Error]     = hPID.error;
    data[Error_int] = hPID.integralError;
    data[Error_der] = hPID.derivativeError;

    // 2. Advance the plant one step
    plant_Update(&hPlant, hPID.output, DISTURBANCE, PERIOD);

    // 3. Update the PID with the new plant measurement
    PID_Update(&hPID, hPlant.outMeasured, PERIOD);

    // 4. Write data to CSV
    logData(&hLogger, i);
}
```

If either `plant_Update()` or `PID_Update()` returns an error, the loop prints a diagnostic message and breaks immediately.

---

## Initialization Functions

| Function | Initializes | Halts on error |
|---|---|---|
| `TEST_LoggerInit()` | `hLogger` — configures data and event CSV loggers. | Yes (`while(1)`) |
| `TEST_Plant_Init()` | `hPlant` — configures the first-order plant model. | Yes (`while(1)`) |
| `TEST_PID_Init()` | `hPID` — configures the PID controller. | Yes (`while(1)`) |

All three are called at the start of `main()` before the simulation loop begins.

---

## Output Files

After a successful run, two files are created in the working directory:

- **`dataLogger.csv`** — 1000 rows × 5 columns of float data, with a header row:
  ```
  INPUT PID, OUTPUT PID, ERROR, ERROR INTEGRAL, ERROR DERIVATIVO
  ```
- **`eventLogger.csv`** — event log with a `DESCRIPCION` column.

These files can be opened in Excel, MATLAB, Python (pandas/matplotlib), or any CSV-compatible tool for further analysis and plotting.

