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

    // Inicar siste de emergencia
    emergencySystem.begin();

    // Iniciar recepción de datos del sonar
    sonar.begin();

    // Iniciar los sensores
    sensors.begin();

    // Iniciar SD
    micro_sd.begin();
    micro_sd.writeHeader("Ph, PH crudo");

    // Inicializar interfaz Pixhawk
    pixhawk.begin();


}

void loop() {
    unsigned long currentTime = millis();
    commandManager.update();

    // ===== SISTEMA DE EMERGENCIA =====
    emergencySystem.update();

    // ===== LECTURA DE SENSORES Y SONAR ===== 
    if (currentTime - lastEmergencyDisplay >= 2000) {
        sensors.update();
        sonar.update();
    }

    /* PIX */
    // Actualizar interfaz Pixhawk
    pixhawk.update();

    delay(100);

}