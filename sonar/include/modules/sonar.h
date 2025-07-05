#ifndef NMEA2000_SONAR_READER_H
#define NMEA2000_SONAR_READER_H

#include <Arduino.h>
#include <NMEA2000_CAN.h>
#include <NMEA2000_esp32.h>
#include <N2kMessages.h>
#include <N2kMsg.h>


class NMEA2000SonarReader {
public:
    /**
     * @brief Constructor de la clase
     */
    NMEA2000SonarReader();

    /**
     * @brief Destructor de la clase
     */
    ~NMEA2000SonarReader();

    /**
     * @brief Inicializa el lector NMEA2000
     * @param baudRate Velocidad del puerto serie (por defecto 115200)
     * @return true si la inicialización fue exitosa
     */
    bool begin(uint32_t baudRate = 115200);

    /**
     * @brief Actualiza el procesamiento de mensajes NMEA2000
     * Debe llamarse en el loop principal
     */
    void update();

    /**
     * @brief Habilita o deshabilita la impresión de mensajes raw
     * @param enable true para habilitar, false para deshabilitar
     */
    void enableRawMessages(bool enable);

    /**
     * @brief Obtiene el último valor de profundidad leído
     * @return Profundidad en metros, o NaN si no hay datos válidos
     */
    double getLastDepth() const;

    /**
     * @brief Obtiene el último offset leído
     * @return Offset en metros, o NaN si no hay datos válidos
     */
    double getLastOffset() const;

    /**
     * @brief Obtiene el último rango leído
     * @return Rango en metros, o NaN si no hay datos válidos
     */
    double getLastRange() const;

    /**
     * @brief Obtiene el último log total leído
     * @return Log total en metros, o 0 si no hay datos válidos
     */
    uint32_t getLastTotalLog() const;

    /**
     * @brief Obtiene el último trip log leído
     * @return Trip log en metros, o 0 si no hay datos válidos
     */
    uint32_t getLastTripLog() const;

    /**
     * @brief Verifica si hay datos de profundidad válidos
     * @return true si hay datos válidos
     */
    bool hasValidDepthData() const;

    /**
     * @brief Verifica si hay datos de log válidos
     * @return true si hay datos válidos
     */
    bool hasValidLogData() const;

private:
    // Variables para almacenar los últimos datos recibidos
    double lastDepth_;
    double lastOffset_;
    double lastRange_;
    uint32_t lastTotalLog_;
    uint32_t lastTripLog_;
    
    // Flags de estado
    bool depthDataValid_;
    bool logDataValid_;
    bool rawMessagesEnabled_;
    bool initialized_;

    // Métodos privados para procesar mensajes
    /**
     * @brief Handler principal para mensajes NMEA2000
     * @param N2kMsg Mensaje NMEA2000 recibido
     */
    void handleNMEA2000Message(const tN2kMsg &N2kMsg);

    /**
     * @brief Procesa datos de profundidad del agua
     * @param N2kMsg Mensaje NMEA2000 con datos de profundidad
     */
    void processDepthData(const tN2kMsg &N2kMsg);

    /**
     * @brief Procesa datos del log de distancia
     * @param N2kMsg Mensaje NMEA2000 con datos de log
     */
    void processDistanceLog(const tN2kMsg &N2kMsg);

    /**
     * @brief Imprime mensaje raw para debugging
     * @param N2kMsg Mensaje NMEA2000 raw
     */
    void printRawMessage(const tN2kMsg &N2kMsg);

    /**
     * @brief Función estática para manejar callbacks de NMEA2000
     * @param N2kMsg Mensaje NMEA2000 recibido
     */
    static void staticMessageHandler(const tN2kMsg &N2kMsg);

    // Instancia estática para el callback
    static NMEA2000SonarReader* instance_;
};

#endif // NMEA2000_SONAR_READER_H