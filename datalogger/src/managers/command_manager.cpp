#include "managers/command_manager.h"

CommandManager::CommandManager(AnalogSensors& sensors) : sensors(sensors) {
}

void CommandManager::begin() {    
    // Inicializar EEPROM
    if (!EEPROMManager::begin()) {
        LOG_ERROR("CMD", "Error al inicializar EEPROM");
        return;
    }
    
    LOG_INFO("CMD", "Sistema de calibración inicializado");
    LOG_INFO("CMD", "Escriba 'help' para ver comandos disponibles");
    
    // Cargar calibraciones guardadas automáticamente
    loadCalibrationFromEEPROM();
}

void CommandManager::update() {
    // Actualizar sensores
    sensors.update();
    
    // Procesar comandos
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        processCommand(command);
    }
}

// ====================== DEFINICIÓN DE COMANDOS ======================
void CommandManager::processCommand(String command) {
// ************ COMANDOS DE CALIBRACION ************
    // Comando para calibrar pH
    if (command.startsWith("cal_ph")) {
        float knownPH = command.substring(7).toFloat();
        int rawValue = sensors.calibrateCurrentPH(knownPH);

        String msg = "pH calibrado a " + String(knownPH) + " (en el valor crudo: " + String(rawValue) + ")";
        LOG_INFO("CMD", msg);
        Serial.println(msg);  // También mostrar en serial para respuesta inmediata
    }
    
    // Comando para calibrar DO
    else if (command.startsWith("cal_do")) {
        float knownDO = command.substring(7).toFloat();
        int rawValue = sensors.calibrateCurrentDO(knownDO);
        
        String msg = "Oxígeno disuelto calibrado a " + String(knownDO) + " mg/L (valor crudo: " + String(rawValue) + ")";
        LOG_INFO("CMD", msg);
        Serial.println(msg);
    }
    
    // Comando para calibrar EC
    else if (command.startsWith("cal_ec")) {
        float knownEC = command.substring(7).toFloat();
        int rawValue = sensors.calibrateCurrentEC(knownEC);
        
        String msg = "Conductividad calibrada a " + String(knownEC) + " μS/cm (valor crudo: " + String(rawValue) + ")";
        LOG_INFO("CMD", msg);
        Serial.println(msg);
    }
    
// ************ COMANDOS CALIBRACION MANUAL ************
    // Comandos adicionales para setear offset y slope directamente
    else if (command.startsWith("set_ph_offset")) {
        float offset = command.substring(14).toFloat();
        float currentSlope = sensors.phSlope;
        sensors.setPhCalibration(offset, currentSlope);
        
        String msg = "pH offset seteado a: " + String(offset, 4);
        LOG_INFO("CMD", msg);
        Serial.println(msg);
    }
    
    else if (command.startsWith("set_ph_slope")) {
        float slope = command.substring(13).toFloat();
        float currentOffset = sensors.phOffset;
        sensors.setPhCalibration(currentOffset, slope);
        
        String msg = "pH slope seteado a: " + String(slope, 4);
        LOG_INFO("CMD", msg);
        Serial.println(msg);
    }
    
    else if (command.startsWith("set_do_offset")) {
        float offset = command.substring(14).toFloat();
        float currentSlope = sensors.doSlope;
        sensors.setDoCalibration(offset, currentSlope);
        
        String msg = "DO offset seteado a: " + String(offset, 4);
        LOG_INFO("CMD", msg);
        Serial.println(msg);
    }
    
    else if (command.startsWith("set_do_slope")) {
        float slope = command.substring(13).toFloat();
        float currentOffset = sensors.doOffset;
        sensors.setDoCalibration(currentOffset, slope);
        
        String msg = "DO slope seteado a: " + String(slope, 4);
        LOG_INFO("CMD", msg);
        Serial.println(msg);
    }
    
    else if (command.startsWith("set_ec_offset")) {
        float offset = command.substring(14).toFloat();
        float currentSlope = sensors.ecSlope;
        float currentK = sensors.ecK;
        sensors.setEcCalibration(offset, currentSlope, currentK);
        
        String msg = "EC offset seteado a: " + String(offset, 4);
        LOG_INFO("CMD", msg);
        Serial.println(msg);
    }
    
    else if (command.startsWith("set_ec_slope")) {
        float slope = command.substring(13).toFloat();
        float currentOffset = sensors.ecOffset;
        float currentK = sensors.ecK;
        sensors.setEcCalibration(currentOffset, slope, currentK);
        
        String msg = "EC slope seteado a: " + String(slope, 4);
        LOG_INFO("CMD", msg);
        Serial.println(msg);
    }

    else if (command.startsWith("set_ec_k")) {
        float k = command.substring(9).toFloat();
        float currentOffset = sensors.ecOffset;
        float currentSlope = sensors.ecSlope;
        sensors.setEcCalibration(currentOffset, currentSlope, k);
        
        String msg = "EC constante K seteada a: " + String(k, 4);
        LOG_INFO("CMD", msg);
        Serial.println(msg);
    }

// ************ COMANDOS COMUNES ************
    // Comando para mostrar datos
    else if (command == "show_data") {
        displaySensorData();
    }
    
    // Comando de ayuda
    else if (command == "help") {
        displayHelp();
    }

    else if (command == "show_cal") {
        displayCalibrationData();
    }

    // Comando no reconocido
    else {
        String msg = "Comando no reconocido: '" + command + "'. Escriba 'help' para ver comandos disponibles.";
        LOG_WARN("CMD", msg);
        Serial.println(msg);
    }
}

// ====================== MENSAJE SENSORES ANALOGOS ======================
// ====================== calibraciones y datos ======================
void CommandManager::displaySensorData() {
    // Leer valores crudos
    int phRaw = sensors.lastRawPH;
    int doRaw = sensors.lastRawDO;
    int ecRaw = sensors.lastRawEC;
    
    // Leer valores convertidos
    float ph = sensors.lastPH;
    float dissolvedOxygen = sensors.lastDO;
    float conductivity = sensors.lastEC;
    
    // Mostrar datos
    Serial.println("========================================");
    Serial.println("         LECTURAS DE SENSORES");
    Serial.println("========================================");
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
    Serial.println("========================================");
    
    String logMsg = "Datos mostrados - pH: " + String(ph, 2) + 
                   ", DO: " + String(dissolvedOxygen, 2) + 
                   ", EC: " + String(conductivity, 0);
    LOG_DEBUG("CMD", logMsg);
}

void CommandManager::displayCalibrationData() {
    Serial.println("\n========== VALORES DE CALIBRACIÓN ACTUALES ==========");
    
    Serial.println("pH:");
    Serial.print("  Offset: "); Serial.println(sensors.phOffset, 4);
    Serial.print("  Slope:  "); Serial.println(sensors.phSlope, 4);
    
    Serial.println("Oxígeno Disuelto (DO):");
    Serial.print("  Offset: "); Serial.println(sensors.doOffset, 4);
    Serial.print("  Slope:  "); Serial.println(sensors.doSlope, 4);
    
    Serial.println("Conductividad Eléctrica (EC):");
    Serial.print("  Offset: "); Serial.println(sensors.ecOffset, 4);
    Serial.print("  Slope:  "); Serial.println(sensors.ecSlope, 4);
    Serial.print("  K:      "); Serial.println(sensors.ecK, 4);

    Serial.println("=====================================================");
    
    Serial.println("\nFórmulas aplicadas:");
    Serial.println("pH = 7.0 + ((voltaje - 2.5) / slope) + offset");
    Serial.println("DO = voltaje * slope + offset");
    Serial.println("EC = voltaje * K * slope + offset");
    Serial.println("=====================================================\n");
    
    LOG_DEBUG("CMD", "Valores de calibración mostrados");
}

// ====================== MENSAJE DE AYUDA ======================
void CommandManager::displayHelp() {
    Serial.println("\n=================== COMANDOS DISPONIBLES ===================");
    Serial.println("CALIBRACIÓN: \t (Toma lectura y las setea al valor ingresado)");
    Serial.println("  cal_ph X     - Calibrar sensor de pH con valor conocido X");
    Serial.println("  cal_do X     - Calibrar sensor de oxígeno disuelto con valor conocido X (mg/L)");
    Serial.println("  cal_ec X     - Calibrar sensor de conductividad con valor conocido X (μS/cm)");
    Serial.println("");
    Serial.println("CALIBRACIÓN AVANZADA: \t (Setear offset y slope directamente con el valor)");
    Serial.println("  set_ph_offset X   - Setear offset de pH a X");
    Serial.println("  set_ph_slope X    - Setear slope de pH a X");
    Serial.println("  set_do_offset X   - Setear offset de DO a X");
    Serial.println("  set_do_slope X    - Setear slope de DO a X");
    Serial.println("  set_ec_offset X   - Setear offset de EC a X");
    Serial.println("  set_ec_slope X    - Setear slope de EC a X");
    Serial.println("  set_ec_k X        - Setear constante K de EC a X");
    Serial.println("");
    Serial.println("DATOS:");
    Serial.println("  show_data    - Mostrar lecturas actuales de sensores");
    Serial.println("  show         - Mostrar variables de calibracion almacenadas");
    Serial.println("  help         - Mostrar esta ayuda");
    Serial.println("==========================================================\n");
    
    LOG_DEBUG("CMD", "Ayuda mostrada");
}