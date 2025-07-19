#ifndef CONFIG_H
#define CONFIG_H

/*
 * CONFIGURACIÓN GENERAL
 */
#ifndef USE_LOGGER
#define USE_LOGGER 1  // Habilitar sistema de logging
#endif

/*
 * SONAR NMEA2000
 */
// Pines CAN para ESP-WROOM-32
#define ESP32_CAN_TX_PIN GPIO_NUM_2  // CAN TX pin  
#define ESP32_CAN_RX_PIN GPIO_NUM_4  // CAN RX pin

// Configuración de comunicación
#define SONAR_SERIAL_BAUD 115200
#define SONAR_UPDATE_RATE 100        // Actualizar cada 100ms
#define SONAR_DISPLAY_RATE 5000      // Mostrar datos cada 5 segundos
#define SONAR_LOG_RATE 10000         // Log cada 10 segundos

// Timeout para datos del sonar
#define SONAR_DATA_TIMEOUT 10000     // 10 segundos sin datos = timeout

/*
 * CONFIGURACIÓN DE PROMEDIADO Y TRANSMISIÓN
 */
#define SONAR_SAMPLES_TO_AVERAGE 10     // Número de muestras para promediar
#define SONAR_TRANSMISSION_INTERVAL 2000 // Transmitir cada 2 segundos (2000ms)

/*
 * COMUNICACIÓN CON DATALOGGER
 */
// Comunicación UART con el ESP32-S3 datalogger
#define DATALOGGER_UART_TX_PIN GPIO_NUM_32    // TX hacia datalogger
#define DATALOGGER_UART_RX_PIN GPIO_NUM_21    // RX desde datalogger  
#define DATALOGGER_BAUD_RATE 9600

#endif // CONFIG_H