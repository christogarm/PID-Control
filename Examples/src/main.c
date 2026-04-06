/*
 * Simulacion del control PID
 *
 *  Created on: 31 mar 2026
 *      Author: Christogarm
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

#include "PID.h"
#include "Logger.h"
#include "Plant.h"

/* Prototipos de las funciones */
void TEST_LoggerInit(void);
void TEST_Plant_Init(void);
void TEST_PID_Init(void);

/* Variables Inicializadas para las Pruebas */
Logger_t hLogger = {0};
Plant_t hPlant = {0};
PID_Control hPID = {0};

float data[5] = {0};
char event[100] = "Eventos";

int main(void)
{
    /* Inicializacion de los tipos datos */
    TEST_LoggerInit();
    TEST_Plant_Init();
    TEST_PID_Init();

    char element[10];
    char msg[100];

    /* Comenzamos aqui la Prueba de la Libreria PID */
    for (size_t i = 0; i < ITERATIONS; i++)
    {

        data[inputPID] = hPID.input;
        data[outputPID] = hPID.param.maxOutput-hPID.output;
        data[Error] = hPID.error;
        data[Error_int] = hPID.integralError;
        data[Error_der] = hPID.derivativeError;

        if (plant_Update(&hPlant, hPID.param.maxOutput-hPID.output, DISTURBANCE, PERIOD) != PLANT_STATUS_OK)
        {
            printf("Error en la Actualizacion de los datos de la Planta %d", i);
            break;
        }

        if (PID_Update(&hPID, hPlant.outMeasured, PERIOD) != PID_STATUS_OK)
        {
            printf("Error en la Actualizacion de los datos del PID %d", i);
            break;
        }

        logData(&hLogger, i);
    }

    system("pause"); // Pausamos el programa antes de que termine
    return 0;
}

/* Funciones Desarrolladas */
void TEST_LoggerInit(void)
{
    char *encabezadoData[] = {
        "INPUT PID",
        "OUTPUT PID",
        "ERROR",
        "ERROR INTEGRAL",
        "ERROR DERIVATIVO"};

    char *encabezadoEvent[] = {
        "DESCRIPCION"};

    configLogger_t dataLogger_ = {
        .filename = "dataLogger.csv",
        .dataType = LOG_FLOAT,
        .size = 5,
        .dataPoint = data,
        .pEncabezado = encabezadoData};

    configLogger_t eventLogger_ = {
        .filename = "eventLogger.csv",
        .dataType = LOG_STRING,
        .size = 1,
        .dataPoint = event,
        .pEncabezado = encabezadoEvent};

    if (loggerInit(&hLogger, &dataLogger_, &eventLogger_) != LOGGER_STATUS_OK)
    {
        printf("Error en la Configuracion del Logger");
        while (1)
            ;
    }
}

void TEST_Plant_Init(void)
{
    configPlant configPlant_ = {
        .K = -0.2,                     // Ganancia de la Entrada
        .tau = 60,                    // Constante del tiempo del Sistema
        .initInput = OUTPUT_PID_INIT, // Condicion Inicial de la entrada de la Planta
        .initOutput = INPUT_PID_INIT, // Condicion Inicial de la salida de la Planta
        .minOut = -5,
        .maxOut = 25, // Saturacion de la salida de la Planta
        .minIn = OUTPUT_PID_MIN,
        .maxIn = OUTPUT_PID_MAX, // Saturacion de la entrada de la entrada
        .noiseAmpIN = 0.1,         // Amplitud del Ruido de la entrada
        .noiseAmpOUT = 0.2,        // Amplitud del Ruido de la entrada
        .delaySamplesIN = 10,     // Retraso de la entrada del sistema
        .delaySamplesOUT = 10,    // Retraso de la entrada del sistema
    };
    if (plant_Init(&hPlant, &configPlant_) != PLANT_STATUS_OK)
    {
        printf("Error en la Configuracion de la Planta");
        while (1)
            ;
    }
}

void TEST_PID_Init(void)
{
    PID_Parameters PID_config_ = {
        .setPoint = 10,
        .deadband = 0.1,
        .Kp = 60,
        .Ki = 2,
        .Kd = 10,
        .alpha = 0.7,
        .rateLimit = 20,
        .minOutput = OUTPUT_PID_MIN,
        .maxOutput = OUTPUT_PID_MAX,
        .initOutput = OUTPUT_PID_INIT,
        .initInput = INPUT_PID_INIT,
        .clampIntMin = -35,
        .clampIntMax = 35,
        .dt = PERIOD};

    if (PID_Init(&hPID, &PID_config_) != PID_STATUS_OK)
    {
        printf("Error en la Configuracion del PID");
        while (1)
            ;
    }
}