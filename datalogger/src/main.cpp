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

// Variables de control de tiempo
unsigned long lastDataLog = 0;
unsigned long lastStatusDisplay = 0;
const unsigned long DATA_LOG_INTERVAL = 2000;    // Cada 2 segundos
const unsigned long STATUS_DISPLAY_INTERVAL = 10000; // Mostrar estado cada 10 segundos

void init_logger(){
#if USE_LOGGER
    // Inicializar logger con nivel predeterminado
    LogInit(INFO);
    
    // Configurar niveles por módulo
    LogSetModuleLevel("MAIN", INFO);
    LogSetModuleLevel("EMERGENCY", INFO);
    LogSetModuleLevel("ANALOG", INFO);
    LogSetModuleLevel("SD_LOGGER", INFO);
    LogSetModuleLevel("SONAR_RX", INFO);
    LogSetModuleLevel("PIXHAWK", INFO);
    LogSetModuleLevel("CMD", WARN);
    
    LOG_INFO("MAIN", "Sistema datalogger iniciando...");
#endif // USE_LOGGER
}

String createCSVHeader() {
    String header = "Timestamp,";
    
    // 1. Datos del sonar
    header += sonar.getCSVHeader() + ",";
    
    // 2. Sensores analógicos
    header += "pH,DO,EC,pHRaw,DORaw,ECRaw,";
    
    // 3. Datos de Pixhawk
    header += "Latitude,Longitude,Altitude,Heading,BattVoltage,BattCurrent,BattRemaining,BattTemp,GroundSpeed,AirSpeed,NumSats,";
    
    // 4. Sistema de emergencia
    header += "EmergencyActive,SystemVoltage,PowerControlActive";
    
    return header;
}

String collectAllData() {
    String data = "";
    
    // Timestamp
    data += String(millis()) + ",";
    
    // 1. Datos del sonar
    data += sonar.getCSVData() + ",";
    
    // 2. Sensores analógicos
    data += String(sensors.lastPH, 3) + ",";
    data += String(sensors.lastDO, 3) + ",";
    data += String(sensors.lastEC, 1) + ",";
    data += String(sensors.lastRawPH) + ",";
    data += String(sensors.lastRawDO) + ",";
    data += String(sensors.lastRawEC) + ",";
    
    // 3. Datos de Pixhawk
    data += pixhawk.save_CSVData() + ",";
    
    // 4. Sistema de emergencia
    data += String(emergencySystem.isEmergencyActive() ? 1 : 0) + ",";
    data += String(emergencySystem.getCurrentVoltage(), 3) + ",";
    data += String(emergencySystem.isPowerControlActive() ? 1 : 0);
    
    return data;
}

void displaySystemStatus() {
    LOG_INFO("MAIN", "=================== ESTADO DEL SISTEMA ===================");
    
    // Estado de emergencia
    LOG_INFO("MAIN", "  SISTEMA DE EMERGENCIA:");
    LOG_INFO("MAIN", "  Voltaje: " + String(emergencySystem.getCurrentVoltage(), 3) + "V");
    LOG_INFO("MAIN", "  Emergencia: " + String(emergencySystem.isEmergencyActive() ? "ACTIVA" : "OK"));
    LOG_INFO("MAIN", "  Control alimentación: " + String(emergencySystem.isPowerControlActive() ? "NORMAL" : "EMERGENCIA"));
    
    // Estado del sonar
    LOG_INFO("MAIN", "SONAR:");
    if (sonar.hasValidData()) {
        LOG_INFO("MAIN", "  Estado: CONECTADO");
        LOG_INFO("MAIN", "  Profundidad: " + String(sonar.getDepth(), 2) + "m");
        LOG_INFO("MAIN", "  Muestras: " + String(sonar.getSampleCount()));
        LOG_INFO("MAIN", "  Packets válidos: " + String(sonar.getValidPacketsReceived()));
    } else {
        LOG_WARN("MAIN", "  Estado: SIN DATOS VÁLIDOS");
        LOG_INFO("MAIN", "  Total packets: " + String(sonar.getTotalPacketsReceived()));
        LOG_INFO("MAIN", "  Errores: " + String(sonar.getErrorPacketsReceived()));
    }
    
    // Sensores analógicos
    LOG_INFO("MAIN", "  SENSORES ANALÓGICOS:");
    LOG_INFO("MAIN", "  pH: " + String(sensors.lastPH, 2) + " (raw: " + String(sensors.lastRawPH) + ")");
    LOG_INFO("MAIN", "  DO: " + String(sensors.lastDO, 2) + " mg/L (raw: " + String(sensors.lastRawDO) + ")");
    LOG_INFO("MAIN", "  EC: " + String(sensors.lastEC, 0) + " μS/cm (raw: " + String(sensors.lastRawEC) + ")");
    
    // Pixhawk
    LOG_INFO("MAIN", "  PIXHAWK:");
    LOG_INFO("MAIN", "  Posición: " + String(pixhawk.getLatitude(), 6) + "°, " + String(pixhawk.getLongitude(), 6) + "°");
    LOG_INFO("MAIN", "  Altitud: " + String(pixhawk.getAltitude(), 1) + "m");
    LOG_INFO("MAIN", "  Batería: " + String(pixhawk.getBatteryVoltage(), 2) + "V (" + String(pixhawk.getBatteryRemaining()) + "%)");
    LOG_INFO("MAIN", "  Satélites: " + String(pixhawk.getNumSatellites()));
    
    LOG_INFO("MAIN", "=========================================================");
}

void setup() {
    // Inicializar comunicación serial
    Serial.begin(115200);
    while (!Serial && millis() < 5000) {
    }
    delay(100);

    // Inicializar sistema de logging
    init_logger();

    LOG_INFO("MAIN", "=== SISTEMA DATALOGGER ESP32-S3 ===");
    LOG_INFO("MAIN", "Intervalo de captura: " + String(DATA_LOG_INTERVAL) + "ms");
    LOG_INFO("MAIN", "=====================================");
        
    // Inicializar comandos del monitor serial
    LOG_INFO("MAIN", "Inicializando sistema de comandos...");
    commandManager.begin();
    LOG_INFO("MAIN", "Sistema de comandos listo");

    // Inicializar sistema de emergencia (PRIORITARIO)
    LOG_INFO("MAIN", "Inicializando sistema de emergencia...");
    emergencySystem.begin();
    LOG_INFO("MAIN", "Sistema de emergencia listo");
    
    // Inicializar sensores analógicos
    LOG_INFO("MAIN", "Inicializando sensores analógicos...");
    sensors.begin();
    LOG_INFO("MAIN", "Sensores analógicos listos");
    
    // Inicializar receptor de sonar
    LOG_INFO("MAIN", "Inicializando receptor de sonar...");
    if (sonar.begin()) {
        LOG_INFO("MAIN", "Receptor de sonar listo");
    } else {
        LOG_ERROR("MAIN", "Error al inicializar receptor de sonar");
    }
    
    // Inicializar interfaz Pixhawk
    LOG_INFO("MAIN", "Inicializando interfaz Pixhawk");
    pixhawk.begin();
    LOG_INFO("MAIN", "Interfaz Pixhawk lista");
    
    // Inicializar tarjeta SD
    LOG_INFO("MAIN", "Inicializando tarjeta SD");
    if (micro_sd.begin()) {
        // Crear header del CSV con todos los datos
        String csvHeader = createCSVHeader();
        micro_sd.writeHeader(csvHeader);
        LOG_INFO("MAIN", "Tarjeta SD lista");
        LOG_DEBUG("MAIN", "Header CSV: " + csvHeader);
    } else {
        LOG_ERROR("MAIN", "Error al inicializar tarjeta SD");
    }
    
    LOG_INFO("MAIN", "");
    LOG_INFO("MAIN", " SISTEMA LISTO - Iniciando captura de datos");
    LOG_INFO("MAIN", " Datos se guardarán cada " + String(DATA_LOG_INTERVAL/1000) + " segundos");
    LOG_INFO("MAIN", " Estado se mostrará cada " + String(STATUS_DISPLAY_INTERVAL/1000) + " segundos");
    LOG_INFO("MAIN", "");
    
    // Mostrar estado inicial
    displaySystemStatus();

}

void loop() {
    unsigned long currentTime = millis();
    
    // ===== ACTUALIZAR TODOS LOS MÓDULOS =====
    // Procesar comandos seriales
    commandManager.update();

    // Sistema de emergencia
    emergencySystem.update();
    
    // Actualizar sensores analógicos
    sensors.update();
    
    // Actualizar receptor de sonar
    sonar.update();
    
    // Actualizar interfaz Pixhawk
    pixhawk.update();
    
    // ===== CAPTURA Y ALMACENAMIENTO DE DATOS =====
    if (currentTime - lastDataLog >= DATA_LOG_INTERVAL) {
        LOG_DEBUG("MAIN", "Capturando datos");
        
        // Recopilar todos los datos
        String allData = collectAllData();
        
        // Escribir a SD
        micro_sd.writeData(allData);
        micro_sd.update();
        
        LOG_INFO("MAIN", "Datos guardados en SD");
        LOG_VERBOSE("MAIN", "Datos: " + allData);
        
        lastDataLog = currentTime;
    }
    
    // ===== MOSTRAR ESTADO DEL SISTEMA =====
    if (currentTime - lastStatusDisplay >= STATUS_DISPLAY_INTERVAL) {
        displaySystemStatus();
        lastStatusDisplay = currentTime;
    }
    
    // Pequeña pausa para no saturar el procesador
    delay(50);
}