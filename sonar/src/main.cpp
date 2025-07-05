#include <Arduino.h>
#include "config.h"
#include "logger.h"
#include "modules/sonar_nmea2000.h"

// Instancia del sonar
SonarNMEA2000 sonar;

// Variables de control de tiempo
unsigned long lastDisplayTime = 0;
unsigned long lastDataLogTime = 0;

void init_logger() {
#if USE_LOGGER
    // Inicializar logger con nivel predeterminado
    LogInit(INFO);
    // Configurar niveles por módulo
    LogSetModuleLevel("SONAR", DEBUG);
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
    if (sonar.setup(115200)) {
        LOG_INFO("MAIN", "✅ Sonar inicializado correctamente");
    } else {
        LOG_ERROR("MAIN", "❌ Error al inicializar sonar");
        while(1) {
            delay(1000);
        }
    }
    
    // Opcional: habilitar mensajes raw para debugging
    // sonar.enableRawMessages(true);
    
    LOG_INFO("MAIN", "Sistema listo. Esperando datos del sonar...");
}

void loop() {
    unsigned long currentTime = millis();
    
    // Actualizar sonar (procesar mensajes NMEA2000)
    sonar.update();
    
    // Mostrar datos cada 5 segundos
    if (currentTime - lastDisplayTime >= 5000) {
        sonar.show_data();
        lastDisplayTime = currentTime;
    }
    
    // Logging de datos cada 10 segundos (opcional)
    if (currentTime - lastDataLogTime >= 10000) {
        if (sonar.hasValidDepthData() || sonar.hasValidLogData()) {
            String csvData = sonar.getCSVData();
            LOG_INFO("MAIN", "CSV: " + csvData);
            
            // Aquí podrías guardar en SD, enviar por serie, etc.
            // sdLogger.writeData(csvData);
        }
        lastDataLogTime = currentTime;
    }
    
    // Pequeña pausa para no saturar el procesador
    delay(100);
}