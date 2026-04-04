
#include "Logger.h"

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

static LoggerStatus logging(configLogger_t *configLogger_, uint64_t time_); // Logging gemeralizado
static LoggerStatus printEncabezado(configLogger_t *configLogger_);         // Imprime el encabezado

LoggerStatus loggerInit(Logger_t *logger_, configLogger_t *dataLogger_, configLogger_t *eventLogger_)
{
    if (logger_ == NULL || dataLogger_ == NULL || eventLogger_ == NULL)
        return LOGGER_STATUS_NULL_POINTER;

    if (dataLogger_->dataPoint == NULL || eventLogger_->dataPoint == NULL)
        return LOGGER_STATUS_NULL_POINTER;

    if (dataLogger_->size <= 0 || eventLogger_->size <= 0)
        return LOGGER_STATUS_ERROR;

    if (dataLogger_->pEncabezado == NULL || eventLogger_->pEncabezado == NULL)
        return LOGGER_STATUS_NULL_POINTER;

    /* Se crea el archivo y se escribe su encabezado */
    if (printEncabezado(dataLogger_) != LOGGER_STATUS_OK || printEncabezado(eventLogger_) != LOGGER_STATUS_OK)
        return LOGGER_STATUS_ERROR;

    logger_->dataConfig = *dataLogger_;
    logger_->eventConfig = *eventLogger_;

    return LOGGER_STATUS_OK;
}

LoggerStatus logData(Logger_t *logger_, uint64_t time_)
{
    return logging(&logger_->dataConfig, time_);
}

LoggerStatus logEvent(Logger_t *logger_, uint64_t time_)
{
    return logging(&logger_->eventConfig, time_);
}

static LoggerStatus logging(configLogger_t *configLogger_, uint64_t time_)
{
    if (configLogger_->dataPoint == NULL)
        return LOGGER_STATUS_NULL_POINTER;

    configLogger_->fileLogger = fopen(configLogger_->filename, "a");
    if (configLogger_->fileLogger == NULL)
        return LOGGER_STATUS_NULL_POINTER;

    fprintf(configLogger_->fileLogger, "%ld", time_); // Colocamos el tiempo

    /* Colocamos los datos */
    for (size_t i = 0; i < configLogger_->size; i++)
    {
        switch (configLogger_->dataType)
        {
        case LOG_INT:
            fprintf(configLogger_->fileLogger, ",%d", ((int *)(configLogger_->dataPoint))[i]);
            break;
        case LOG_FLOAT:
            fprintf(configLogger_->fileLogger, ",%f", ((float *)(configLogger_->dataPoint))[i]);
            break;
        case LOG_STRING:
            fprintf(configLogger_->fileLogger, ",%s", ((const char *)(configLogger_->dataPoint)));
            break;
        default:
            fclose(configLogger_->fileLogger); // Cerramos el Archivo
            return LOGGER_STATUS_ERROR;
            break;
        }
    }

    fprintf(configLogger_->fileLogger, "\n"); // Salto de Linea
    fclose(configLogger_->fileLogger);        // Cerramos el Archivo

    return LOGGER_STATUS_OK;
}

static LoggerStatus printEncabezado(configLogger_t *configLogger_)
{
    if (configLogger_ == NULL || configLogger_->pEncabezado == NULL)
        return LOGGER_STATUS_NULL_POINTER;

    if (configLogger_->size <= 0)
        return LOGGER_STATUS_ERROR;

    configLogger_->fileLogger = fopen(configLogger_->filename, "w"); // Abrimos el verificar que todo esta correcto
    if (configLogger_->fileLogger == NULL)
        return LOGGER_STATUS_NULL_POINTER;

    char *labelTime = "Time";
    int sizeEncabezados = strlen(labelTime); //  Etiqueta del Tiempo

    /* Calculamos la cantidad de caracteres que son en total */
    for (size_t i = 0; i < configLogger_->size; i++)
        sizeEncabezados += strlen(configLogger_->pEncabezado[i]);

    char *str = (char *)malloc(sizeEncabezados + (configLogger_->size - 1 + 1) + 1 + 1); // Sumamos el tamano, debido a que colocaremos comas entre palabra, ademas sumamos un elemento extra para que tenggamos un caracter \0 y un salto de linea

    if (str == NULL) // Si tenemmos espacio de mewmoria?
        return LOGGER_STATUS_NULL_POINTER;

    str[0] = '\0';          // inicializar string
    strcat(str, labelTime); // Colocamos el primer elemento (Etiqueta de Tiempo)

    // concatenamos el resto de datos con comas
    for (int i = 0; i < configLogger_->size; i++)
    {
        strcat(str, ",");
        strcat(str, configLogger_->pEncabezado[i]);
    }
    strcat(str, "\n");

    fprintf(configLogger_->fileLogger, "%s", str);

    fclose(configLogger_->fileLogger);
    free(str);

    return LOGGER_STATUS_OK;
}