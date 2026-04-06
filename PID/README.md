# PID Controller Library

A lightweight, portable PID controller implementation in C, designed for embedded systems. It features derivative kick prevention, conditional anti-windup, an EMA derivative filter, a rate limiter, and a deadband — all configurable at runtime.

---

## Table of Contents

- [Features](#features)
- [File Structure](#file-structure)
- [Data Types](#data-types)
  - [PID\_Status](#pid_status)
  - [PID\_Parameters](#pid_parameters)
  - [PID\_Control](#pid_control)
- [API Reference](#api-reference)
  - [Initialization](#initialization)
  - [Runtime Configuration](#runtime-configuration)
  - [Operation](#operation)
  - [Getters](#getters)
  - [Maintenance](#maintenance)
- [Control Loop Execution Order](#control-loop-execution-order)
- [Usage Example](#usage-example)
- [Design Notes](#design-notes)

---

## Features

- **Derivative on measurement** — eliminates derivative kick on setpoint changes.
- **Conditional anti-windup** — prevents integral accumulation when the output is saturated in the same direction as the error.
- **Integral clamping** — secondary hard clamp on the integral accumulator.
- **EMA derivative filter** — smooths noisy derivative terms via a configurable coefficient.
- **Rate limiter** — restricts the maximum change in output per cycle.
- **Deadband** — errors smaller than the threshold are treated as zero.
- **Variable sample time** — `PID_Update()` accepts the real cycle period `dt` at each call.
- **Safe initialization** — `PID_Init()` clamps `initOutput` to `[minOutput, maxOutput]` automatically.

---

## File Structure

```
PID.h   — Public API: types, parameter struct, function declarations.
PID.c   — Implementation.
```

---

## Data Types

### `PID_Status`

Return code for all API functions (except `PID_Reset()`).

| Value | Meaning |
|---|---|
| `PID_STATUS_OK` | Operation successful. |
| `PID_STATUS_ERROR` | Invalid parameter value. |
| `PID_STATUS_NULL_POINTER` | A `NULL` pointer was received. |

---

### `PID_Parameters`

Configuration structure passed to `PID_Init()`.

| Field | Type | Description | Constraint |
|---|---|---|---|
| `setPoint` | `float` | Target value for the process variable. | — |
| `deadband` | `float` | Errors smaller than this are treated as zero. | `>= 0` |
| `Kp` | `float` | Proportional gain. | `>= 0` |
| `Ki` | `float` | Integral gain. | `>= 0` |
| `Kd` | `float` | Derivative gain. | `>= 0` |
| `alpha` | `float` | EMA coefficient for the derivative filter. `1.0` = no filter (current sample only); `0.0` = frozen (previous sample only). | `[0.0, 1.0]` |
| `rateLimit` | `float` | Maximum output change per cycle (output units/seconds). | `> 0` |
| `minOutput` | `float` | Lower saturation limit. | `< maxOutput` |
| `maxOutput` | `float` | Upper saturation limit. | `> minOutput` |
| `initOutput` | `float` | Output value after `PID_Reset()`. Automatically clamped to `[minOutput, maxOutput]` by `PID_Init()`. | — |
| `initInput` | `float` | Value assigned to `lastInput` after `PID_Reset()`. Prevents a derivative spike on the first cycle. | — |
| `clampIntMin` | `float` | Lower hard clamp for the integral accumulator. | `< clampIntMax` |
| `clampIntMax` | `float` | Upper hard clamp for the integral accumulator. | `> clampIntMin` |
| `dt` | `float` | Default sample period in seconds. Overwritten by the `dt_` argument in each `PID_Update()` call. | `> 0` |

---

### `PID_Control`

Internal controller state. Contains a copy of `PID_Parameters` plus all runtime state variables.

> **Do not access fields directly.** Use the provided API functions to read or modify the controller state.

---

## API Reference

### Initialization

#### `PID_Status PID_Init(PID_Control *Ppid_, PID_Parameters *Pparam_)`

Initializes the controller with the given parameters and calls `PID_Reset()` internally.

**Validations performed:**
- Both pointers must be non-`NULL`.
- `minOutput < maxOutput`
- `clampIntMin < clampIntMax`
- `rateLimit > 0`
- `alpha` in `[0, 1]`
- `dt > 0`
- `deadband >= 0`
- `initOutput` is silently clamped to `[minOutput, maxOutput]` if out of range.

```c
PID_Parameters params = {
    .setPoint    = 100.0f,
    .Kp = 1.2f, .Ki = 0.05f, .Kd = 0.01f,
    .alpha       = 0.8f,
    .rateLimit   = 50.0f,
    .minOutput   = 0.0f,   .maxOutput   = 200.0f,
    .initOutput  = 0.0f,   .initInput   = 0.0f,
    .clampIntMin = -100.0f,.clampIntMax = 100.0f,
    .deadband    = 0.5f,
    .dt          = 0.01f
};

PID_Control pid;
PID_Status status = PID_Init(&pid, &params);
```

---

### Runtime Configuration

All setters return `PID_STATUS_NULL_POINTER` if `Ppid_` is `NULL`, or `PID_STATUS_ERROR` if the value fails validation.

| Function | Description |
|---|---|
| `PID_setSetPoint(Ppid_, setPoint_)` | Updates the target setpoint. |
| `PID_setGain(Ppid_, Kp_, Ki_, Kd_)` | Updates all three gains simultaneously. |
| `PID_setDTime(Ppid_, dt_)` | Updates the default sample period (`dt_ > 0`). Useful when `dt` is constant and you want to configure it without going through `PID_Init()`. Note: each `PID_Update()` call overwrites this value with the real cycle period. |
| `PID_setLimitOut(Ppid_, minOut_, maxOut_)` | Updates output saturation limits (`minOut_ < maxOut_`). |
| `PID_setClampInt(Ppid_, minInt_, maxInt_)` | Updates integral accumulator clamp limits (`minInt_ < maxInt_`). |
| `PID_setRateLimit(Ppid_, rateLimit_)` | Updates the rate limiter (`rateLimit_ > 0`). |
| `PID_setAlpha(Ppid_, alpha_)` | Updates the EMA filter coefficient (`alpha_` in `[0, 1]`). |
| `PID_setDeadband(Ppid_, deadband_)` | Updates the deadband (`deadband_ >= 0`). |

---

### Operation

#### `PID_Status PID_Update(PID_Control *Ppid_, float input_, float dt_)`

Executes one PID cycle.

| Parameter | Description |
|---|---|
| `Ppid_` | Pointer to the controller. |
| `input_` | Current measured process value. |
| `dt_` | Real sample period for this cycle, in seconds (`> 0`). Overwrites the stored `dt`. |

**Internal execution order:**

1. Compute tracking error (`setPoint - input`).
2. Apply deadband.
3. Conditional anti-windup — accumulate integral only if output is not saturated in the same direction as the error.
4. Compute derivative on measurement: `-(input - lastInput) / dt`.
5. Apply EMA filter to derivative.
6. Hard clamp integral accumulator.
7. Compute PID output: `Kp*e + Ki*∫e + Kd*de`.
8. Apply rate limiter.
9. Saturate output to `[minOutput, maxOutput]`.
10. Update previous-cycle state variables.

---

### Getters

#### `PID_Status PID_GetOutput(const PID_Control *Ppid_, float *out_)`

Returns the last computed output value.

```c
float output;
PID_GetOutput(&pid, &output);
```

> Prefer this getter over reading `pid.output` directly to preserve encapsulation.

---

### Maintenance

#### `void PID_Reset(PID_Control *Ppid_)`

Resets all internal state variables to their initial values without modifying the configuration parameters.

- `output` and `lastOutput` → `initOutput`
- `input` and `lastInput` → `initInput`
- All error and accumulator fields → `0.0f`

Useful when restarting control after a pause, a mode switch, or a fault recovery.

---

## Control Loop Execution Order

```
┌─────────────────────────────────────────────┐
│              Each control cycle             │
│                                             │
│  1. Read process variable  →  input_        │
│  2. Measure real elapsed time  →  dt_       │
│  3. PID_Update(&pid, input_, dt_)           │
│  4. PID_GetOutput(&pid, &output)            │
│  5. Apply output to actuator                │
└─────────────────────────────────────────────┘
```

---

## Usage Example

```c
#include "PID.h"

PID_Control   pid;
PID_Parameters params = {
    .setPoint    = 50.0f,
    .Kp = 2.0f, .Ki = 0.1f, .Kd = 0.05f,
    .alpha       = 0.7f,
    .rateLimit   = 20.0f,
    .minOutput   = -100.0f, .maxOutput   = 100.0f,
    .initOutput  = 0.0f,    .initInput   = 0.0f,
    .clampIntMin = -50.0f,  .clampIntMax = 50.0f,
    .deadband    = 0.2f,
    .dt          = 0.01f    /* 10 ms default */
};

void control_init(void)
{
    PID_Init(&pid, &params);
}

void control_loop(void)
{
    float sensor_value = read_sensor();   /* user-provided */
    float dt           = get_elapsed_s(); /* user-provided */
    float output;

    PID_Update(&pid, sensor_value, dt);
    PID_GetOutput(&pid, &output);

    set_actuator(output); /* user-provided */
}
```

---

## Design Notes

**Derivative on measurement vs. derivative on error**
The derivative term is computed as `-(input - lastInput) / dt` rather than `(error - lastError) / dt`. This avoids a sudden large derivative spike ("derivative kick") when the setpoint changes abruptly, because only the measured value — not the setpoint — contributes to the derivative.

**EMA derivative filter**
A first-order exponential moving average smooths the derivative term before it is used in the output calculation. Set `alpha = 1.0` to disable filtering entirely (pure current sample). Lower values introduce more smoothing but also more lag.

**Anti-windup strategy**
Two complementary mechanisms are used: a conditional integrator (does not accumulate when the output is saturated against the error direction) and a hard clamp on the accumulator as a secondary guardrail.

**Rate limiter**
The rate limiter acts on the raw PID output before saturation. It limits the delta between the current and previous output per cycle to `rateLimit * dt`, preventing abrupt actuator commands during transients or setpoint jumps.
