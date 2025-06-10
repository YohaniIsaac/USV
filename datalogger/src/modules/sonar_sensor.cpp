#include "modules/sonar_sensor.h"
#include "logger.h"

SonarSensor::SonarSensor() : sonarSerial(2) {  // Usar UART2
    depth = 0.0;
    temperature = 0.0;
    speed = 0.0;
    dataValid = false;
    lastUpdateTime = 0;
    lastDataReceived = 0;
    inputBuffer = "";
    lastRawData = "";
}

void SonarSensor::begin() {
    // Inicializar el puerto serie para el sonar
    sonarSerial.begin(SONAR_BAUD_RATE, SERIAL_8N1, SONAR_RX_PIN, SONAR_TX_PIN);
    sonarSerial.setTimeout(100);  // Timeout de 100ms para readStringUntil
    
    // Resetear datos
    resetData();
    
    LOG_INFO("SONAR", "Sensor sonar inicializado");
    LOG_INFO("SONAR", "Puerto: UART2, Baudios: " + String(SONAR_BAUD_RATE));
    LOG_INFO("SONAR", "Pines RX: " + String(SONAR_RX_PIN) + ", TX: " + String(SONAR_TX_PIN));
}

void SonarSensor::update() {
    unsigned long currentTime = millis();
    
    // Verificar si han pasado más de SONAR_UPDATE_RATE ms desde la última actualización
    if (currentTime - lastUpdateTime < SONAR_UPDATE_RATE) {
        return;
    }
    
    // Leer datos disponibles del puerto serie
    while (sonarSerial.available() > 0) {
        String receivedData = sonarSerial.readStringUntil('\n');
        receivedData.trim();  // Eliminar espacios y caracteres de nueva línea
        
        if (receivedData.length() > 0) {
            lastRawData = receivedData;  // Guardar para diagnóstico
            lastDataReceived = currentTime;
            
            LOG_DEBUG("SONAR", "Datos recibidos: " + receivedData);
            
            // Procesar los datos
            parseData(receivedData);
        }
    }
    
    // Verificar timeout - si no recibimos datos por mucho tiempo, marcar como inválidos
    if (isTimeoutExpired()) {
        if (dataValid) {
            LOG_WARN("SONAR", "Timeout - marcando datos como inválidos");
            dataValid = false;
        }
    }
    
    lastUpdateTime = currentTime;
}

void SonarSensor::parseData(String data) {
    // El módulo wcmcu-230 puede enviar datos en diferentes formatos
    // Intentamos detectar el formato automáticamente
    
    if (data.startsWith("$")) {
        // Formato NMEA
        parseNMEAData(data);
    } else if (data.indexOf("DEPTH") >= 0 || data.indexOf("TEMP") >= 0) {
        // Formato Garmin personalizado
        parseGarminData(data);
    } else {
        // Intentar parsear como datos separados por comas
        LOG_DEBUG("SONAR", "Intentando parsear formato CSV: " + data);
        
        int firstComma = data.indexOf(',');
        int secondComma = data.indexOf(',', firstComma + 1);
        
        if (firstComma > 0) {
            float newDepth = parseFloatValue(data.substring(0, firstComma));
            if (isValidDepth(newDepth)) {
                depth = newDepth;
                dataValid = true;
                LOG_INFO("SONAR", "Profundidad actualizada: " + String(depth, 2) + " m");
            }
            
            if (secondComma > firstComma) {
                float newTemp = parseFloatValue(data.substring(firstComma + 1, secondComma));
                if (isValidTemperature(newTemp)) {
                    temperature = newTemp;
                    LOG_INFO("SONAR", "Temperatura actualizada: " + String(temperature, 2) + " °C");
                }
            }
        }
    }
}

void SonarSensor::parseGarminData(String data) {
    LOG_DEBUG("SONAR", "Parseando datos Garmin: " + data);
    
    // Buscar patrones típicos de Garmin
    // Ejemplo: "DEPTH:12.5,TEMP:18.3"
    
    int depthPos = data.indexOf("DEPTH:");
    if (depthPos >= 0) {
        int startPos = depthPos + 6;  // Longitud de "DEPTH:"
        int endPos = data.indexOf(',', startPos);
        if (endPos == -1) endPos = data.indexOf(' ', startPos);
        if (endPos == -1) endPos = data.length();
        
        String depthStr = data.substring(startPos, endPos);
        float newDepth = parseFloatValue(depthStr);
        
        if (isValidDepth(newDepth)) {
            depth = newDepth;
            dataValid = true;
            LOG_INFO("SONAR", "Profundidad Garmin: " + String(depth, 2) + " m");
        }
    }
    
    int tempPos = data.indexOf("TEMP:");
    if (tempPos >= 0) {
        int startPos = tempPos + 5;  // Longitud de "TEMP:"
        int endPos = data.indexOf(',', startPos);
        if (endPos == -1) endPos = data.indexOf(' ', startPos);
        if (endPos == -1) endPos = data.length();
        
        String tempStr = data.substring(startPos, endPos);
        float newTemp = parseFloatValue(tempStr);
        
        if (isValidTemperature(newTemp)) {
            temperature = newTemp;
            LOG_INFO("SONAR", "Temperatura Garmin: " + String(temperature, 2) + " °C");
        }
    }
}

void SonarSensor::parseNMEAData(String data) {
    LOG_DEBUG("SONAR", "Parseando datos NMEA: " + data);
    
    // Ejemplo de sentencias NMEA típicas para profundidad:
    // $SDDPT,12.5,0.0*hh  (Depth)
    // $SDMTW,18.3,C*hh    (Water Temperature)
    
    if (data.startsWith("$SDDPT") || data.startsWith("$GPDPT")) {
        // Sentencia de profundidad
        int firstComma = data.indexOf(',');
        int secondComma = data.indexOf(',', firstComma + 1);
        
        if (firstComma > 0 && secondComma > firstComma) {
            String depthStr = data.substring(firstComma + 1, secondComma);
            float newDepth = parseFloatValue(depthStr);
            
            if (isValidDepth(newDepth)) {
                depth = newDepth;
                dataValid = true;
                LOG_INFO("SONAR", "Profundidad NMEA: " + String(depth, 2) + " m");
            }
        }
    }
    
    if (data.startsWith("$SDMTW") || data.startsWith("$GPMTW")) {
        // Sentencia de temperatura del agua
        int firstComma = data.indexOf(',');
        int secondComma = data.indexOf(',', firstComma + 1);
        
        if (firstComma > 0 && secondComma > firstComma) {
            String tempStr = data.substring(firstComma + 1, secondComma);
            float newTemp = parseFloatValue(tempStr);
            
            if (isValidTemperature(newTemp)) {
                temperature = newTemp;
                LOG_INFO("SONAR", "Temperatura NMEA: " + String(temperature, 2) + " °C");
            }
        }
    }
}

float SonarSensor::parseFloatValue(String str) {
    str.trim();
    if (str.length() == 0) return 0.0;
    
    // Convertir string a float
    float value = str.toFloat();
    
    // Verificar si la conversión fue exitosa
    if (value == 0.0 && !str.startsWith("0")) {
        LOG_WARN("SONAR", "Error al convertir valor: '" + str + "'");
        return 0.0;
    }
    
    return value;
}

bool SonarSensor::isValidDepth(float depth_val) {
    // Validar rango razonable de profundidad (0 a 1000 metros)
    return (depth_val >= 0.0 && depth_val <= 1000.0);
}

bool SonarSensor::isValidTemperature(float temp_val) {
    // Validar rango razonable de temperatura (-5°C a 50°C)
    return (temp_val >= -5.0 && temp_val <= 50.0);
}

void SonarSensor::resetData() {
    depth = 0.0;
    temperature = 0.0;
    speed = 0.0;
    dataValid = false;
    lastDataReceived = millis();
    inputBuffer = "";
    
    LOG_DEBUG("SONAR", "Datos del sensor reiniciados");
}

bool SonarSensor::isTimeoutExpired() {
    unsigned long currentTime = millis();
    return (currentTime - lastDataReceived) > SONAR_TIMEOUT;
}

// ====================== GETTERS ======================

float SonarSensor::getDepth() {
    return depth;
}

float SonarSensor::getTemperature() {
    return temperature;
}

float SonarSensor::getSpeed() {
    return speed;
}

bool SonarSensor::isDataValid() {
    return dataValid && !isTimeoutExpired();
}

unsigned long SonarSensor::getLastUpdateTime() {
    return lastDataReceived;
}

String SonarSensor::getLastRawData() {
    return lastRawData;
}

// ====================== CSV FUNCTIONS ======================

String SonarSensor::getCSVHeader() {
    return "Profundidad(m),Temperatura(C),Velocidad(m/s),SonarValido";
}

String SonarSensor::getCSVData() {
    String data = "";
    data += String(depth, 2) + ",";
    data += String(temperature, 2) + ",";
    data += String(speed, 2) + ",";
    data += String(isDataValid() ? 1 : 0);
    
    return data;
}

/* TESTS */

bool SonarSensor::detectBaudRate() {
    LOG_INFO("SONAR", "Iniciando detección automática de velocidad de baudios...");
    
    // Velocidades comunes para sensores sonar
    uint32_t baudRates[] = {1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200};
    int numRates = sizeof(baudRates) / sizeof(baudRates[0]);
    
    for (int i = 0; i < numRates; i++) {
        LOG_INFO("SONAR", "Probando velocidad: " + String(baudRates[i]) + " baudios");
        
        if (testBaudRate(baudRates[i])) {
            LOG_INFO("SONAR", "¡Velocidad detectada correctamente: " + String(baudRates[i]) + " baudios!");
            LOG_INFO("SONAR", "Actualiza SONAR_BAUD_RATE en config.h a " + String(baudRates[i]));
            return true;
        }
        
        delay(100); // Pausa entre pruebas
    }
    
    LOG_ERROR("SONAR", "No se pudo detectar una velocidad válida");
    return false;
}

bool SonarSensor::testBaudRate(uint32_t baudRate) {
    // Reinicializar el puerto serie con la nueva velocidad
    sonarSerial.end();
    delay(50);
    sonarSerial.begin(baudRate, SERIAL_8N1, SONAR_RX_PIN, SONAR_TX_PIN);
    sonarSerial.setTimeout(100);
    delay(1000); // Dar tiempo para que se estabilice
    sonarSerial.println("S5"); // 250 kbps for NMEA 2000
    delay(100);
    sonarSerial.println("O");  // Open channel
    delay(100);
    
    // Limpiar buffer
    while (sonarSerial.available()) {
        sonarSerial.read();
    }
    int totalBytes = 0;
    int validDataCount = 0;
    int totalAttempts = 0;
    unsigned long startTime = millis();
    String hexData = "";
    
    // Probar durante 3 segundos
    while (millis() - startTime < 3000 && totalAttempts < 10) {
        if (sonarSerial.available() > 0) {
            uint8_t rawByte = sonarSerial.read();
            totalBytes++;

                        // Convertir byte a hexadecimal
            if (rawByte < 16) hexData += "0";
            hexData += String(rawByte, HEX);
            hexData += " ";
            
            // Mostrar en grupos de 16 bytes por línea
            if (totalBytes % 16 == 0) {
                LOG_INFO("SONAR", "RAW: " + hexData);
                hexData = "";
            }
            
            // También mostrar el valor decimal y si es un carácter imprimible
            String byteInfo = "Byte " + String(totalBytes) + ": 0x" + 
                             String(rawByte, HEX) + " (" + String(rawByte) + ")";
            if (rawByte >= 32 && rawByte <= 126) {
                byteInfo += " = '" + String((char)rawByte) + "'";
            }
            LOG_DEBUG("SONAR", byteInfo);

            
            String response = Serial2.readString();
            LOG_INFO("SONAR", "Module response: " + response);
            String data = sonarSerial.readStringUntil('\n');
            data.trim();
            
            if (data.length() > 0) {
                totalAttempts++;
                LOG_DEBUG("SONAR", "Baudios " + String(baudRate) + " - Datos: " + data);
                
                if (isValidSonarData(data)) {
                    validDataCount++;
                    LOG_DEBUG("SONAR", "Datos válidos encontrados en " + String(baudRate) + " baudios");
                }
            }
        }
        delay(50);
    }
    
    // Considerar exitoso si al menos 30% de los datos son válidos
    bool success = (totalAttempts > 0) && (validDataCount * 100 / totalAttempts >= 30);
    
    LOG_INFO("SONAR", "Baudios " + String(baudRate) + " - Intentos: " + String(totalAttempts) + 
             ", Válidos: " + String(validDataCount) + " (" + 
             String(totalAttempts > 0 ? validDataCount * 100 / totalAttempts : 0) + "%)");
    
    return success;
}

bool SonarSensor::isValidSonarData(String data) {
    // Verificar que no esté vacío
    if (data.length() == 0) return false;
    
    // Contar caracteres imprimibles vs caracteres de control/corruptos
    int printableChars = 0;
    int controlChars = 0;
    
    for (int i = 0; i < data.length(); i++) {
        char c = data.charAt(i);
        
        // Caracteres válidos para datos de sonar
        if ((c >= '0' && c <= '9') ||  // Números
            (c >= 'A' && c <= 'Z') ||  // Letras mayúsculas
            (c >= 'a' && c <= 'z') ||  // Letras minúsculas
            c == '.' || c == ',' ||     // Separadores decimales y CSV
            c == ':' || c == '-' ||     // Separadores comunes
            c == '$' || c == '*' ||     // Caracteres NMEA
            c == ' ' || c == '\t') {    // Espacios
            printableChars++;
        } else {
            controlChars++;
        }
    }
    
    // Considerar válido si al menos 80% son caracteres imprimibles
    bool hasGoodRatio = (printableChars * 100 / data.length()) >= 80;
    
    // Verificar patrones comunes de datos de sonar
    bool hasNumbers = false;
    for (int i = 0; i < data.length(); i++) {
        if (data.charAt(i) >= '0' && data.charAt(i) <= '9') {
            hasNumbers = true;
            break;
        }
    }
    
    // Verificar si parece NMEA
    bool looksLikeNMEA = data.startsWith("$") && data.indexOf(',') > 0;
    
    // Verificar si parece CSV con números
    bool looksLikeCSV = hasNumbers && data.indexOf(',') > 0;
    
    // Verificar si parece formato Garmin
    bool looksLikeGarmin = data.indexOf("DEPTH") >= 0 || data.indexOf("TEMP") >= 0;
    
    bool isValid = hasGoodRatio && hasNumbers && 
                   (looksLikeNMEA || looksLikeCSV || looksLikeGarmin);
    
    if (isValid) {
        LOG_DEBUG("SONAR", "Datos VÁLIDOS detectados: " + data.substring(0, min(50, (int)data.length())));
    }
    
    return isValid;
}