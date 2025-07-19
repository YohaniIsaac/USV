/*
 * SONAR NMEA2000
 */
// Pines CAN para ESP-WROOM-32
#define ESP32_CAN_TX_PIN GPIO_NUM_16  // CAN TX pin  
#define ESP32_CAN_RX_PIN GPIO_NUM_4  // CAN RX pin

#include <Arduino.h>
#include "config.h"
#include "logger.h"
#include "modules/sonar_nmea2000.h"
#include "modules/sonar_transmitter.h" 

// Instancia del sonar
SonarNMEA2000 sonar;
SonarTransmitter transmitter;

// Variables de control de tiempo
unsigned long lastDisplayTime = 0;
unsigned long lastDataCaptureTime = 0;
unsigned long lastTransmissionCheck = 0;

void init_logger() {
#if USE_LOGGER
    // Inicializar logger con nivel predeterminado
    LogInit(INFO);
    // Configurar niveles por módulo
    LogSetModuleLevel("SONAR", DEBUG);
    LogSetModuleLevel("SONAR_TX", DEBUG);
    LOG_INFO("MAIN", "Sistema sonar NMEA2000 iniciando...");
#endif // USE_LOGGER
}

void setup() {
    // Inicializar comunicación serial
    Serial.begin(115200);
    while (!Serial && millis() < 5000) {
        // Esperar conexión serial
    }
    delay(100);

    // Inicializar logger
    init_logger();

    LOG_INFO("MAIN", "=== SISTEMA SONAR NMEA2000 ===");

    // Inicializar sonar
    if (sonar.setup()) {
        LOG_INFO("MAIN", "Sonar inicializado correctamente");
    } else {
        LOG_ERROR("MAIN", "Error al inicializar sonar");
        while(1) {
            delay(1000);
        }
    }

    // Opcional: habilitar mensajes raw para debugging
    // sonar.enableRawMessages(true);

    // Inicializar transmisor
    if (transmitter.begin()) {
        LOG_INFO("MAIN", "Transmisor hacia datalogger inicializado");
        
        // Configurar intervalo de transmisión (opcional)
        transmitter.setTransmissionInterval(SONAR_TRANSMISSION_INTERVAL);
        LOG_INFO("MAIN", "Intervalo de transmisión: " + String(SONAR_TRANSMISSION_INTERVAL) + "ms");
        LOG_INFO("MAIN", "Muestras para promediar: " + String(SONAR_SAMPLES_TO_AVERAGE));

    } else {
        LOG_ERROR("MAIN", "Error al inicializar transmisor");
        while(1) {
            delay(1000);
        }
    }

    LOG_INFO("MAIN", "Sistema listo. Iniciando captura y transmisión de datos...");
    LOG_INFO("MAIN", "");
    LOG_INFO("MAIN", "ORDEN DE DATOS TRANSMITIDOS POR UART:");
    LOG_INFO("MAIN", "Formato: SONAR,timestamp,depth,offset,range,totalLog,tripLog,temperature,valid,samples");
    LOG_INFO("MAIN", "1. timestamp  - Tiempo en milisegundos");
    LOG_INFO("MAIN", "2. depth      - PROFUNDIDAD en metros (PRIMER DATO PRINCIPAL)");
    LOG_INFO("MAIN", "3. offset     - Offset del transductor en metros");
    LOG_INFO("MAIN", "4. range      - Rango de medición en metros");
    LOG_INFO("MAIN", "5. totalLog   - Log total de distancia");
    LOG_INFO("MAIN", "6. tripLog    - Log de viaje");
    LOG_INFO("MAIN", "7. temperature - TEMPERATURA DEL AGUA desde sonar Garmin (°C)");
    LOG_INFO("MAIN", "8. valid      - Validez de los datos (1=válido, 0=inválido)");
    LOG_INFO("MAIN", "9. samples    - Número de muestras promediadas");
    LOG_INFO("MAIN", "");
}

void loop() {
    unsigned long currentTime = millis();

    // Actualizar sonar
    sonar.update();

    // Capturar datos del sonar cada 100ms para tener suficientes muestras
    if (currentTime - lastDataCaptureTime >= 100) {
        
        // Verificar si tenemos datos válidos del sonar
        if (sonar.hasValidDepthData()) {
            // Obtener datos del sonar
            double depth = sonar.getDepth();
            double offset = sonar.getOffset();
            double range = sonar.getRange();
            uint32_t totalLog = sonar.getTotalLog();
            uint32_t tripLog = sonar.getTripLog();
            float temperature = sonar.getTemperature();
            
            // Enviar datos al transmisor para promediado
            transmitter.addSonarMeasurement(depth, offset, range, totalLog, tripLog, temperature);
            
            LOG_VERBOSE("MAIN", "Muestra capturada: depth=" + String(depth, 2) + 
                       "m, temp_agua=" + String(temperature, 1) + "°C, muestras=" + 
                       String(transmitter.getMeasurementCount()));
        } else {
            LOG_DEBUG("MAIN", "Esperando datos válidos del sonar...");
        }
        
        lastDataCaptureTime = currentTime;
    }
    transmitter.update();
    
    delay(50);
}