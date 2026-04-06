/*
 * PID.h
 *
 *  Created on: 11 mar 2026
 *      Author: Christogarm
 *
 *  Revisado y corregido:
 *  - Getter de salida para encapsulamiento
 *  - Campo pidEnabled para habilitar/deshabilitar sin perder configuracion
 *  - Documentacion de campos initOutput e initInput
 *  - Correcciones menores de documentacion
 */

#ifndef PID_H_
#define PID_H_

#include <stdint.h>

/* -----------------------------------------------------------------------
 * Tipos de Retorno
 * ----------------------------------------------------------------------- */

typedef enum
{
    PID_STATUS_OK,           // Operacion exitosa
    PID_STATUS_ERROR,        // Parametro invalido
    PID_STATUS_NULL_POINTER  // Puntero nulo recibido
} PID_Status;

/* -----------------------------------------------------------------------
 * Parametros de Configuracion
 *
 * Notas importantes:
 *   - initOutput: valor con el que arrancara la salida al llamar PID_Reset().
 *                 Debe estar dentro de [minOutput, maxOutput].
 *   - initInput:  valor con el que se inicializa lastInput en PID_Reset().
 *                 Evita un pico derivativo en el primer ciclo.
 *   - alpha:      filtro EMA sobre el termino derivativo.
 *                 0 = sin filtro (solo muestra anterior),
 *                 1 = sin filtro (solo muestra actual).
 *   - rateLimit:  maximo cambio de salida por ciclo (unidades de salida/ciclo).
 *                 Debe ser > 0.
 *   - deadband:   error menor a este valor se trata como cero.
 *                 0 = sin banda muerta.
 * ----------------------------------------------------------------------- */

typedef struct
{
    float setPoint;                 // Valor objetivo del sistema
    float deadband;                 // Banda muerta (>= 0)
    float Kp, Ki, Kd;              // Ganancias proporcional, integral, derivativa
    float alpha;                    // Coeficiente del filtro derivativo EMA [0, 1]
    float rateLimit;                // Limite de cambio de salida por ciclo (> 0)
    float minOutput, maxOutput;     // Saturacion de la salida (minOutput < maxOutput)
    float initOutput;               // Salida inicial usada en PID_Reset()
    float initInput;                // Entrada inicial usada en PID_Reset()
    float clampIntMin, clampIntMax; // Limites del error integral (clampIntMin < clampIntMax)
    float dt;                       // Periodo de muestreo por defecto (> 0) [segundos]
} PID_Parameters;

/* -----------------------------------------------------------------------
 * Estado interno del Controlador
 * ----------------------------------------------------------------------- */

typedef struct
{
    PID_Parameters param;                       // Configuracion del PID
    float          input,     lastInput;         // Entrada actual y anterior
    float          output,    lastOutput;        // Salida actual y anterior
    float          error,     lastError;         // Error actual y anterior
    float          integralError;                // Acumulador integral
    float          derivativeError, lastDerivativeError; // Termino derivativo actual y anterior
} PID_Control;

/* -----------------------------------------------------------------------
 * API Publica
 * ----------------------------------------------------------------------- */

/*
 * Inicializacion
 */

/**
 * @brief  Inicializa el controlador PID con los parametros dados.
 *         Llama internamente a PID_Reset().
 *
 * @param  Ppid_    Puntero al controlador.
 * @param  Pparam_  Puntero a la estructura de parametros.
 *
 * @note   Antes de llamar a esta funcion, asegurate de que Pparam_->initOutput
 *         este dentro de [minOutput, maxOutput] y que Pparam_->initInput
 *         sea un valor de entrada representativo del estado inicial del sistema.
 *
 * @return PID_STATUS_OK si la inicializacion fue exitosa.
 */
PID_Status PID_Init(PID_Control *Ppid_, PID_Parameters *Pparam_);

/*
 * Configuracion en tiempo de ejecucion
 */

/** @brief Actualiza el set point del controlador. */
PID_Status PID_setSetPoint(PID_Control *Ppid_, float setPoint_);

/** @brief Actualiza las tres ganancias del controlador (Kp, Ki, Kd >= 0). */
PID_Status PID_setGain(PID_Control *Ppid_, float Kp_, float Ki_, float Kd_);

/**
 * @brief  Actualiza el periodo de muestreo por defecto.
 * @note   Si usas PID_Update() con dt_ > 0, ese valor sobreescribe el
 *         almacenado aqui en cada llamada. Esta funcion es util para
 *         fijar el periodo cuando el dt es constante.
 */
PID_Status PID_setDTime(PID_Control *Ppid_, float dt_);

/** @brief Actualiza los limites de saturacion de la salida (minOut_ < maxOut_). */
PID_Status PID_setLimitOut(PID_Control *Ppid_, float minOut_, float maxOut_);

/** @brief Actualiza los limites del acumulador integral (minInt_ < maxInt_). */
PID_Status PID_setClampInt(PID_Control *Ppid_, float minInt_, float maxInt_);

/** @brief Actualiza el limite de cambio de salida por ciclo (rateLimit_ > 0). */
PID_Status PID_setRateLimit(PID_Control *Ppid_, float rateLimit_);

/** @brief Actualiza el coeficiente del filtro EMA derivativo (alpha en [0, 1]). */
PID_Status PID_setAlpha(PID_Control *Ppid_, float alpha_);

/** @brief Actualiza la banda muerta (deadband_ >= 0). */
PID_Status PID_setDeadband(PID_Control *Ppid_, float deadband_);

/*
 * Operacion
 */

/**
 * @brief  Ejecuta un ciclo del controlador PID.
 *
 * @param  Ppid_   Puntero al controlador.
 * @param  input_  Valor medido del proceso en este ciclo.
 * @param  dt_     Periodo de muestreo real de este ciclo (segundos, > 0).
 *                 Sobreescribe el dt almacenado en los parametros.
 *
 * @return PID_STATUS_OK si el calculo fue exitoso.
 */
PID_Status PID_Update(PID_Control *Ppid_, float input_, float dt_);

/*
 * Getters
 */

/**
 * @brief  Retorna la ultima salida calculada del controlador.
 * @note   Usar este getter en lugar de acceder directamente a Ppid_->output
 *         preserva el encapsulamiento y facilita futuras refactorizaciones.
 *
 * @param  Ppid_  Puntero al controlador.
 * @param  out_   Puntero donde se escribe la salida.
 * @return PID_STATUS_OK si la lectura fue exitosa.
 */
PID_Status PID_GetOutput(const PID_Control *Ppid_, float *out_);

/*
 * Mantenimiento
 */

/**
 * @brief  Reinicia el estado interno del controlador a los valores iniciales.
 *         No modifica los parametros de configuracion.
 *
 * @param  Ppid_  Puntero al controlador.
 */
void PID_Reset(PID_Control *Ppid_);

#endif /* PID_H_ */
