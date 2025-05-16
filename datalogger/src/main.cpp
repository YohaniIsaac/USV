#include <Arduino.h>
#include "logger.h"
#include "config.h"
#include "modules/analog_sensors.h"
#include "modules/sd_logger.h"
#include "modules/config_storage.h"
#include "managers/command_manager.h"

// Instancia de configuración
AnalogSensors sensors(ANALOG_SENSOR1_PIN, ANALOG_SENSOR2_PIN, ANALOG_SENSOR3_PIN);
SDLogger sdLogger;
ConfigStorage configStorage;
CommandManager commandManager(sensors, sdLogger, configStorage);
void init_logger(){
    #if USE_LOGGER
    // Inicializar logger con nivel predeterminado
    LogInit(INFO);
    // Configurar niveles por módulo (opcional)
    LogSetModuleLevel("ANALOG", DEBUG);
    LogSetModuleLevel("SD", ERROR);
    LogSetModuleLevel("PIXHAWK", ERROR);
    LogSetModuleLevel("SONAR", ERROR);
    LOG_INFO("MAIN", "Sistema datalogger iniciando...");
    #endif
}

void setup() {
    // Inicializar comunicación serial
    Serial.begin(115200);
    while (!Serial && millis() < 5000) {
    }
    init_logger();

    commandManager.begin();

    LOG_INFO("MAIN", "Configuración completada");
    LOG_INFO("MAIN", "Escriba 'help' para ver comandos disponibles");
}

void loop() {

    // Procesar comandos seriales (siempre activo)
    commandManager.update();
}