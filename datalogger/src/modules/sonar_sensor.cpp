#include "modules/sonar_sensor.h"

SonarSensor::SonarSensor() : sonarSerial(2) {  // Usar UART2 en ESP32
    depth = 0.0;
    lastUpdateTime = 0;
}

void SonarSensor::begin() {
    sonarSerial.begin(9600, SERIAL_8N1, SONAR_RX_PIN, SONAR_TX_PIN);
}

void SonarSensor::update() {
    unsigned long currentTime = millis();
    
    // Actualizar a la frecuencia configurada
    if (currentTime - lastUpdateTime >= SONAR_SAMPLING_RATE) {
        if (sonarSerial.available()) {
            String data = sonarSerial.readStringUntil('\n');
            parseData(data);
        }
        
        lastUpdateTime = currentTime;
    }
}

void SonarSensor::parseData(String data) {
    // La implementación específica dependerá del protocolo del sensor sonar
    // Este es un ejemplo genérico que asume una cadena simple con la profundidad
    
    // Ejemplo: data = "D123.45" donde 123.45 es la profundidad en metros
    if (data.startsWith("D")) {
        depth = data.substring(1).toFloat();
    }
}

float SonarSensor::getDepth() {
    return depth;
}

String SonarSensor::getCSVHeader() {
    return "depth";
}

String SonarSensor::getCSVData() {
    return String(depth);
}