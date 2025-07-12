#include <Arduino.h>
#include "config.h"
#include "logger.h"
#include "modules/analog_sensors.h"
#include "managers/command_manager.h"
#include "modules/sd_logger.h"
#include "modules/emergency_system.h"
#include "modules/sonar_receiver.h"
#include "modules/pixhawk_interface.h"

// Instancia de configuración PROBADO Y CONFIRMADO
AnalogSensors sensors;
CommandManager commandManager(sensors);
SDLogger micro_sd;
EmergencySystem emergencySystem;
SonarReceiver sonar;
PixhawkInterface pixhawk;

// Variables de control
unsigned long lastSonarDisplay = 0;
unsigned long lastDataLog = 0;
unsigned long lastDisplayTime = 0;
unsigned long lastLogTime = 0;
unsigned long lastEmergencyDisplay = 0;

// Tests
bool one;

void init_logger(){
#if USE_LOGGER
    // Inicializar logger con nivel predeterminado
    LogInit(INFO);
    // Configurar niveles por módulo (opcional)
    LogSetModuleLevel("EMERGENCY", VERBOSE);
    LogSetModuleLevel("ANALOG", DEBUG);
    LogSetModuleLevel("SD_LOGGER", DEBUG);
    LogSetModuleLevel("PIXHAWK", ERROR);
    LogSetModuleLevel("SONAR", DEBUG);
    LogSetModuleLevel("PIXHAWK", DEBUG);
    LOG_INFO("MAIN", "Sistema datalogger iniciando...");
#endif // USE_LOGGER
}


void setup() {
    // Inicializar comunicación serial
    Serial.begin(115200);
    while (!Serial && millis() < 5000) {
    }
    delay(10);
    init_logger();

    // Iniciar los comandos del monitor serial
    commandManager.begin();

    // Inicar los sensores
    sensors.begin();

    // Iniciar SD
    micro_sd.begin();
    micro_sd.writeHeader("Ph, PH crudo");

    // Sistema de emergencia
    LOG_INFO("MAIN", "=== TEST DE DETECCIÓN DE VELOCIDAD DEL SONAR ===");
    
    LOG_INFO("MAIN", "=== CONFIGURACIÓN INICIAL DEL SISTEMA DE EMERGENCIA ===");
    LOG_INFO("MAIN", "Pin de lectura de voltaje: GPIO" + String(EMERGENCY_VOLTAGE_PIN));
    LOG_INFO("MAIN", "Pin de control de alimentación: GPIO" + String(EMERGENCY_POWER_CONTROL_PIN));
    LOG_INFO("MAIN", "Voltaje máximo del sistema: " + String(VOLTAGE_MAX_REAL, 1) + "V");
    LOG_INFO("MAIN", "Factor de escala: " + String(VOLTAGE_SCALE_FACTOR, 2));
    LOG_INFO("MAIN", "Umbral de emergencia: " + String(EMERGENCY_VOLTAGE_THRESHOLD_REAL, 2) + "V");
    LOG_INFO("MAIN", "Histéresis: " + String(EMERGENCY_VOLTAGE_HYSTERESIS_REAL, 2) + "V");
    LOG_INFO("MAIN", "Frecuencia de chequeo: cada " + String(EMERGENCY_CHECK_RATE) + "ms");
    LOG_INFO("MAIN", "========================================================");


    /* PIX */
    // LOG_INFO("MAIN", "Iniciando sistema de telemetría Pixhawk");
    
    // Inicializar interfaz Pixhawk
    // pixhawk.begin();


}

void loop() {
    unsigned long currentTime = millis();

    // ===== SISTEMA DE EMERGENCIA (PRIORITARIO) =====
    emergencySystem.update();
    
    // Mostrar estado de emergencia cada 5 segundos
    if (currentTime - lastEmergencyDisplay >= 5000) {
        LOG_DEBUG("MAIN", "Voltaje: " + String(emergencySystem.getCurrentVoltage(), 3) + 
                  "V | Umbral: " + String(EMERGENCY_VOLTAGE_THRESHOLD_REAL, 2) + 
                  "V | Emergencia: " + String(emergencySystem.isEmergencyActive() ? "ACTIVA" : "OK"));
        lastEmergencyDisplay = currentTime;
    }

    // Procesar lectura de sensores 
    // sensors.update();
    // // Procesar comandos seriales (siempre activo)
    // commandManager.update();
    // if (one) {
    //     LOG_INFO("MAIN", "entrando al if de primera escritura");
    //     String enviar = String(sensors.lastPH) + "," + String(sensors.lastRawPH);
    //     micro_sd.writeData(enviar);
    //     micro_sd.update();
    //     LOG_INFO("MAIN", "SD updated");
    //     one = false;
    // }    
    // delay(10000);
    // emergencySystem.update();
        // Solo actualizar y mostrar datos del sonar cada 2 segundos
    // static unsigned long lastDisplay = 0;
    // unsigned long currentTime = millis();
    
    // // Actualizar sonar
    // sonar.update();
    
    // // Mostrar datos cada 2 segundos
    // if (currentTime - lastDisplay >= 2000) {
    //     if (sonar.isDataValid()) {
    //         LOG_INFO("MAIN", "Sonar OK - Profundidad: " + String(sonar.getDepth(), 2) + 
    //                  "m, Temp: " + String(sonar.getTemperature(), 1) + "°C");
    //     } else {
    //         LOG_WARN("MAIN", "Sonar - Sin datos válidos");
    //         String lastData = sonar.getLastRawData();
    //         if (lastData.length() > 0) {
    //             LOG_DEBUG("MAIN", "Último dato raw: " + lastData.substring(0, min(100, (int)lastData.length())));
    //         }
    //     }
    //     lastDisplay = currentTime;
    // }
    
    // delay(100);

    // Mostrar información del sonar cada 2 segundos
    // if (currentTime - lastSonarDisplay >= 2000) {
    //     if (sonar.isDataValid()) {
    //         LOG_INFO("MAIN", "Sonar - Profundidad: " + String(sonar.getDepth(), 2) + 
    //                  "m, Temp: " + String(sonar.getTemperature(), 1) + "°C");
    //     } else {
    //         LOG_WARN("MAIN", "Sonar - Datos inválidos o timeout");
    //         LOG_DEBUG("MAIN", "Último dato raw: " + sonar.getLastRawData());
    //     }
    //     lastSonarDisplay = currentTime;
    // }

    // Sistema de sonar - detectar velocidad automáticamente

    // emergencySystem.testGPSBaudRates();

    /* PIX */
    // Actualizar interfaz Pixhawk
    pixhawk.update();

    // pixhawk.show_message();



    // Logging a SD cada 10 segundos (opcional)
    if (currentTime - lastLogTime >= 10000) {
        String csvData = pixhawk.save_CSVData();
        // micro_sd.writeData(csvData);
        // micro_sd.update();
        LOG_DEBUG("MAIN", "CSV: " + csvData);
        lastLogTime = currentTime;
    }
    delay(100);

}