
#include "Plant.h"
#include <stddef.h>
#include <stdlib.h>
#include <time.h>

/* Prototipos de Funciones Privadas */
static PlantStatus saturation(float *value_, float minValue_, float maxValue_);                                // Saturacion de un valor
static PlantStatus bufferDelayInit(float **delayBuffer_, int delaySamples_, float initValue_);                 // Inicializa los valores del Buffer de Delay
static PlantStatus bufferDelayUpdate(float *delayBuffer_, int *delayIndex_, int delaySamples_, float *value_); // Actualiza un nuevo valor del Delay

/***************************
 *   Funciones Publicas
 ****************************/

/**
 * @brief   Inicializa la planta con los parametros de configuracion dados.
 *
 * @details Valida los parametros, reserva memoria para el buffer de retardo,
 *          e inicializa el estado interno con las condiciones iniciales.
 *          Llama a srand() para inicializar el generador de ruido aleatorio.
 *
 * @param[out] plant_   Puntero a la estructura de la planta a inicializar.
 * @param[in]  config_  Puntero a la configuracion deseada. No se modifica.
 *
 * @return PLANT_STATUS_OK           Inicializacion exitosa.
 * @return PLANT_STATUS_NULL_POINTER Si plant_ o config_ son NULL,
 *                                   o si falla la reserva de memoria.
 * @return PLANT_STATUS_ERROR        Si algun parametro de configuracion es invalido.
 *
 * @note  Debe llamarse antes de cualquier otra funcion de la API.
 * @note  Liberar los recursos con plant_Deinit() cuando ya no se necesite la planta.
 */
PlantStatus plant_Init(Plant_t *plant_, configPlant *config_)
{
    if (plant_ == NULL || config_ == NULL)
        return PLANT_STATUS_NULL_POINTER;

    plant_->isInitialized = false; // En caso de que los parametros de configuracion no esten bien configurados

    if (config_->minIn >= config_->maxIn || config_->minOut >= config_->maxOut)
        return PLANT_STATUS_ERROR;

    if (config_->tau <= 0 || config_->noiseAmpIN < 0 || config_->noiseAmpOUT < 0)
        return PLANT_STATUS_ERROR;

    if (config_->delaySamplesIN <= 0 || config_->delaySamplesOUT <= 0)
        return PLANT_STATUS_ERROR;

    if (config_->initOutput < config_->minOut || config_->initOutput > config_->maxOut)
        return PLANT_STATUS_ERROR;

    plant_->config = *config_;

    plant_->d = 0;
    plant_->difTime = 0;
    plant_->in = 0;
    plant_->outMeasured = config_->initOutput;
    plant_->out = config_->initOutput;
    plant_->outPrev = config_->initOutput;

    plant_->delayIndexIN = 0; // Index del buffer
    plant_->delayIndexOUT = 0;

    if (bufferDelayInit(&plant_->delayBufferIN, plant_->config.delaySamplesIN, plant_->config.initInput) != PLANT_STATUS_OK)
        return PLANT_STATUS_ERROR;

    if (bufferDelayInit(&plant_->delayBufferOUT, plant_->config.delaySamplesOUT, plant_->config.initOutput) != PLANT_STATUS_OK)
    {
        free(plant_->delayBufferIN); // liberar antes de salir
        plant_->delayBufferIN = NULL;
        return PLANT_STATUS_ERROR;
    }
    srand(time(NULL));

    plant_->isInitialized = true;

    return PLANT_STATUS_OK;
}

/**
 * @brief   Calcula un nuevo valor de salida de la planta para el ciclo actual.
 *
 * @details Aplica saturacion a la entrada, la introduce al buffer de retardo,
 *          integra la dinamica de primer orden con el metodo de Euler hacia adelante,
 *          agrega ruido aleatorio y satura la salida.
 *
 * @param[in,out] plant_        Puntero a la estructura de la planta.
 * @param[in]     input_        Porcentaje de apertura de la valvula [minIn, maxIn].
 * @param[in]     disturbance_  Perturbacion externa del sistema (carga termica, etc.).
 * @param[in]     difTime_      Paso de tiempo transcurrido desde el ultimo ciclo [s]. Debe ser > 0.
 *
 * @return PLANT_STATUS_OK           Actualizacion exitosa.
 * @return PLANT_STATUS_NULL_POINTER Si plant_ es NULL.
 * @return PLANT_STATUS_ERROR        Si la planta no esta inicializada, difTime_ <= 0,
 *                                   o falla la saturacion interna.
 *
 * @warning La estabilidad numerica requiere que difTime_ < 2 * tau.
 *          Con pasos de tiempo mayores el integrador de Euler puede divergir.
 */
PlantStatus plant_Update(Plant_t *plant_, float input_, float disturbance_, float difTime_)
{
    if (plant_ == NULL) // Proteccion para que no envie un apuntador nulo
        return PLANT_STATUS_NULL_POINTER;

    if (plant_->isInitialized == false)
        return PLANT_STATUS_ERROR;

    if (difTime_ <= 0.0f)
        return PLANT_STATUS_ERROR;

    float noise = (((float)rand() / RAND_MAX) * 2.0f - 1.0f) * plant_->config.noiseAmpIN;
    input_ += noise;

    if (saturation(&input_, plant_->config.minIn, plant_->config.maxIn) != PLANT_STATUS_OK)
        return PLANT_STATUS_ERROR;

    /* Almacenas la nueva entrada en el buffer y entregas la entrada retrasada */
    if (bufferDelayUpdate(plant_->delayBufferIN, &(plant_->delayIndexIN), plant_->config.delaySamplesIN, &input_) != PLANT_STATUS_OK)
        return PLANT_STATUS_ERROR;

    /* Cargamos el nuevo valor de la entrada del Sistema */
    plant_->in = input_;
    plant_->d = disturbance_;

    /* Cargamos el tiempo Transcurrido */
    plant_->difTime = difTime_;

    /* Obtenemos el nuevo valor de Salida */
    plant_->out = plant_->outPrev + (plant_->difTime) * ((plant_->config.K * plant_->in - plant_->outPrev + plant_->d) / (plant_->config.tau));

    noise = (((float)rand() / RAND_MAX) * 2.0f - 1.0f) * plant_->config.noiseAmpOUT;
    plant_->out += noise;

    if (saturation(&plant_->out, plant_->config.minOut, plant_->config.maxOut) != PLANT_STATUS_OK)
        return PLANT_STATUS_ERROR;

    /* Actualizamos el Salida previa */
    plant_->outPrev = plant_->out;
    plant_->outMeasured = plant_->out;

    /* Almacenas la nueva salida en el buffer y entregas la entrada retrasada */
    if (bufferDelayUpdate(plant_->delayBufferOUT, &(plant_->delayIndexOUT), plant_->config.delaySamplesOUT, &plant_->outMeasured) != PLANT_STATUS_OK)
        return PLANT_STATUS_ERROR;

    return PLANT_STATUS_OK;
}

/**
 * @brief   Reinicia el estado interno de la planta a las condiciones iniciales.
 *
 * @details Restaura out, outPrev, el buffer de retardo e indices al estado
 *          equivalente al de una inicializacion recien completada. No libera
 *          ni reasigna memoria. Los parametros de configuracion no cambian.
 *
 * @param[in,out] plant_  Puntero a la estructura de la planta.
 *
 * @return PLANT_STATUS_OK           Reset exitoso.
 * @return PLANT_STATUS_NULL_POINTER Si plant_ o delayBuffer son NULL.
 * @return PLANT_STATUS_ERROR        Si la planta no esta inicializada.
 */
PlantStatus plant_Reset(Plant_t *plant_)
{
    if (plant_ == NULL)
        return PLANT_STATUS_NULL_POINTER;
    if (plant_->delayBufferIN == NULL || plant_->delayBufferOUT == NULL)
        return PLANT_STATUS_NULL_POINTER;
    if (plant_->isInitialized == false)
        return PLANT_STATUS_ERROR;

    plant_->d = 0;
    plant_->difTime = 0;
    plant_->in = 0;
    plant_->out = plant_->config.initOutput;
    plant_->outPrev = plant_->config.initOutput;
    plant_->outMeasured = plant_->config.initOutput;

    plant_->delayIndexIN = 0;  // Index del buffer
    plant_->delayIndexOUT = 0; // Index del buffer

    for (size_t i = 0; i < plant_->config.delaySamplesIN; i++)
        plant_->delayBufferIN[i] = plant_->config.initInput;

    for (size_t i = 0; i < plant_->config.delaySamplesOUT; i++)
        plant_->delayBufferOUT[i] = plant_->config.initOutput;

    return PLANT_STATUS_OK;
}

/**
 * @brief   Desinicializa la planta y libera los recursos de memoria.
 *
 * @details Libera el buffer de retardo reservado en plant_Init() y marca
 *          la planta como no inicializada. Despues de esta llamada la estructura
 *          puede reutilizarse con plant_Init().
 *
 * @param[in,out] plant_  Puntero a la estructura de la planta.
 *
 * @return PLANT_STATUS_OK           Desinicializacion exitosa.
 * @return PLANT_STATUS_NULL_POINTER Si plant_ es NULL.
 * @return PLANT_STATUS_ERROR Si plant_->isInitialized es falso
 *
 * @note  Siempre llamar esta funcion antes de descartar una planta inicializada
 *        para evitar memory leaks.
 */
PlantStatus plant_Deinit(Plant_t *plant_)
{
    if (plant_ == NULL)
        return PLANT_STATUS_NULL_POINTER;
    if(plant_->isInitialized == false)
        return PLANT_STATUS_ERROR;

    free(plant_->delayBufferIN);
    free(plant_->delayBufferOUT);

    plant_->delayBufferIN = NULL;
    plant_->delayBufferOUT = NULL;
    plant_->isInitialized = false;

    return PLANT_STATUS_OK;
}

/***************************
 *   Funciones Privadas
 ****************************/

/**
 * @brief  Aplica saturacion a un valor escalar dentro de un rango dado.
 *
 * @param[in,out] value_     Puntero al valor a saturar. Se modifica en sitio.
 * @param[in]     minValue_  Limite inferior del rango.
 * @param[in]     maxValue_  Limite superior del rango. Debe ser > minValue_.
 *
 * @return PLANT_STATUS_OK           Saturacion aplicada correctamente.
 * @return PLANT_STATUS_NULL_POINTER Si value_ es NULL.
 * @return PLANT_STATUS_ERROR        Si minValue_ >= maxValue_.
 */
static PlantStatus saturation(float *value_, float minValue_, float maxValue_)
{
    if (value_ == NULL)
        return PLANT_STATUS_NULL_POINTER;
    if (minValue_ >= maxValue_)
        return PLANT_STATUS_ERROR;

    if (*value_ > maxValue_)
        *value_ = maxValue_;
    else if (*value_ < minValue_)
        *value_ = minValue_;

    return PLANT_STATUS_OK;
}

/**
 * @brief  Reserva memoria e inicializa un buffer circular de retardo.
 *
 * @details Asigna dinamicamente un arreglo de delaySamples_ elementos de tipo float
 *          y los inicializa todos con initValue_. El puntero resultante se escribe
 *          en *delayBuffer_.
 *
 * @param[out] delayBuffer_   Doble puntero donde se almacenara la direccion del buffer
 *                            reservado. Al retornar, *delayBuffer_ apunta a la memoria asignada.
 * @param[in]  delaySamples_  Numero de muestras del buffer. Debe ser > 0.
 * @param[in]  initValue_     Valor con el que se prellenan todas las posiciones del buffer.
 *                            Debe coincidir con la condicion inicial del sistema para evitar
 *                            transitorios artificiales al arranque.
 *
 * @return PLANT_STATUS_OK           Buffer reservado e inicializado correctamente.
 * @return PLANT_STATUS_NULL_POINTER Si delayBuffer_ es NULL o si malloc falla.
 * @return PLANT_STATUS_ERROR        Si delaySamples_ <= 0.
 *
 * @note  La memoria reservada debe liberarse con free() cuando ya no se necesite.
 *        Esto se realiza en plant_Deinit().
 */
static PlantStatus bufferDelayInit(float **delayBuffer_, int delaySamples_, float initValue_)
{
    if (delayBuffer_ == NULL)
        return PLANT_STATUS_NULL_POINTER;
    if (delaySamples_ <= 0)
        return PLANT_STATUS_ERROR;

    *delayBuffer_ = (float *)malloc(delaySamples_ * sizeof(float)); // Buffer del delay

    if (*delayBuffer_ == NULL)
        return PLANT_STATUS_NULL_POINTER;

    for (size_t i = 0; i < delaySamples_; i++)
        (*delayBuffer_)[i] = initValue_; // Inicializamos la variable

    return PLANT_STATUS_OK;
}

/**
 * @brief  Inserta un nuevo valor en el buffer circular y retorna el valor retrasado.
 *
 * @details Opera como un buffer FIFO circular de longitud fija:
 *          1. Escribe *value_ en la posicion actual del indice.
 *          2. Avanza el indice al siguiente slot (con wraparound).
 *          3. Lee el valor mas antiguo del buffer (el que lleva delaySamples_ ciclos)
 *             y lo escribe de vuelta en *value_.
 *
 *          De esta forma, la funcion entrega una version retrasada N muestras
 *          de la señal de entrada, donde N = delaySamples_.
 *
 * @param[in]     delayBuffer_   Puntero al buffer circular previamente inicializado.
 * @param[in,out] delayIndex_    Puntero al indice actual de escritura. Se actualiza en sitio.
 * @param[in]     delaySamples_  Tamaño del buffer. Debe ser > 0.
 * @param[in,out] value_         Entrada: valor nuevo a insertar.
 *                               Salida:  valor retrasado N muestras.
 *
 * @return PLANT_STATUS_OK           Operacion exitosa.
 * @return PLANT_STATUS_NULL_POINTER Si delayBuffer_, delayIndex_ o value_ son NULL.
 * @return PLANT_STATUS_ERROR        Si delaySamples_ <= 0.
 */
static PlantStatus bufferDelayUpdate(float *delayBuffer_, int *delayIndex_, int delaySamples_, float *value_)
{
    if (delayBuffer_ == NULL || delayIndex_ == NULL || value_ == NULL)
        return PLANT_STATUS_NULL_POINTER;
    if (delaySamples_ <= 0)
        return PLANT_STATUS_ERROR;

    delayBuffer_[*delayIndex_] = *value_; // Guardas entrada actual

    *delayIndex_ = (*delayIndex_ + 1) % delaySamples_; // Calculas índice retrasado y a su vez sera el proximo indice de escritura

    *value_ = delayBuffer_[*delayIndex_];

    return PLANT_STATUS_OK;
}
