#include <Arduino.h>
#include "config.h"
#include "logger.h"
#include "modules/analog_sensors.h"
#include "managers/command_manager.h"
#include "modules/sd_logger.h"
#include "modules/emergency_system.h"

// Instancia de configuración PROBADO Y CONFIRMADO
AnalogSensors sensors;
CommandManager commandManager(sensors);
SDLogger micro_sd;
EmergencySystem emergencySystem;


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
    LogSetModuleLevel("SONAR", ERROR);
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
    emergencySystem.begin();

    LOG_INFO("MAIN", "Configuración completada");
    LOG_INFO("MAIN", "Escriba 'help' para ver comandos disponibles");
    one = true;
}

void loop() {
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
    delay(5000);
    emergencySystem.update();
    // emergencySystem.testGPSBaudRates();

}