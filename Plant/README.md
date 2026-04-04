# Plant — First-Order Dynamic System Simulator

This library simulates the behavior of a first-order dynamic system, modeled
by the following discrete-time expression derived from Euler's forward method:

$$
y[k] = y[k-1] + \frac{T}{\tau} \left( K \cdot x[k-N_{in}] - y[k-1] + d[k] \right)
$$

where:

| Symbol          | Description                               |
|-----------------|-------------------------------------------|
| $y[k]$          | Current output                            |
| $y[k-1]$        | Previous output                           |
| $T$             | Sample period [s]                         |
| $\tau$          | System time constant [s]                  |
| $K$             | Input gain (static gain)                  |
| $x[k-N_{in}]$  | Input delayed by $N_{in}$ samples         |
| $d[k]$          | External disturbance at current sample    |

The library was designed to test and validate control algorithms — such as PID
controllers — by providing a configurable plant that realistically models:

- First-order dynamics
- Transport delay on the input (e.g. valve response, fluid transport time)
- Measurement delay on the output (e.g. slow sensor response)
- Additive noise on the output (e.g. sensor noise)
- Input and output saturation

Although originally developed to simulate a **refrigeration evaporator** — where
the input is the electronic expansion valve opening percentage and the output is
the **Superheat (SH)** — the API is generic enough to represent any first-order system.

---

## API Overview

The public interface consists of four functions:

| Function       | Description                                          |
|----------------|------------------------------------------------------|
| `plant_Init`   | Initializes the plant and allocates internal buffers |
| `plant_Update` | Computes a new output sample                         |
| `plant_Reset`  | Restores the plant to its initial conditions         |
| `plant_Deinit` | Releases all allocated memory                        |

---

## Quick Start

```c
#include "Plant.h"

Plant_t     myPlant;
configPlant myConfig = {
    .tau             = 5.0f,   // Time constant [s]
    .K               = 1.2f,   // Input gain
    .initInput       = 0.0f,   // Initial input condition
    .initOutput      = 10.0f,  // Initial output condition
    .minIn           = 0.0f,   // Input lower saturation limit
    .maxIn           = 100.0f, // Input upper saturation limit
    .minOut          = 0.0f,   // Output lower saturation limit
    .maxOut          = 30.0f,  // Output upper saturation limit
    .noiseAmp        = 0.1f,   // Output noise amplitude
    .delaySamplesIN  = 3,      // Input delay (samples)
    .delaySamplesOUT = 2,      // Output delay (samples)
};

// Initialize once
plant_Init(&myPlant, &myConfig);

// Call periodically at each control cycle
float pidOutput    = 50.0f;  // Output from your PID controller
float disturbance  = 0.0f;   // External disturbance (thermal load, etc.)
float samplePeriod = 0.1f;   // [s] — must satisfy T < 2 * tau

plant_Update(&myPlant, pidOutput, disturbance, samplePeriod);

// Read the measured output (delayed + noise) — this is what your PID should see
float superheat = myPlant.measuredOut;

// Free resources when done
plant_Deinit(&myPlant);
```

---

## Configuration Reference

All parameters are set through the `configPlant` struct before calling `plant_Init`.

### Dynamics

| Parameter | Type    | Description                                                                     |
|-----------|---------|---------------------------------------------------------------------------------|
| `tau`     | `float` | Time constant of the system [s]. Must be non-zero.                              |
| `K`       | `float` | Static gain — proportional relationship between input and steady-state output.  |

### Initial Conditions

| Parameter    | Type    | Description                                                                                        |
|--------------|---------|----------------------------------------------------------------------------------------------------|
| `initInput`  | `float` | Initial value preloaded into the input delay buffer. Must be within `[minIn, maxIn]`.              |
| `initOutput` | `float` | Initial output value. Preloaded into `out`, `outPrev`, and the output delay buffer. Must be within `[minOut, maxOut]`. |

Setting these correctly avoids artificial transients at startup — the system
begins as if it had been running at steady state with these values.

### Saturation

| Parameter | Type    | Description                        |
|-----------|---------|------------------------------------|
| `minIn`   | `float` | Lower saturation limit for input.  |
| `maxIn`   | `float` | Upper saturation limit for input.  |
| `minOut`  | `float` | Lower saturation limit for output. |
| `maxOut`  | `float` | Upper saturation limit for output. |

Any value outside these bounds is clamped before being processed by the plant dynamics.

### Noise

| Parameter  | Type    | Description                                                                 |
|------------|---------|-----------------------------------------------------------------------------|
| `noiseAmp` | `float` | Maximum amplitude of the additive random noise applied to the output. Set to `0.0f` to disable. Must be >= 0. |

The noise is uniformly distributed in the range `[-noiseAmp, +noiseAmp]` and
models the measurement uncertainty of a real sensor. The random seed is
initialized once in `plant_Init`.

### Delay

| Parameter        | Type  | Description                                                                                      |
|------------------|-------|--------------------------------------------------------------------------------------------------|
| `delaySamplesIN` | `int` | Number of samples by which the input is delayed before entering the plant dynamics. Must be > 0. |
| `delaySamplesOUT`| `int` | Number of samples by which the output is delayed before being delivered to the controller. Must be > 0. |

Each delay is implemented as an independent circular FIFO buffer. The two delays
model distinct physical phenomena:

```
Controller ──► [Input Delay] ──► Plant Dynamics ──► [Output Delay] ──► Controller
                (valve / fluid                        (sensor response
                 transport time)                       / signal processing)
```

- **Input delay** (`delaySamplesIN`): models the time between the controller
  issuing a command and the plant actually responding — for example, the
  mechanical response of an expansion valve or the refrigerant transport time
  from the valve to the evaporator.

- **Output delay** (`delaySamplesOUT`): models the time between the plant
  reaching a new state and the controller receiving the updated measurement —
  for example, a slow temperature or pressure sensor.

> **Important:** `plant_->out` always holds the true (undelayed) plant output,
> which is used internally for the Euler integration. `plant_->measuredOut`
> holds the delayed output that the controller should read.

---

## Function Reference

### `plant_Init`

```c
PlantStatus plant_Init(Plant_t *plant_, configPlant *config_);
```

Validates all configuration parameters, allocates the input and output delay
buffers, pre-fills them with the initial conditions, and seeds the random noise
generator. Must be called once before any other function.

Returns `PLANT_STATUS_ERROR` if any parameter is invalid, or
`PLANT_STATUS_NULL_POINTER` if a pointer is NULL or memory allocation fails.

---

### `plant_Update`

```c
PlantStatus plant_Update(Plant_t *plant_, float input_, float disturbance_, float difTime_);
```

Advances the plant by one sample. The sequence of operations is:

1. Clamp `input_` to `[minIn, maxIn]`.
2. Push `input_` into the input delay buffer; retrieve the delayed input.
3. Compute the new output using the Euler forward equation.
4. Add random noise bounded by `noiseAmp`.
5. Clamp the output to `[minOut, maxOut]`.
6. Update `outPrev` with the true output.
7. Push the true output into the output delay buffer; store the delayed result in `measuredOut`.

| Parameter      | Description                                              |
|----------------|----------------------------------------------------------|
| `input_`       | Plant input (e.g. valve opening percentage).             |
| `disturbance_` | External disturbance (e.g. thermal load variation).      |
| `difTime_`     | Elapsed time since the last call [s]. Must be > 0.       |

---

### `plant_Reset`

```c
PlantStatus plant_Reset(Plant_t *plant_);
```

Restores all internal state variables and delay buffers to their initial
conditions without releasing or reallocating memory. Useful for restarting a
simulation without reconfiguring the plant.

---

### `plant_Deinit`

```c
PlantStatus plant_Deinit(Plant_t *plant_);
```

Frees the memory allocated for both delay buffers and marks the plant as
uninitialized. The `Plant_t` struct can be reused with a new `plant_Init` call
after this.

> Always call `plant_Deinit` before discarding a plant instance to avoid memory leaks.

---

## Return Codes

All functions return a `PlantStatus` value:

| Code                       | Meaning                                              |
|----------------------------|------------------------------------------------------|
| `PLANT_STATUS_OK`          | Operation completed successfully.                    |
| `PLANT_STATUS_NULL_POINTER`| A required pointer was NULL or memory allocation failed. |
| `PLANT_STATUS_ERROR`       | An invalid parameter was provided or the plant is not initialized. |

---

## Warnings

### Numerical Stability

The Euler forward integration method is only numerically stable when the sample
period $T$ is less than twice the time constant $\tau$:

$$
T < 2\tau
$$

Violating this condition will cause the output to diverge. As a practical rule,
keep $T \leq \tau / 10$ for accurate simulation results.

### Memory Management

`plant_Init` dynamically allocates two internal buffers. Always pair every
`plant_Init` call with a corresponding `plant_Deinit` call to avoid memory leaks.