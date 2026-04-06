/*
 * Simulacion del control PID
 *
 *  Created on: 31 mar 2026
 *      Author: Christogarm
 */

#ifndef MAIN_H
#define MAIN_H

/* Macros o enums*/
#define ITERATIONS                  1000

#define OUTPUT_PID_MAX              100
#define OUTPUT_PID_MIN              0

#define OUTPUT_PID_INIT             100
#define INPUT_PID_INIT              12

#define PERIOD                      1
#define DISTURBANCE                 20

typedef enum
{
    inputPID,
    outputPID,
    Error,
    Error_int,
    Error_der
} dataInfoLogger;

#endif
