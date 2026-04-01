/*
 * PID.c
 *
 *  Created on: 11 mar 2026
 *      Author: Christogarm
 */

#include "PID.h"

/*
 * Funciones Privadas
 */

static void PID_rateLimit(PID_Control *Ppid_); // Limitamos la salidas bruscas
static float PID_fabs(float value_);		   // Valor absoluto

/********************************************
 * Funciones de Inicializacion del PID
 ********************************************/

/*
 * Objetivo: Inicializacion del control PID, a partir de unos parametros dados
 * Parametros:	@Ppid_: Variable donde se guarda la configuracion
 * 				@Pparam_: Parametros de Configuracion
 * Retorno:		Estado del PID
 */
PID_Status PID_Init(PID_Control *Ppid_, PID_Parameters *Pparam_)
{

	if (Ppid_ == NULL || Pparam_ == NULL)
		return PID_STATUS_NULL_POINTER;
	if (Pparam_->Kp < 0 || Pparam_->Ki < 0 || Pparam_->Kd < 0)
		return PID_STATUS_ERROR;
	if (Pparam_->minOutput >= Pparam_->maxOutput)
		return PID_STATUS_ERROR;
	if (Pparam_->clampIntMin >= Pparam_->clampIntMax)
		return PID_STATUS_ERROR;
	if (Pparam_->rateLimit <= 0)
		return PID_STATUS_ERROR;
	if (Pparam_->alpha < 0 || Pparam_->alpha > 1)
		return PID_STATUS_ERROR;
	if (Pparam_->dt <= 0)
		return PID_STATUS_ERROR;

	Ppid_->param = *Pparam_; // Configuracion de los parametros

	PID_Reset(Ppid_); // Iguala a 0 los valores de los Errores

	return PID_STATUS_OK;
}

/********************************************
 * Funciones de Configuracion del Control PID
 ********************************************/

/*
 * Objetivo: Configuracion del Set Point
 * Parametros:	@Ppid_: Variable donde se guarda la configuracion
 * 				@setPoint_: set Point Deseado
 * Retorno:		Estado del PID
 */
PID_Status PID_setSetPoint(PID_Control *Ppid_, float setPoint_)
{
	if (Ppid_ == NULL)
		return PID_STATUS_NULL_POINTER;

	Ppid_->param.setPoint = setPoint_;

	return PID_STATUS_OK;
}

/*
 * Objetivo: Configuracion de las Ganancias
 * Parametros:	@Ppid_: Variable donde se guarda la configuracion
 * 				@Kp_: Ganancia Proporcional
 * 				@Ki_: Ganancia Integral
 * 				@Kd_: Ganancia Derivativa
 * Retorno:		Estado del PID
 */
PID_Status PID_setGain(PID_Control *Ppid_, float Kp_, float Ki_, float Kd_)
{
	if (Ppid_ == NULL)
		return PID_STATUS_NULL_POINTER;
	if (Kp_ < 0 || Ki_ < 0 || Kd_ < 0)
		return PID_STATUS_ERROR;

	Ppid_->param.Kp = Kp_;
	Ppid_->param.Ki = Ki_;
	Ppid_->param.Kd = Kd_;

	return PID_STATUS_OK;
}

/*
 * Objetivo: Configuracion del Intervalo de Tiempo
 * Parametros:	@Ppid_: Variable donde se guarda la configuracion
 * 				@dt_: Intervalo de Tiempo
 * Retorno:		Estado del PID
 */
PID_Status PID_setDTime(PID_Control *Ppid_, float dt_)
{
	if (Ppid_ == NULL)
		return PID_STATUS_NULL_POINTER;
	if (dt_ <= 0)
		return PID_STATUS_ERROR;

	Ppid_->param.dt = dt_;

	return PID_STATUS_OK;
}

/*
 * Objetivo: Configuracion del Intervalo Mínimo y Máximo de la salida
 * Parametros:	@Ppid_: Variable donde se guarda la configuracion
 * 				@minOut_: Mínimo de la Salida
 * 				@maxOut_: Máximo de la Salida
 * Retorno:		Estado del PID
 */
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

/*
 * Objetivo: Configuracion de los limites del Error Integral
 * Parametros:	@Ppid_: Variable donde se guarda la configuracion
 * 				@minInt_: Mínimo del Error Integral
 * 				@maxInt_: Máximo del Error Integral
 * Retorno:		Estado del PID
 */
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

/*
 * Objetivo: Configuracion del límite de los cambios abruptos de la salida
 * Parametros:	@Ppid_: Variable donde se guarda la configuracion
 * 				@rateLimit_: Límite de los cambios de la salida
 * Retorno:		Estado del PID
 */
PID_Status PID_setRateLimit(PID_Control *Ppid_, float rateLimit_)
{
	if (Ppid_ == NULL)
		return PID_STATUS_NULL_POINTER;
	if (rateLimit_ <= 0)
		return PID_STATUS_ERROR;

	Ppid_->param.rateLimit = rateLimit_;

	return PID_STATUS_OK;
}

/*
 * Objetivo: Configuracion del alpha para filtro del Error Derivativo
 * Parametros:	@Ppid_: Variable donde se guarda la configuracion
 * 				@alpha_: Valor de Alpha
 * Retorno:		Estado del PID
 */
PID_Status PID_setAlpha(PID_Control *Ppid_, float alpha_)
{
	if (Ppid_ == NULL)
		return PID_STATUS_NULL_POINTER;
	if (alpha_ < 0 || alpha_ > 1)
		return PID_STATUS_ERROR;

	Ppid_->param.alpha = alpha_;

	return PID_STATUS_OK;
}

/*
 * Objetivo: Configuracion de la banda Muerta
 * Parametros:	@Ppid_: Variable donde se guarda la configuracion
 * 				@alpha_: Valor de Banda Muerta
 * Retorno:		Estado del PID
 */
PID_Status PID_setDeadband(PID_Control *Ppid_, float deadband_)
{
	if (Ppid_ == NULL)
		return PID_STATUS_NULL_POINTER;

	if (deadband_ < 0)
		return PID_STATUS_ERROR;

	Ppid_->param.deadband = deadband_;

	return PID_STATUS_OK;
}

/********************************************
 * Funciones de Operacion
 ********************************************/

/*
 * Objetivo: Calculo de la salida del control PID
 * Parametros:	@Ppid_: Variable donde se obtiene los valores para el calculo de la salida
 * 				@input_: Entrada del Sistema
 * 				@dt_: Intervalo de tiempos que se esya ejecutando el PID.
 * Retorno:		Salida del Control
 */
PID_Status PID_Update(PID_Control *Ppid_, float input_, float dt_)
{
	if (Ppid_ == NULL)
		return PID_STATUS_NULL_POINTER;

	if (dt_ <= 0)
		;
	return PID_STATUS_ERROR;

	Ppid_->param.dt = dt_; // Guardamos el periodo de Tiempo
	Ppid_->input = input_; // Guardamos la Entrada

	Ppid_->error = Ppid_->param.setPoint - Ppid_->input; // Error

	/* Banda Muerta para que no intervenga */
	if (PID_fabs(Ppid_->error) < Ppid_->param.deadband)
		Ppid_->error = 0;

	if (!((Ppid_->output >= Ppid_->param.maxOutput && Ppid_->error > 0) || // Solo generamos el error integral, cuando la salida no esta saturada y no incremente el error integral
		  (Ppid_->output <= Ppid_->param.minOutput && Ppid_->error < 0)))
		Ppid_->integralError += Ppid_->error * Ppid_->param.dt; // Error Integral

	Ppid_->derivativeError = -(Ppid_->input - Ppid_->lastInput) / Ppid_->param.dt; // Error Derivativo

	/* Filtro Derivativo */
	Ppid_->derivativeError = Ppid_->param.alpha * Ppid_->derivativeError + (1 - Ppid_->param.alpha) * Ppid_->lastDerivativeError;

	/* Anti-Windup (Integral Clamp) */
	if (Ppid_->integralError > Ppid_->param.clampIntMax)
		Ppid_->integralError = Ppid_->param.clampIntMax;

	if (Ppid_->integralError < Ppid_->param.clampIntMin)
		Ppid_->integralError = Ppid_->param.clampIntMin;

	/* PID */
	Ppid_->output = (Ppid_->param.Kp * Ppid_->error) +
					(Ppid_->param.Ki * Ppid_->integralError) +
					(Ppid_->param.Kd * Ppid_->derivativeError);

	/* Limitamos una salida brusca del control PID */
	PID_rateLimit(Ppid_);

	/* Saturación de salida */
	if (Ppid_->output > Ppid_->param.maxOutput)
		Ppid_->output = Ppid_->param.maxOutput;

	if (Ppid_->output < Ppid_->param.minOutput)
		Ppid_->output = Ppid_->param.minOutput;

	/* Guardar valores anteriores */
	Ppid_->lastError = Ppid_->error;
	Ppid_->lastOutput = Ppid_->output;
	Ppid_->lastInput = Ppid_->input;
	Ppid_->lastDerivativeError = Ppid_->derivativeError;

	return PID_STATUS_OK;
}

/********************************************
 * Funciones de Mantenimiento
 ********************************************/

/*
 * Objetivo: Reinicia los valores de los valores
 * Parametros:	@Ppid_: Variable donde se guarda la configuracion
 * 				@Pparam_: Parametros de Configuracion
 * Retorno:		Estado del
 */
void PID_Reset(PID_Control *Ppid_)
{
	if (Ppid_ == NULL)
		return;

	Ppid_->output = 0;	   // Borramos la salida
	Ppid_->lastOutput = 0; // Borramos la salida Previa
	Ppid_->input = 0;	   // Borramos la entrada
	Ppid_->lastInput = 0;

	Ppid_->error = 0;
	Ppid_->lastError = 0;
	Ppid_->integralError = 0;
	Ppid_->derivativeError = 0;
	Ppid_->lastDerivativeError = 0;
}

/********************************************
 * Funciones Pivadas
 ********************************************/
/*
 * Objetivo: Otorga una salida mas suave del Control PID
 * Parametros:	@Ppid_: Variable donde se guarda la configuracion
 * Retorno:		Vacio
 */
static void PID_rateLimit(PID_Control *Ppid_)
{
	float delta = Ppid_->output - Ppid_->lastOutput;

	if (delta > Ppid_->param.rateLimit) // Aumentas mas del limite de la salida?
		Ppid_->output = Ppid_->lastOutput + Ppid_->param.rateLimit;

	if (delta < -Ppid_->param.rateLimit) // Disminuyes menos que el limite de la salida?
		Ppid_->output = Ppid_->lastOutput - Ppid_->param.rateLimit;
}

/*
 * Objetivo: Sacar el valor Absoluto
 * Parametros:	@value_: valor
 * Retorno:		Valor abosluto del valor dado
 */
static float PID_fabs(float value_)
{
	if (value_ < 0)
		value_ = -value_;
	return value_;
}
