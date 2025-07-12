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
#define SD_CS_PIN 11
#define SD_MOSI_PIN 12
#define SD_MISO_PIN 14
#define SD_SCK_PIN 13

/* 
 * EMERGENCY SYSTEM 
 */
// Pines para el sistema de emergencia
#define EMERGENCY_VOLTAGE_PIN  8   // Pin para detección de emergencia (normalmente HIGH)
#define EMERGENCY_POWER_CONTROL_PIN 7           // Detecta el voltaje de la batyería

// Configuración del divisor de tensión
#define VOLTAGE_MAX_REAL 16.8               // Voltaje máximo real del sistema (16.8V)
#define VOLTAGE_SCALE_FACTOR 5.09           // Factor de escalado (16.8V / 3.3V = 5.09)

// Configuración de voltajes de emergencia
#define EMERGENCY_VOLTAGE_THRESHOLD_REAL 12.20    // Voltaje mínimo antes de activar emergencia (en Volts)
#define EMERGENCY_VOLTAGE_HYSTERESIS_REAL  0.5   // Histéresis para evitar oscilaciones (en Volts)

// Configuración de muestreo
#define EMERGENCY_VOLTAGE_SAMPLES 5         // Número de muestras para promedio

// GPS
#define EMERGENCY_GPS_RX_PIN 17    // RX del GPS de respaldo 18
#define EMERGENCY_GPS_TX_PIN 18    // TX del GPS de respaldo 17 ********

// Pines SPI para el NRF24L01
#define EMERGENCY_NRF_MOSI_PIN 36  // MOSI del NRF
#define EMERGENCY_NRF_SCK_PIN 38   // Clock del NRF
#define EMERGENCY_NRF_MISO_PIN 37  // MISO del NRF
#define EMERGENCY_NRF_CS_PIN 35    // Chip Select (CSN) del NRF
#define EMERGENCY_NRF_CE_PIN 39    // Chip Enable del NRF

#define NRF_CHANNEL 76             // Canal RF (0-125)

// Configuración de tasas de muestreo (en milisegundos) cada cuánto se lee el estado den pin
#define EMERGENCY_CHECK_RATE 100      // Verificar voltaje cada 100ms (10 Hz)

/*
 * PIXHAWK INTERFACE
 */
#define PIXHAWK_RX_PIN 16  // GPIO1 para recibir datos (solo RX)
#define PIXHAWK_TX_PIN 15  // GPIO1 para recibir datos (solo RX)
#define PIXHAWK_BAUD_RATE 57600 // Velocidad de comunicación MAVLink

/*
 * COMUNICACIÓN CON ESP-WROOM32 - UART3 PERSONALIZADO
 */
#define WROOM_UART_RX_PIN 2      
#define WROOM_UART_TX_PIN 1        //  (aunque no lo uses)
#define WROOM_BAUD_RATE 9600       // Velocidad de comunicación
#define WROOM_UART_NUM 1           // Usar UART1 reasignado

#endif // CONFIG_H