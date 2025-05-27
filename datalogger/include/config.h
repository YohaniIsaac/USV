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

// Pines para el sistema de emergencia
#define EMERGENCY_PIN 7            // Pin para detección de emergencia (normalmente HIGH)
// GPS
#define EMERGENCY_GPS_RX_PIN 18    // RX del GPS de respaldo n18
#define EMERGENCY_GPS_TX_PIN 17    // TX del GPS de respaldo 17
// Pines SPI para el NRF24L01
#define EMERGENCY_NRF_MOSI_PIN 35  // MOSI del NRF
#define EMERGENCY_NRF_SCK_PIN 36   // Clock del NRF
#define EMERGENCY_NRF_MISO_PIN 37  // MISO del NRF
#define EMERGENCY_NRF_CS_PIN 38    // Chip Select (CSN) del NRF
#define EMERGENCY_NRF_CE_PIN 39    // Chip Enable del NRF (necesario para NRF24L01)


#define NRF_CHANNEL 76             // Canal RF (0-125)


// Configuración de tasas de muestreo (en milisegundos)
#define SD_WRITE_RATE 1000          // 1 Hz
#define EMERGENCY_CHECK_RATE 0.02     // 20 Hz

// Nombre del archivo de registro
#define LOG_FILENAME "/data_log.csv"

#endif // CONFIG_H