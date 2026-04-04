/*
 * Logger tiene la funcion de Grabar Eventos y Datos en dos archivos distintos
 * DataLog.csv      ->  Se graban datos de interes
 * EventosLog.csv   ->  Se graban Eventos como errores, advertencias o informacion
 */

#ifndef LOGGER_LOGGER_H_
#define LOGGER_LOGGER_H_

#include <stdint.h>
#include <stdio.h>

typedef enum
{
    LOGGER_STATUS_OK,
    LOGGER_STATUS_NULL_POINTER,
    LOGGER_STATUS_ERROR
} LoggerStatus;

typedef enum
{
    LOG_INT,
    LOG_FLOAT,
    LOG_DOUBLE,
    LOG_STRING,
} loggerDataType_t;

typedef struct
{
    const char *filename;      // Nombre del Archivo
    FILE *fileLogger;          // Archivo
    loggerDataType_t dataType; // Apunta un buffer de los tipos de Datos a utilizar
    size_t size;             // Cantidad de Datos que se cargaran en una sola fila
    void *dataPoint;           // Apuntador de datos que se almacenaran
    char **pEncabezado;        // Apuntador para escribir el encabezado de la tabla de Datos
} configLogger_t;

typedef struct
{
    configLogger_t dataConfig;  // Configuracion del Logger de Datos
    configLogger_t eventConfig; // Configuracion del Logger de Eventos
} Logger_t;

LoggerStatus loggerInit(Logger_t *logger_, configLogger_t *dataLogger_, configLogger_t *eventLogger_); // Configuracion del Logger
LoggerStatus logData(Logger_t *logger_, uint64_t time_);                                                               // Logging de Datos
LoggerStatus logEvent(Logger_t *logger_, uint64_t time_);                                                              // Logging de Eventos
LoggerStatus loggerDeInit(Logger_t *logger_);

#endif