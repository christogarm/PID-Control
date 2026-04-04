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

float data[4] = {0};
char event[100] = "Eventos";

int main(void)
{
    /* Inicializacion de los tipos datos */
    TEST_LoggerInit();
    TEST_PID_Init();
    TEST_Plant_Init();

    char element[10];
    char msg[100];

    /* Comenzamos aqui la Prueba de la Libreria PID */
    for (size_t i = 0; i < ITERATIONS; i++)
    {
        if(plant_Update(&hPlant, INPUT_PLANT_INIT, 0, 0.01f) != PLANT_STATUS_OK){
            printf("Error en la Actualizacion de los datos de la Planta %d",i);
            break;
        }

        data[input] = hPlant.in;
        data[output] = hPlant.outMeasured;

        logData(&hLogger, i);
    }

    system("pause"); // Pausamos el programa antes de que termine
    return 0;
}

/* Funciones Desarrolladas */
void TEST_LoggerInit(void)
{
    char *encabezadoData[] = {
        "INPUT",
        "OUTPUT",
        "SET POINT",
        "ERROR"};

    char *encabezadoEvent[] = {
        "DESCRIPCION"};

    configLogger_t dataLogger_ = {
        .filename = "dataLogger.csv",
        .dataType = LOG_FLOAT,
        .size = 4,
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
        .K = -5,          // Ganancia de la Entrada
        .tau = 4,         // Constante del tiempo del Sistema
        .initInput = 0,  // Condicion Inicial de la entrada de la Planta
        .initOutput = 0, // Condicion Inicial de la salida de la Planta
        .minOut = -100,
        .maxOut = 15, // Saturacion de la salida de la Planta
        .minIn = 0,
        .maxIn = 100,         // Saturacion de la entrada de la entrada
        .noiseAmpIN = 0,      // Amplitud del Ruido de la entrada
        .noiseAmpOUT = 0,      // Amplitud del Ruido de la entrada
        .delaySamplesIN = 1,  // Retraso de la entrada del sistema
        .delaySamplesOUT = 1, // Retraso de la entrada del sistema
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
}