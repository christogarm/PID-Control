/**
 * @file    Plant.h
 * @brief   Simulacion de un sistema de Refrigeracion enfocado en el Evaporador.
 *
 * @details Modela la dinamica de un evaporador mediante una funcion de primer orden
 *          con retardo de transporte. La entrada es el porcentaje de apertura de la
 *          valvula electronica y la salida es el Superheat (SH).
 *
 *          Modelo discreto (Euler hacia adelante):
 *          out[k] = out[k-1] + dt * (K * in[k-N] - out[k-1] + d[k]) / tau
 *
 *          Donde N es el numero de muestras de retardo (delaySamples).
 */
#ifndef PLANT_PLANT_H_
#define PLANT_PLANT_H_

#include <stdbool.h>

/* Estado de la Planta */
typedef enum
{
    PLANT_STATUS_OK,
    PLANT_STATUS_NULL_POINTER,
    PLANT_STATUS_ERROR
} PlantStatus;

typedef struct
{
    float tau;                              // Constante del tiempo del Evaporador
    float K;                                // Ganancia de la Entrada, esta es una constante proporcional entre la entrada y la salida
    
    float initOutput, initInput;            // Condiciones iniciales de la salida
    float minOut, maxOut;                   // Saturacion de Salida
    float minIn, maxIn;                     // Saturacion de la Entrada

    float noiseAmpIN, noiseAmpOUT;          // Ruido Aletorio que tendra una maxima amplitud

    int delaySamplesIN, delaySamplesOUT;    // Retardos entre las muestras para la entrada y salida

} configPlant;

/* Datos de la Planta */
typedef struct
{
    float in;                           // Porcentaje de Apertura de la Valvula (Entrada de la Planta)
    float out, outPrev;                 // Superheat actual y anterior (Salida de la Planta)
    float outMeasured;                  // Salida que contiene el retraso
    float d;                            // Perturbaciones del sistema (carga térmica, temperatura ambiente, etc)
    float difTime;                      // Diferencial de Tiempo entre los dos SH
    float *delayBufferIN;               // Buffer de datos donde se almacenaran los datos Retardados de la entrada
    float *delayBufferOUT;              // Buffer de datos donde se almacenaran los datos Retardados de la Salida
    int delayIndexIN, delayIndexOUT;    // Index del Buffer
    bool isInitialized;                 // Bandera para verificar si esta inicializado

    configPlant config;                 // Paramteros de condiguracion de la Planta
} Plant_t;

PlantStatus plant_Init(Plant_t *plant_, configPlant *config_);                                  // Inicializacion de la Planta
PlantStatus plant_Update(Plant_t *plant_, float input_, float disturbance_, float difTime_);    // Actualizamos un nuevo valor de Salida de la Planta
PlantStatus plant_Reset(Plant_t *plant_);                                                       // Reinicia los valores
PlantStatus plant_Deinit(Plant_t *plant_);                                                      // Desinializa la Planta

#endif