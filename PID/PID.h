/*
 * PID.h
 *
 *  Created on: 11 mar 2026
 *      Author: Christogarm
 */

#ifndef PID_H_
#define PID_H_

#include <stdlib.h>
#include <stdint.h>

typedef enum
{
	PID_STATUS_OK,
	PID_STATUS_ERROR,
	PID_STATUS_NULL_POINTER
} PID_Status;

typedef struct
{
	float setPoint;					// Valor objetivo que el sistema intenta mantener mediante el control.
	float deadband;					// Banda Muerta para que no haya cambios de salida
	float Kp, Ki, Kd;				// Ganancias del control PID
	float alpha;					// Constante alpha para un error derivativo mas suave
	float rateLimit;				// Limite de cambios bruscos de la salida
	float minOutput, maxOutput;		// Limites de la salida del PID
	float clampIntMin, clampIntMax; // Limites de crecimiento del Error Integral
	float dt;						// intervalo de tiempo entre dos ejecuciones del controlador.

} PID_Parameters;

typedef struct
{
	PID_Parameters param;						// PID Parameters
	float lastInput, input;						// Entrada y Entrada previa del Control
	float lastOutput, output;					// Salida y Salida anterior del control PID
	float lastError, error;						// Error y Error Previo
	float integralError;						// Error Acumulado o Integral
	float lastDerivativeError, derivativeError; // Error derivativo y Error derivativo previo
} PID_Control;

/*
 * Funciones de Inicializacion del PID
 */
PID_Status PID_Init(PID_Control *Ppid_, PID_Parameters *Pparam_); // Inicializacion del control PID de forma general

/*
 * Funciones de Configuracion del Control PID
 */
PID_Status PID_setSetPoint(PID_Control *Ppid_, float setPoint_);			  // Configura el Set Point
PID_Status PID_setGain(PID_Control *Ppid_, float Kp_, float Ki_, float Kd_);  // Configura las ganancias del control PID
PID_Status PID_setDTime(PID_Control *Ppid_, float dt_);						  // Configura el Intervalo de tiempo entre dos ejecuciones
PID_Status PID_setLimitOut(PID_Control *Ppid_, float minOut_, float maxOut_); // Configuracion de los limites de la Salida del PID
PID_Status PID_setClampInt(PID_Control *Ppid_, float minInt_, float maxInt_); // Configuracion de los limites del crecimiento del error Integral
PID_Status PID_setRateLimit(PID_Control *Ppid_, float rateLimit_);			  // Configuracion del límite de cambios bruscos de la salida
PID_Status PID_setAlpha(PID_Control *Ppid_, float alpha_);					  // Configuracion de Alpha
PID_Status PID_setDeadband(PID_Control *Ppid_, float deadband_);			  // Configuracion de la Banda Muerta

/*
 * Funciones de Operacion
 */
PID_Status PID_Update(PID_Control *Ppid_, float input_, float dt_); // Actualizacion de la salida del PID

/*
 * Funciones de Mantenimiento
 */
void PID_Reset(PID_Control *Ppid_); // Limpia los errores del Control PID

#endif /* PID_H_ */
