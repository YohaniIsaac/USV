#include "emergency_system.h"

EmergencySystem::EmergencySystem() : gpsSerial(1) {  // Usar UART1 en ESP32 (compartido con Pixhawk)
    emergencyActive = false;
    lastCheckTime = 0;
}

void EmergencySystem::begin() {
    // Configurar pin de emergencia con resistencia pull-up interna
    pinMode(EMERGENCY_PIN, INPUT_PULLUP);
    
    // Configurar pin para controlar la alimentación del GPS
    pinMode(GPS_POWER_PIN, OUTPUT);
    digitalWrite(GPS_POWER_PIN, LOW);  // GPS inicialmente apagado
}

void EmergencySystem::update() {
    unsigned long currentTime = millis();
    
    // Verificar el pin de emergencia a la frecuencia configurada
    if (currentTime - lastCheckTime >= EMERGENCY_CHECK_RATE) {
        // El pin normalmente está en HIGH, si cambia a LOW, se activa la emergencia
        if (digitalRead(EMERGENCY_PIN) == LOW && !emergencyActive) {
            emergencyActive = true;
            activateEmergencyGPS();
        }
        
        // Si la emergencia está activa, enviar señal periódicamente
        if (emergencyActive) {
            sendEmergencySignal();
        }
        
        lastCheckTime = currentTime;
    }
}

void EmergencySystem::activateEmergencyGPS() {
    // Encender el GPS de respaldo
    digitalWrite(GPS_POWER_PIN, HIGH);
    
    // Inicializar la comunicación serial con el GPS
    gpsSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    
    Serial.println("¡EMERGENCIA ACTIVADA! GPS de respaldo encendido.");
}

void EmergencySystem::sendEmergencySignal() {
    // Esta función enviaría una señal de emergencia a través del GPS
    // La implementación específica dependerá del GPS utilizado
    
    // Ejemplo simplificado
    Serial.println("Enviando señal de emergencia...");
    
    // Aquí se implementaría el código para enviar la señal de emergencia
    // a través del GPS
}

bool EmergencySystem::isEmergencyActive() {
    return emergencyActive;
}