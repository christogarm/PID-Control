# Logger API

A lightweight C logging library designed for **Software-in-the-Loop (SIL)** testing of control algorithms — such as PID controllers — running on a PC. It records data signals and system events into separate CSV files, making it easy to analyze and visualize algorithm behavior after a simulation run.

---

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [File Structure](#file-structure)
- [Data Types](#data-types)
- [API Reference](#api-reference)
- [Usage Example](#usage-example)
- [CSV Output Format](#csv-output-format)
- [Return Codes](#return-codes)
- [Notes](#notes)

---

## Overview

The Logger API separates logging into two independent channels:

| Channel | Default File | Purpose |
|---|---|---|
| **Data Logger** | `DataLog.csv` | Records numerical signals per simulation step (e.g. setpoint, output, error, control action) |
| **Event Logger** | `EventosLog.csv` | Records timestamped messages such as warnings, errors, or state changes |

Both channels share the same internal engine and write CSV files with a header row followed by timestamped data rows.

---

## Features

- Two independent logging channels: data and events
- Supports `int`, `float`, `double`, and `string` data types
- CSV output with auto-generated column headers
- File handle kept open during the session for efficient sequential writes
- Simple lifecycle: `Init` → `Log` → `DeInit`
- Minimal dependencies — standard C library only (`stdio.h`, `stdlib.h`, `string.h`)

---

## File Structure

```
.
├── Logger.h    # Public API — types, structs, and function declarations
└── Logger.c    # Implementation
```

---

## Data Types

The following types are supported for the data array pointed to by `dataPoint`:

| Enum Value | C Type | Description |
|---|---|---|
| `LOG_INT` | `int[]` | Array of integers |
| `LOG_FLOAT` | `float[]` | Array of single-precision floats |
| `LOG_DOUBLE` | `double[]` | Array of double-precision floats (recommended for PID) |
| `LOG_STRING` | `const char*[]` | Array of string pointers |

---

## API Reference

### `loggerInit`
```c
LoggerStatus loggerInit(Logger_t *logger_,
                        configLogger_t *dataLogger_,
                        configLogger_t *eventLogger_);
```
Initializes the logger. Creates both CSV files, writes the header row to each, and leaves the file handles open in append mode for subsequent writes.

**Parameters:**
- `logger_` — Pointer to the `Logger_t` instance to initialize.
- `dataLogger_` — Configuration for the data logging channel.
- `eventLogger_` — Configuration for the event logging channel.

---

### `logData`
```c
LoggerStatus logData(Logger_t *logger_, uint64_t time_);
```
Writes one row to the data CSV file. The row contains the timestamp followed by all values in the configured data array.

**Parameters:**
- `logger_` — Pointer to the initialized `Logger_t` instance.
- `time_` — Simulation timestamp (in microseconds or the simulator's time unit).

---

### `logEvent`
```c
LoggerStatus logEvent(Logger_t *logger_, uint64_t time_);
```
Writes one row to the event CSV file. Intended for logging diagnostic messages such as `"[WARNING] PID output saturated"`. Works identically to `logData` but writes to the event channel.

**Parameters:**
- `logger_` — Pointer to the initialized `Logger_t` instance.
- `time_` — Timestamp of the event.

---

### `loggerDeInit`
```c
LoggerStatus loggerDeInit(Logger_t *logger_);
```
Closes both CSV files and releases their handles. Must be called at the end of the simulation to ensure all buffered data is flushed to disk.

**Parameters:**
- `logger_` — Pointer to the `Logger_t` instance to release.

---

## Usage Example

The following example shows how to log PID signals (setpoint, output, error) and a warning event during a SIL simulation loop.

```c
#include "Logger.h"
#include <stdint.h>

int main(void)
{
    /* --- PID signals to log --- */
    double pidData[3] = {0.0, 0.0, 0.0}; // [setpoint, output, error]
    char *dataHeaders[] = {"Setpoint", "Output", "Error"};

    /* --- Event messages to log --- */
    const char *eventMsg[1] = {"[INFO] Simulation started"};
    char *eventHeaders[] = {"Message"};

    /* --- Configure data channel --- */
    configLogger_t dataCfg = {
        .filename    = "dataLogger.csv",
        .dataType    = LOG_DOUBLE,
        .size        = 3,
        .dataPoint   = pidData,
        .pEncabezado = dataHeaders
    };

    /* --- Configure event channel --- */
    configLogger_t eventCfg = {
        .filename    = "eventLogger.csv",
        .dataType    = LOG_STRING,
        .size        = 1,
        .dataPoint   = eventMsg,
        .pEncabezado = eventHeaders
    };

    /* --- Initialize logger --- */
    Logger_t logger;
    if (loggerInit(&logger, &dataCfg, &eventCfg) != LOGGER_STATUS_OK)
        return -1;

    /* --- Simulation loop --- */
    for (uint64_t t = 0; t < 1000; t++)
    {
        /* Update PID signals (replace with actual algorithm output) */
        pidData[0] = 1.0;               // setpoint
        pidData[1] = 0.95 + t * 0.001;  // simulated output
        pidData[2] = pidData[0] - pidData[1]; // error

        logData(&logger, t);

        /* Log an event at a specific simulation step */
        if (t == 500)
        {
            eventMsg[0] = "[WARNING] Output approaching saturation limit";
            logEvent(&logger, t);
        }
    }

    /* --- Close files --- */
    loggerDeInit(&logger);

    return 0;
}
```

---

## CSV Output Format

**`DataLog.csv`**
```
Time,Setpoint,Output,Error
0,1.000000,0.950000,0.050000
1,1.000000,0.951000,0.049000
...
```

**`EventosLog.csv`**
```
Time,Message
0,[INFO] Simulation started
500,[WARNING] Output approaching saturation limit
```

---

## Return Codes

| Code | Value | Description |
|---|---|---|
| `LOGGER_STATUS_OK` | `0` | Operation completed successfully |
| `LOGGER_STATUS_NULL_POINTER` | `1` | A required pointer was NULL |
| `LOGGER_STATUS_ERROR` | `2` | General error (invalid size, file open failure, unknown data type) |

---

## Notes

- This library is intended for **PC-based SIL testing only**. It is not designed for use on microcontroller firmware.
- The algorithm under test (e.g. a PID controller) is the same C code that will later be deployed to the target microcontroller, which is the core principle of SIL validation.
- `loggerDeInit` should always be called before the program exits to prevent data loss from unflushed file buffers.
- For the event channel, it is recommended to prefix messages with `[INFO]`, `[WARNING]`, or `[ERROR]` to make log filtering easier in tools like Excel or Python/pandas.
