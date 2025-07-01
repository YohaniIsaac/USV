#ifndef CONFIG_H
#define CONFIG_H


/* 
 *ANALOG SENSOR
 */
// Cantidad de lecturas para los sensores
#define NUM_READINGS 10

// Pines para los sensores analógicos
#define ANALOG_SENSOR_PH 4
#define ANALOG_SENSOR_DO 5
#define ANALOG_SENSOR_EC 6

/*
 * SD LOGGER
 */
// Pines para la tarjeta SD (SPI)
#define SD_CS_PIN 10
#define SD_MOSI_PIN 11
#define SD_MISO_PIN 13
#define SD_SCK_PIN 12

/* 
 * EMERGENCY SYSTEM 
 */
// Pines para el sistema de emergencia
#define EMERGENCY_PIN 7            // Pin para detección de emergencia (normalmente HIGH)

// GPS
#define EMERGENCY_GPS_RX_PIN 18    // RX del GPS de respaldo 18
#define EMERGENCY_GPS_TX_PIN 17    // TX del GPS de respaldo 17

// Pines SPI para el NRF24L01
#define EMERGENCY_NRF_MOSI_PIN 35  // MOSI del NRF
#define EMERGENCY_NRF_SCK_PIN 36   // Clock del NRF
#define EMERGENCY_NRF_MISO_PIN 37  // MISO del NRF
#define EMERGENCY_NRF_CS_PIN 38    // Chip Select (CSN) del NRF
#define EMERGENCY_NRF_CE_PIN 39    // Chip Enable del NRF

#define NRF_CHANNEL 76             // Canal RF (0-125)

// Configuración de tasas de muestreo (en milisegundos) cada cuánto se lee el estado den pin
#define EMERGENCY_CHECK_RATE 0.02     // 20 Hz

/* 
 * SONAR SENSOR
 */
// Pines para el sensor sonar (wcmcu-230)
#define SONAR_RX_PIN 16      
#define SONAR_TX_PIN 15 
#define SONAR_BAUD_RATE 4800   // Velocidad de comunicación 

#define SONAR_UPDATE_RATE 100       // 10 Hz para el sonar

// Timeout para comunicación sonar (ms)
#define SONAR_TIMEOUT 5000         // 5 segundos sin datos = error

/*
 * PIXHAWK INTERFACE
 */
#define PIXHAWK_RX_PIN 16  // GPIO1 para recibir datos (solo RX)
#define PIXHAWK_TX_PIN 15  // GPIO1 para recibir datos (solo RX)
#define PIXHAWK_BAUD_RATE 57600 // Velocidad de comunicación MAVLink


#endif // CONFIG_H