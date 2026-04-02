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
void TEST_PID_Init(void);
void TEST_Plant_Init(void);

/* Variables Inicializadas para las Pruebas */
Logger_t hLogger = {0};
float data[4] = {0, 1, 2, 3};
char event[100] = "Eventos";

float inputInit = 20; // Entrada Inicial para estimular la Planta

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

void TEST_PID_Init(void)
{
}

void TEST_Plant_Init(void)
{
}