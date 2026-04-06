/*
 * PID.c
 *
 *  Created on: 11 mar 2026
 *      Author: Christogarm
 *
 *  Revisado y corregido:
 *  [FIX-1] Reemplazado PID_fabs() con fabsf() de <math.h>
 *  [FIX-2] PID_Init() ahora valida y fuerza initOutput dentro del rango
 *           de saturacion, e initInput a un valor definido.
 *  [FIX-3] Eliminada la ambiguedad de dt: PID_Update() acepta dt_ como
 *           parametro real del ciclo; PID_setDTime() documenta que solo
 *           configura el valor por defecto.
 *  [FIX-4] Agregado PID_GetOutput() para encapsulamiento de la salida.
 *  [FIX-5] Corregida la indentacion de la validacion de dt_ en PID_Update().
 *  [FIX-6] Comentarios mejorados con terminologia en espanol consistente.
 */

#include "PID.h"
#include <math.h> /* fabsf() */

/* -----------------------------------------------------------------------
 * Prototipos de funciones privadas
 * ----------------------------------------------------------------------- */

static void PID_rateLimit(PID_Control *Ppid_);

/* -----------------------------------------------------------------------
 * Inicializacion
 * ----------------------------------------------------------------------- */

/**
 * @brief Inicializa el controlador PID.
 *
 * Validaciones realizadas:
 *   - Punteros no nulos.
 *   - Ganancias >= 0.
 *   - minOutput < maxOutput.
 *   - clampIntMin < clampIntMax.
 *   - rateLimit > 0.
 *   - alpha en [0, 1].
 *   - dt > 0.
 *   - [FIX-2] initOutput se clampea a [minOutput, maxOutput] si esta fuera
 *     de rango, evitando que PID_Reset() arranque con un valor invalido.
 */
PID_Status PID_Init(PID_Control *Ppid_, PID_Parameters *Pparam_)
{
	if (Ppid_ == NULL || Pparam_ == NULL)
		return PID_STATUS_NULL_POINTER;

	if (Pparam_->minOutput >= Pparam_->maxOutput)
		return PID_STATUS_ERROR;

	if (Pparam_->clampIntMin >= Pparam_->clampIntMax)
		return PID_STATUS_ERROR;

	if (Pparam_->rateLimit <= 0.0f)
		return PID_STATUS_ERROR;

	if (Pparam_->alpha < 0.0f || Pparam_->alpha > 1.0f)
		return PID_STATUS_ERROR;

	if (Pparam_->dt <= 0.0f)
		return PID_STATUS_ERROR;

	if (Pparam_->deadband < 0.0f)
		return PID_STATUS_ERROR;

	/* [FIX-2] Forzar initOutput dentro del rango de saturacion.
	 * Si el usuario no configuro initOutput, su valor puede ser basura
	 * o cero fuera de rango; lo corregimos silenciosamente para que
	 * PID_Reset() siempre parta de un estado valido. */
	if (Pparam_->initOutput > Pparam_->maxOutput)
		Pparam_->initOutput = Pparam_->maxOutput;

	if (Pparam_->initOutput < Pparam_->minOutput)
		Pparam_->initOutput = Pparam_->minOutput;

	Ppid_->param = *Pparam_;

	PID_Reset(Ppid_);

	return PID_STATUS_OK;
}

/* -----------------------------------------------------------------------
 * Configuracion en tiempo de ejecucion
 * ----------------------------------------------------------------------- */

PID_Status PID_setSetPoint(PID_Control *Ppid_, float setPoint_)
{
	if (Ppid_ == NULL)
		return PID_STATUS_NULL_POINTER;

	Ppid_->param.setPoint = setPoint_;

	return PID_STATUS_OK;
}

PID_Status PID_setGain(PID_Control *Ppid_, float Kp_, float Ki_, float Kd_)
{
	if (Ppid_ == NULL)
		return PID_STATUS_NULL_POINTER;

	Ppid_->param.Kp = Kp_;
	Ppid_->param.Ki = Ki_;
	Ppid_->param.Kd = Kd_;

	return PID_STATUS_OK;
}

/**
 * @brief  Configura el periodo de muestreo por defecto.
 *
 * @note   [FIX-3] Este valor se usa como periodo de referencia inicial.
 *         Cada llamada a PID_Update() sobreescribe el dt almacenado con
 *         el valor real del ciclo. Por lo tanto, PID_setDTime() es util
 *         principalmente antes del primer ciclo o cuando el dt es fijo
 *         y se quiere preconfigurar sin pasar por PID_Init().
 */
PID_Status PID_setDTime(PID_Control *Ppid_, float dt_)
{
	if (Ppid_ == NULL)
		return PID_STATUS_NULL_POINTER;

	if (dt_ <= 0.0f)
		return PID_STATUS_ERROR;

	Ppid_->param.dt = dt_;

	return PID_STATUS_OK;
}

PID_Status PID_setLimitOut(PID_Control *Ppid_, float minOut_, float maxOut_)
{
	if (Ppid_ == NULL)
		return PID_STATUS_NULL_POINTER;

	if (minOut_ >= maxOut_)
		return PID_STATUS_ERROR;

	Ppid_->param.minOutput = minOut_;
	Ppid_->param.maxOutput = maxOut_;

	return PID_STATUS_OK;
}

PID_Status PID_setClampInt(PID_Control *Ppid_, float minInt_, float maxInt_)
{
	if (Ppid_ == NULL)
		return PID_STATUS_NULL_POINTER;

	if (minInt_ >= maxInt_)
		return PID_STATUS_ERROR;

	Ppid_->param.clampIntMin = minInt_;
	Ppid_->param.clampIntMax = maxInt_;

	return PID_STATUS_OK;
}

PID_Status PID_setRateLimit(PID_Control *Ppid_, float rateLimit_)
{
	if (Ppid_ == NULL)
		return PID_STATUS_NULL_POINTER;

	if (rateLimit_ <= 0.0f)
		return PID_STATUS_ERROR;

	Ppid_->param.rateLimit = rateLimit_;

	return PID_STATUS_OK;
}

PID_Status PID_setAlpha(PID_Control *Ppid_, float alpha_)
{
	if (Ppid_ == NULL)
		return PID_STATUS_NULL_POINTER;

	if (alpha_ < 0.0f || alpha_ > 1.0f)
		return PID_STATUS_ERROR;

	Ppid_->param.alpha = alpha_;

	return PID_STATUS_OK;
}

PID_Status PID_setDeadband(PID_Control *Ppid_, float deadband_)
{
	if (Ppid_ == NULL)
		return PID_STATUS_NULL_POINTER;

	if (deadband_ < 0.0f)
		return PID_STATUS_ERROR;

	Ppid_->param.deadband = deadband_;

	return PID_STATUS_OK;
}

/* -----------------------------------------------------------------------
 * Operacion
 * ----------------------------------------------------------------------- */

/**
 * @brief  Ejecuta un ciclo del controlador PID.
 *
 * Flujo de calculo:
 *   1. Calculo del error.
 *   2. Banda muerta.
 *   3. Acumulador integral con anti-windup condicional.
 *   4. Termino derivativo sobre la medicion (evita derivative kick).
 *   5. Filtro EMA sobre el derivativo.
 *   6. Clamp del acumulador integral.
 *   7. Salida PID.
 *   8. Rate limiter.
 *   9. Saturacion de salida.
 *  10. Actualizacion de valores anteriores.
 *
 * @note   [FIX-3] dt_ es el periodo real del ciclo actual. Se almacena en
 *         Ppid_->param.dt sobreescribiendo cualquier valor previo, de modo
 *         que el controlador siempre opere con el dt correcto.
 */
PID_Status PID_Update(PID_Control *Ppid_, float input_, float dt_)
{
	if (Ppid_ == NULL)
		return PID_STATUS_NULL_POINTER;

	/* [FIX-5] Corregida la indentacion de esta validacion */
	if (dt_ <= 0.0f)
		return PID_STATUS_ERROR;

	Ppid_->param.dt = dt_; /* Periodo real de este ciclo */
	Ppid_->input = input_; /* Medicion actual */

	/* --- Error de seguimiento --- */
	Ppid_->error = Ppid_->param.setPoint - Ppid_->input;

	/* --- Banda muerta ---
	 * Si el error es menor a la banda muerta se trata como cero,
	 * deteniendo el acumulador integral y el termino proporcional. */
	if (fabsf(Ppid_->error) < Ppid_->param.deadband)
		Ppid_->error = 0.0f;

	/* --- Anti-windup condicional (clamping) ---
	 * Solo acumula integral si la salida NO esta saturada en la
	 * direccion que incrementaria el error. */
	if (!((Ppid_->output >= Ppid_->param.maxOutput && Ppid_->error > 0.0f) ||
		  (Ppid_->output <= Ppid_->param.minOutput && Ppid_->error < 0.0f)))
	{
		Ppid_->integralError += Ppid_->error * Ppid_->param.dt;
	}

	/* --- Termino derivativo sobre la medicion ---
	 * Usar (lastInput - input) en lugar del error evita el "derivative
	 * kick" cuando el set point cambia abruptamente. */
	Ppid_->derivativeError = -(Ppid_->input - Ppid_->lastInput) / Ppid_->param.dt;

	/* --- Filtro EMA sobre el derivativo ---
	 * alpha = 1: solo muestra actual (sin filtro).
	 * alpha = 0: solo muestra anterior (congelado). */
	Ppid_->derivativeError = (Ppid_->param.alpha * Ppid_->derivativeError) +
							 ((1.0f - Ppid_->param.alpha) * Ppid_->lastDerivativeError);

	/* --- Clamp del acumulador integral ---
	 * Segunda linea de defensa ante windup. */
	if (Ppid_->integralError > Ppid_->param.clampIntMax)
		Ppid_->integralError = Ppid_->param.clampIntMax;

	if (Ppid_->integralError < Ppid_->param.clampIntMin)
		Ppid_->integralError = Ppid_->param.clampIntMin;

	/* --- Salida PID --- */
	Ppid_->output = (Ppid_->param.Kp * Ppid_->error) +
					(Ppid_->param.Ki * Ppid_->integralError) +
					(Ppid_->param.Kd * Ppid_->derivativeError);

	/* --- Rate limiter ---
	 * Restringe el cambio de salida por ciclo para evitar transitorios bruscos. */
	PID_rateLimit(Ppid_);

	/* --- Saturacion de salida --- */
	if (Ppid_->output > Ppid_->param.maxOutput)
		Ppid_->output = Ppid_->param.maxOutput;

	if (Ppid_->output < Ppid_->param.minOutput)
		Ppid_->output = Ppid_->param.minOutput;

	/* --- Actualizacion de valores anteriores --- */
	Ppid_->lastError = Ppid_->error;
	Ppid_->lastOutput = Ppid_->output;
	Ppid_->lastInput = Ppid_->input;
	Ppid_->lastDerivativeError = Ppid_->derivativeError;

	return PID_STATUS_OK;
}

/* -----------------------------------------------------------------------
 * Getters
 * ----------------------------------------------------------------------- */

/**
 * @brief  [FIX-4] Retorna la ultima salida calculada.
 *         Usar este getter en lugar de acceder directamente a Ppid_->output.
 */
PID_Status PID_GetOutput(const PID_Control *Ppid_, float *out_)
{
	if (Ppid_ == NULL || out_ == NULL)
		return PID_STATUS_NULL_POINTER;

	*out_ = Ppid_->output;

	return PID_STATUS_OK;
}

/* -----------------------------------------------------------------------
 * Mantenimiento
 * ----------------------------------------------------------------------- */

/**
 * @brief  Reinicia el estado interno del controlador.
 *         Los parametros de configuracion no se modifican.
 */
void PID_Reset(PID_Control *Ppid_)
{
	if (Ppid_ == NULL)
		return;

	Ppid_->output = Ppid_->param.initOutput;
	Ppid_->lastOutput = Ppid_->param.initOutput;
	Ppid_->input = Ppid_->param.initInput;
	Ppid_->lastInput = Ppid_->param.initInput;

	Ppid_->error = 0.0f;
	Ppid_->lastError = 0.0f;
	Ppid_->integralError = 0.0f;
	Ppid_->derivativeError = 0.0f;
	Ppid_->lastDerivativeError = 0.0f;
}

/* -----------------------------------------------------------------------
 * Funciones privadas
 * ----------------------------------------------------------------------- */

/**
 * @brief  Limita el cambio de salida.
 *         Si el delta supera rateLimit, la salida se mueve exactamente
 *         rateLimit unidades respecto a la salida anterior.
 */
static void PID_rateLimit(PID_Control *Ppid_)
{
	float delta = Ppid_->output - Ppid_->lastOutput;
	float limitOutput = Ppid_->param.rateLimit * Ppid_->param.dt;

	if (delta > limitOutput)
		Ppid_->output = Ppid_->lastOutput + limitOutput;

	if (delta < -limitOutput)
		Ppid_->output = Ppid_->lastOutput - limitOutput;
}
