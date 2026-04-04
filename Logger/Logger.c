
#include "Logger.h"

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

/* Prototipos de las Funciones Privadas */
static LoggerStatus logging(configLogger_t *configLogger_, uint64_t time_); // Logging gemeralizado
static LoggerStatus printEncabezado(configLogger_t *configLogger_);         // Imprime el encabezado

/*********************************************
*   Funciones Publicas
**********************************************/

/**
 * @brief Inicializa el logger creando los archivos CSV con sus encabezados.
 *
 * Valida los parametros, escribe la fila de encabezado en cada archivo
 * y los deja abiertos en modo append para las escrituras posteriores.
 *
 * @param logger_       Puntero a la instancia del Logger a inicializar.
 * @param dataLogger_   Puntero a la configuracion del canal de datos.
 * @param eventLogger_  Puntero a la configuracion del canal de eventos.
 * @return LOGGER_STATUS_OK si la inicializacion fue exitosa.
 * @return LOGGER_STATUS_NULL_POINTER si algun puntero requerido es NULL.
 * @return LOGGER_STATUS_ERROR si ocurrio un error al crear los archivos.
 */
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

/**
 * @brief Registra una fila de datos en el archivo de datos (dataLogger.csv).
 *
 * Escribe el timestamp seguido de los valores apuntados por dataPoint
 * segun el tipo de dato configurado.
 *
 * @param logger_  Puntero a la instancia del Logger.
 * @param time_    Timestamp de la muestra en microsegundos (o la unidad del simulador).
 * @return LOGGER_STATUS_OK si la escritura fue exitosa.
 * @return LOGGER_STATUS_NULL_POINTER si algun puntero interno es NULL.
 * @return LOGGER_STATUS_ERROR si el tipo de dato no es reconocido.
 */
LoggerStatus logData(Logger_t *logger_, uint64_t time_)
{
    return logging(&logger_->dataConfig, time_);
}

/**
 * @brief Registra una fila de eventos en el archivo de eventos (eventLogger.csv).
 *
 * Funciona de forma identica a logData pero escribe sobre el canal
 * de eventos. Util para registrar mensajes como "[WARNING] Saturacion PID".
 *
 * @param logger_  Puntero a la instancia del Logger.
 * @param time_    Timestamp del evento en microsegundos (o la unidad del simulador).
 * @return LOGGER_STATUS_OK si la escritura fue exitosa.
 * @return LOGGER_STATUS_NULL_POINTER si algun puntero interno es NULL.
 * @return LOGGER_STATUS_ERROR si el tipo de dato no es reconocido.
 */
LoggerStatus logEvent(Logger_t *logger_, uint64_t time_)
{
    return logging(&logger_->eventConfig, time_);
}

/**
 * @brief Libera los recursos del logger cerrando ambos archivos CSV.
 *
 * Debe llamarse al finalizar la sesion de logging para garantizar
 * que todos los datos en buffer sean escritos al disco.
 *
 * @param logger_  Puntero a la instancia del Logger a liberar.
 * @return LOGGER_STATUS_OK si ambos archivos fueron cerrados correctamente.
 * @return LOGGER_STATUS_NULL_POINTER si logger_ es NULL.
 */
LoggerStatus loggerDeInit(Logger_t *logger_)
{
    if (logger_ == NULL)
        return LOGGER_STATUS_NULL_POINTER;

    fclose(logger_->dataConfig.fileLogger);  // Cerramos el Archivo
    fclose(logger_->eventConfig.fileLogger); // Cerramos el Archivo

    return LOGGER_STATUS_OK;
}


/*********************************************
*   Funciones Privadas
**********************************************/
/**
 * @brief Funcion interna generalizada de escritura en un canal de logging.
 *
 * Escribe una fila en el archivo CSV asociado al canal dado, con el formato:
 * timestamp,dato0,dato1,...,datoN
 * Soporta los tipos LOG_INT, LOG_FLOAT, LOG_DOUBLE y LOG_STRING.
 *
 * @param configLogger_  Puntero a la configuracion del canal (datos o eventos).
 * @param time_          Timestamp a escribir al inicio de la fila.
 * @return LOGGER_STATUS_OK si la escritura fue exitosa.
 * @return LOGGER_STATUS_NULL_POINTER si dataPoint o fileLogger son NULL.
 * @return LOGGER_STATUS_ERROR si el tipo de dato configurado no es valido.
 */
static LoggerStatus logging(configLogger_t *configLogger_, uint64_t time_)
{
    if (configLogger_->dataPoint == NULL)
        return LOGGER_STATUS_NULL_POINTER;

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
        case LOG_DOUBLE:
            fprintf(configLogger_->fileLogger, ",%f", ((double *)(configLogger_->dataPoint))[i]);
            break;
        case LOG_STRING:
            fprintf(configLogger_->fileLogger, ",%s", ((const char **)(configLogger_->dataPoint))[i]);
            break;
        default:
            return LOGGER_STATUS_ERROR;
            break;
        }
    }

    fprintf(configLogger_->fileLogger, "\n"); // Salto de Linea

    return LOGGER_STATUS_OK;
}

/**
 * @brief Funcion interna que crea el archivo CSV y escribe la fila de encabezado.
 *
 * Abre el archivo en modo escritura ("w") para crear o sobreescribir,
 * construye dinamicamente la fila "Time,col1,col2,...,colN" y la escribe.
 * Al finalizar, reabre el archivo en modo append ("a") para dejar el
 * handle listo para las escrituras de logData/logEvent.
 *
 * @param configLogger_  Puntero a la configuracion del canal a inicializar.
 * @return LOGGER_STATUS_OK si el encabezado fue escrito correctamente.
 * @return LOGGER_STATUS_NULL_POINTER si configLogger_, pEncabezado o la
 *         apertura del archivo fallan.
 * @return LOGGER_STATUS_ERROR si size es 0.
 */
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
    
    free(str);

    fclose(configLogger_->fileLogger);

    configLogger_->fileLogger = fopen(configLogger_->filename, "a");
    if (configLogger_->fileLogger == NULL)
        return LOGGER_STATUS_NULL_POINTER;

    return LOGGER_STATUS_OK;
}