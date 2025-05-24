#ifndef CONFIG_H
#define CONFIG_H


// Cantidad de lecturas para los sensores
#define NUM_READINGS 10

// Pines para los sensores analógicos
#define ANALOG_SENSOR_PH 4
#define ANALOG_SENSOR_DO 5
#define ANALOG_SENSOR_EC 6

// Pines para la tarjeta SD (SPI)
#define SD_CS_PIN 10
#define SD_MOSI_PIN 11
#define SD_MISO_PIN 13
#define SD_SCK_PIN 12




// Pines para la comunicación con el sensor sonar (UART)
#define SONAR_RX_PIN 16
#define SONAR_TX_PIN 17

// Pines para la comunicación con la Pixhawk (UART)
#define PIXHAWK_RX_PIN 14
#define PIXHAWK_TX_PIN 15

// Pin para detección de emergencia
#define EMERGENCY_PIN 4

// Pines para el GPS de respaldo
#define GPS_RX_PIN 12
#define GPS_TX_PIN 13
#define GPS_POWER_PIN 27  // Pin para encender/apagar el GPS

// Configuración de tasas de muestreo (en milisegundos)
#define ANALOG_SAMPLING_RATE 100    // 10 Hz
#define SONAR_SAMPLING_RATE 200     // 5 Hz
#define PIXHAWK_SAMPLING_RATE 100   // 10 Hz
#define SD_WRITE_RATE 1000          // 1 Hz
#define EMERGENCY_CHECK_RATE 50     // 20 Hz

// Nombre del archivo de registro
#define LOG_FILENAME "/data_log.csv"

#endif // CONFIG_H