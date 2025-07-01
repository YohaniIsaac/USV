#include <Arduino.h>
#include "config.h"
#include "logger.h"
#include "modules/analog_sensors.h"
#include "managers/command_manager.h"
#include "modules/sd_logger.h"
#include "modules/emergency_system.h"
#include "modules/sonar_sensor.h"
#include "modules/pixhawk_interface.h"

// Instancia de configuración PROBADO Y CONFIRMADO
AnalogSensors sensors;
CommandManager commandManager(sensors);
SDLogger micro_sd;
EmergencySystem emergencySystem;
SonarSensor sonar;
PixhawkInterface pixhawk;

// Variables de control
unsigned long lastSonarDisplay = 0;
unsigned long lastDataLog = 0;

// pix
unsigned long lastDisplayTime = 0;
unsigned long lastLogTime = 0;

// Tests
bool one;

void init_logger(){
#if USE_LOGGER
    // Inicializar logger con nivel predeterminado
    LogInit(INFO);
    // Configurar niveles por módulo (opcional)
    LogSetModuleLevel("EMERGENCY", DEBUG);
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
    // commandManager.begin();

    // // Inicar los sensores
    // sensors.begin();

    // // Iniciar SD
    // micro_sd.begin();
    // micro_sd.writeHeader("Ph, PH crudo");

    // Sistema de emergencia
    // LOG_INFO("MAIN", "=== TEST DE DETECCIÓN DE VELOCIDAD DEL SONAR ===");
    
    // // Ejecutar detección de velocidad del sonar
    // LOG_INFO("MAIN", "Iniciando detección de velocidad del sonar...");
    
    // if (sonar.detectBaudRate()) {
    //     LOG_INFO("MAIN", "¡Velocidad detectada exitosamente!");
    //     LOG_INFO("MAIN", "Ahora reiniciando sonar con la velocidad detectada...");
    //     sonar.begin();
    //     LOG_INFO("MAIN", "=== TEST COMPLETADO ===");
    //     LOG_INFO("MAIN", "Actualiza SONAR_BAUD_RATE en config.h con el valor mostrado arriba");
    // } else {
    //     LOG_ERROR("MAIN", "No se pudo detectar una velocidad válida");
    //     LOG_ERROR("MAIN", "Verifica las conexiones del sonar:");
    //     LOG_ERROR("MAIN", "- RX Pin: " + String(SONAR_RX_PIN));
    //     LOG_ERROR("MAIN", "- TX Pin: " + String(SONAR_TX_PIN));
    //     LOG_ERROR("MAIN", "- Alimentación del sensor");
    // }
    
    // LOG_INFO("MAIN", "Test finalizado. Iniciando monitoreo de datos...");

    /* PIX */
    LOG_INFO("MAIN", "Iniciando sistema de telemetría Pixhawk");
    
    // Inicializar interfaz Pixhawk
    pixhawk.begin();
    one = true;
}

void loop() {
    unsigned long currentTime = millis();

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

    // Mostrar datos cada 2 segundos
    if (currentTime - lastDisplayTime >= 2000) {
        LOG_INFO("MAIN", "=== DATOS PIXHAWK ===");
        LOG_INFO("MAIN", "Latitud: " + String(pixhawk.getLatitude(), 6) + "°");
        LOG_INFO("MAIN", "Longitud: " + String(pixhawk.getLongitude(), 6) + "°");
        LOG_INFO("MAIN", "Altitud: " + String(pixhawk.getAltitude(), 2) + " m");
        LOG_INFO("MAIN", "Heading: " + String(pixhawk.getHeading(), 1) + "°");
        LOG_INFO("MAIN", "====================");
        lastDisplayTime = currentTime;
    }

}