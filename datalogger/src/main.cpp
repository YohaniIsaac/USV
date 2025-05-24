#include <Arduino.h>
#include "config.h"
#include "logger.h"
#include "modules/analog_sensors.h"
#include "managers/command_manager.h"

// Instancia de configuración PROBADO Y CONFIRMADO
AnalogSensors sensors(ANALOG_SENSOR1_PIN, ANALOG_SENSOR2_PIN, ANALOG_SENSOR3_PIN);
CommandManager commandManager(sensors);

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

    LOG_INFO("MAIN", "Configuración completada");
    LOG_INFO("MAIN", "Escriba 'help' para ver comandos disponibles");
}

void loop() {
    // Procesar lectura de sensores 
    sensors.update();
    // Procesar comandos seriales (siempre activo)
    commandManager.update();

}