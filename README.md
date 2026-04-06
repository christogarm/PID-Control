# PID Controller — SIL Simulation Framework

A modular C library suite for developing, testing, and validating PID control algorithms using **Software-in-the-Loop (SIL)** simulation on a PC. The framework couples a configurable PID controller with a first-order plant model and a CSV data logger, enabling full closed-loop validation before deploying to a target microcontroller.

---

## Table of Contents

- [Repository Structure](#repository-structure)
- [Modules](#modules)
- [System Architecture](#system-architecture)
- [Quick Start](#quick-start)
- [Dependencies](#dependencies)
- [Documentation](#documentation)

---

## Repository Structure

```
.
├── PID/                  # PID controller library
│   ├── PID.h
│   └── PID.c
│
├── Plant/                # First-order plant simulator
│   ├── Plant.h
│   ├── Plant.c
│   └── README.md
│
├── Logger/               # CSV data and event logger
│   ├── Logger.h
│   ├── Logger.c
│   └── README.md
│
└── Examples/             # Closed-loop SIL simulation example
    ├── main.h
    └── main.c
```

---

## Modules

### PID
Closed-loop PID controller with derivative kick prevention, conditional anti-windup, EMA derivative filter, rate limiter, and deadband. Accepts the real sample period `dt` on every call, making it suitable for both fixed and variable sample rate environments.

### Plant
Discrete-time first-order system simulator derived from Euler's forward method. Models input and output transport delay via circular FIFO buffers, additive output noise, and input/output saturation. Originally designed to simulate a refrigeration evaporator (electronic expansion valve → superheat), but generic enough for any first-order process.

### Logger
Lightweight CSV logging library for SIL environments. Provides two independent channels — a numerical data channel and a string event channel — each writing to its own CSV file with a header row. Supports `int`, `float`, `double`, and `string` data types.

### Examples
A complete closed-loop simulation that wires all three modules together. Runs 1000 cycles at a 1-second sample period with a constant disturbance, logging PID input, output, and all three error terms to CSV on every cycle.

---

## System Architecture

```
                         ┌─────────────────────────────────┐
                         │          Examples/main.c        │
                         │                                 │
     setPoint ──────────►│                                 │
                         │   ┌─────────┐    ┌──────────┐   │
                         │   │   PID   │u   │  Plant   │   │
                         │   │         ├───►│          │   │
                         │   │         │    │          │   │
                         │   └────▲────┘    └────┬─────┘   │
                         │        │  measured     │        │
                         │        └───────────────┘        │
                         │                                 │
                         │   disturbance ──────────────►   │
                         │                                 │
                         │   ┌──────────────────────────┐  │
                         │   │         Logger           │  │
                         │   │  dataLogger.csv          │  │
                         │   │  eventLogger.csv         │  │
                         │   └──────────────────────────┘  │
                         └─────────────────────────────────┘
```

Each cycle the PID output drives the plant, the plant returns a measured value (with configurable delay and noise), and the measured value feeds back into the PID. All signals are written to CSV after every iteration.

---

## Dependencies

- Standard C library only (`stdio.h`, `stdlib.h`, `string.h`, `math.h`).
- No external libraries or RTOS required.
- Designed and tested on PC (Windows / Linux). Not intended for direct deployment on a microcontroller.

---

## Documentation

Each module has its own README with full API reference, configuration tables, and design notes:

| Module | README |
|---|---|
| PID | [`PID/README.md`](PID/README.md) |
| Plant | [`Plant/README.md`](Plant/README.md) |
| Logger | [`Logger/README.md`](Logger/README.md) |
| Examples | [`Examples/README.md`](Examples/README.md) |
