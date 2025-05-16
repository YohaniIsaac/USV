#include "managers/command_manager.h"

CommandManager::CommandManager(AnalogSensors& sensors, SDLogger& sdLogger, ConfigStorage& configStorage)
    : sensors(sensors), sdLogger(sdLogger), configStorage(configStorage) {
    enableAnalogSensors = true;
    enableSDLogging = false;
    enableSerialOutput = true;
}

void CommandManager::begin() {
    // Cargar estado de módulos desde la configuración
    enableAnalogSensors = configStorage.loadModuleState("analogSensors", true);
    enableSDLogging = configStorage.loadModuleState("sdLogging", false);
    enableSerialOutput = configStorage.loadModuleState("serialOutput", true);
    
    Serial.println("Sistema de comandos inicializado");
    Serial.println("Escriba 'help' para ver comandos disponibles");
}

void CommandManager::update() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        processCommand(command);
    }
}

void CommandManager::processCommand(String command) {
    // Comando para calibrar pH
    if (command.startsWith("cal_ph")) {
        if (!enableAnalogSensors) {
            Serial.println("Error: Los sensores analógicos están deshabilitados");
            return;
        }
        
        float knownPH = command.substring(7).toFloat();
        int rawValue = sensors.readRawPH();
        sensors.calibratePH(knownPH, rawValue);
        Serial.print("pH calibrado a ");
        Serial.print(knownPH);
        Serial.print(" (valor crudo: ");
        Serial.print(rawValue);
        Serial.println(")");
        
        // Guardar calibración
        if (sensors.saveCalibration(configStorage)) {
            Serial.println("Calibración guardada en memoria");
        }
    }
    
    // Comando para calibrar DO
    else if (command.startsWith("cal_do")) {
        if (!enableAnalogSensors) {
            Serial.println("Error: Los sensores analógicos están deshabilitados");
            return;
        }
        
        float knownDO = command.substring(7).toFloat();
        int rawValue = sensors.readRawDO();
        sensors.calibrateDO(knownDO, rawValue);
        Serial.print("Oxígeno disuelto calibrado a ");
        Serial.print(knownDO);
        Serial.print(" mg/L (valor crudo: ");
        Serial.print(rawValue);
        Serial.println(")");
        
        // Guardar calibración
        if (sensors.saveCalibration(configStorage)) {
            Serial.println("Calibración guardada en memoria");
        }
    }
    
    // Comando para calibrar EC
    else if (command.startsWith("cal_ec")) {
        if (!enableAnalogSensors) {
            Serial.println("Error: Los sensores analógicos están deshabilitados");
            return;
        }
        
        float knownEC = command.substring(7).toFloat();
        int rawValue = sensors.readRawEC();
        sensors.calibrateEC(knownEC, rawValue);
        Serial.print("Conductividad calibrada a ");
        Serial.print(knownEC);
        Serial.print(" μS/cm (valor crudo: ");
        Serial.print(rawValue);
        Serial.println(")");
        
        // Guardar calibración
        if (sensors.saveCalibration(configStorage)) {
            Serial.println("Calibración guardada en memoria");
        }
    }
    
    // Comandos para habilitar/deshabilitar módulos
    else if (command == "enable_analog") {
        setAnalogEnabled(true);
        Serial.println("Sensores analógicos habilitados");
    }
    else if (command == "disable_analog") {
        setAnalogEnabled(false);
        Serial.println("Sensores analógicos deshabilitados");
    }
    else if (command == "enable_sd") {
        setSDLoggingEnabled(true);
        Serial.println("Logging SD habilitado");
    }
    else if (command == "disable_sd") {
        setSDLoggingEnabled(false);
        Serial.println("Logging SD deshabilitado");
    }
    else if (command == "enable_serial") {
        setSerialOutputEnabled(true);
        Serial.println("Salida serial habilitada");
    }
    else if (command == "disable_serial") {
        setSerialOutputEnabled(false);
        Serial.println("Salida serial deshabilitada (excepto comandos)");
    }
    
    // Comando de reset
    else if (command == "reset_config") {
        if (configStorage.clearAllSettings()) {
            Serial.println("Configuración restablecida a valores predeterminados");
        }
    }
    
    // Comando para mostrar datos
    else if (command == "show_data") {
        displaySensorData();
    }
    
    // Comando de ayuda
    else if (command == "help") {
        displayHelp();
    }
}

void CommandManager::setAnalogEnabled(bool enabled) {
    enableAnalogSensors = enabled;
    configStorage.saveModuleState("analogSensors", enabled);
}

void CommandManager::setSDLoggingEnabled(bool enabled) {
    enableSDLogging = enabled;
    configStorage.saveModuleState("sdLogging", enabled);
}

void CommandManager::setSerialOutputEnabled(bool enabled) {
    enableSerialOutput = enabled;
    configStorage.saveModuleState("serialOutput", enabled);
}

void CommandManager::displayHelp() {
    Serial.println("\nComandos disponibles:");
    Serial.println("  cal_ph X     - Calibrar sensor de pH con valor conocido X");
    Serial.println("  cal_do X     - Calibrar sensor de oxígeno disuelto con valor conocido X (mg/L)");
    Serial.println("  cal_ec X     - Calibrar sensor de conductividad con valor conocido X (μS/cm)");
    Serial.println("  enable_analog - Habilitar sensores analógicos");
    Serial.println("  disable_analog - Deshabilitar sensores analógicos");
    Serial.println("  enable_sd     - Habilitar registro en SD");
    Serial.println("  disable_sd    - Deshabilitar registro en SD");
    Serial.println("  enable_serial - Habilitar salida serial de datos");
    Serial.println("  disable_serial - Deshabilitar salida serial de datos");
    Serial.println("  reset_config  - Restablecer toda la configuración");
    Serial.println("  show_data     - Mostrar lecturas actuales de sensores");
    Serial.println("  help          - Mostrar esta ayuda\n");
}

void CommandManager::displaySensorData() {
    if (!enableAnalogSensors) {
        Serial.println("Los sensores analógicos están deshabilitados");
        return;
    }
    
    // Leer valores crudos
    int phRaw = sensors.readRawPH();
    int doRaw = sensors.readRawDO();
    int ecRaw = sensors.readRawEC();
    
    // Leer valores convertidos
    float ph = sensors.getLastPH();
    float dissolvedOxygen = sensors.getLastDO();
    float conductivity = sensors.getLastEC();
    
    // Mostrar datos
    Serial.println("----------------------------------------");
    Serial.println("       LECTURAS DE SENSORES");
    Serial.println("----------------------------------------");
    Serial.print("pH: ");
    Serial.print(ph, 2);
    Serial.print(" (raw: ");
    Serial.print(phRaw);
    Serial.println(")");
    
    Serial.print("Oxígeno Disuelto: ");
    Serial.print(dissolvedOxygen, 2);
    Serial.print(" mg/L (raw: ");
    Serial.print(doRaw);
    Serial.println(")");
    
    Serial.print("Conductividad: ");
    Serial.print(conductivity, 0);
    Serial.print(" μS/cm (raw: ");
    Serial.print(ecRaw);
    Serial.println(")");
    Serial.println("----------------------------------------");
}