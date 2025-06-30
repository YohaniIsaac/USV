// Parser para protocolo propietario del sonar
// ESP32-S3 + MCP2515 + Sonar con protocolo CAN propietario

#include <SPI.h>
#include <mcp_can.h>

#define CAN_CS_PIN 10
MCP_CAN CAN0(CAN_CS_PIN);

unsigned long lastHeartbeat = 0;
unsigned long messageCount = 0;

void parseDepthData(unsigned char *data, unsigned char len);
void parseAlternateData(unsigned char *data, unsigned char len);
void printRawMessage(unsigned long canId, unsigned char len, unsigned char *data);

void setup() {
  Serial.begin(115200);
  delay(3000);
  
  Serial.println("=======================================");
  Serial.println("Sonar Propietario - CAN Parser");
  Serial.println("ESP32-S3 + MCP2515");
  Serial.println("=======================================");
  
  SPI.begin();
  delay(100);
  
  if (CAN0.begin(CAN_250KBPS, MCP_8MHz) == CAN_OK) {
    Serial.println("✅ MCP2515 inicializado en 250kbps");
    Serial.println("🔍 Analizando protocolo del sonar...");
    Serial.println("---------------------------------------");
  } else {
    Serial.println("❌ Error inicializando MCP2515");
    while(1) delay(1000);
  }
  
  lastHeartbeat = millis();
}

void loop() {
  if (CAN0.checkReceive() == CAN_MSGAVAIL) {
    unsigned long canId;
    unsigned char len;
    unsigned char data[8];
    
    if (CAN0.readMsgBuf(&len, data) == CAN_OK) {
      canId = CAN0.getCanId();
      messageCount++;
      
      // Analizar mensajes del sonar
      if (canId == 0xDF50B00) {
        parseDepthData(data, len);
      } else if (canId == 0x15FD0800) {
        parseAlternateData(data, len);
      } else {
        // Otros mensajes
        printRawMessage(canId, len, data);
      }
    }
  }
  
  // Heartbeat cada 10 segundos
  if (millis() - lastHeartbeat > 10000) {
    lastHeartbeat = millis();
    Serial.print("💓 Mensajes procesados: ");
    Serial.println(messageCount);
  }
  
  delay(10);
}

void parseDepthData(unsigned char *data, unsigned char len) {
  if (len >= 8) {
    uint8_t sequence = data[0];
    
    // Garmin Intelliducer - Protocol analysis
    // Bytes 1-2: Posible profundidad (little endian)
    uint16_t rawDepth = (uint16_t(data[2]) << 8) | data[1];
    
    // Solo mostrar si no está en secuencias de calibración (FF)
    if (data[1] != 0xFF || data[2] != 0xFF) {
      Serial.print("🌊 GARMIN [");
      Serial.print(sequence, HEX);
      Serial.print("] - Raw Depth: ");
      Serial.print(rawDepth);
      
      // Probar diferentes escalas comunes
      Serial.print(" | Escalas: ");
      Serial.print(rawDepth * 0.1f, 1);  // decímetros -> metros
      Serial.print("m, ");
      Serial.print(rawDepth * 0.01f, 2); // centímetros -> metros  
      Serial.print("m, ");
      Serial.print(rawDepth * 0.001f, 3); // milímetros -> metros
      Serial.print("m");
      
      // Temperatura en otros bytes?
      if (len >= 6 && (data[4] != 0xFF || data[5] != 0xFF)) {
        uint16_t rawTemp = (uint16_t(data[5]) << 8) | data[4];
        Serial.print(" | Temp?: ");
        Serial.print(rawTemp * 0.01f - 273.15f, 1); // Kelvin a Celsius
        Serial.print("°C");
      }
      
      Serial.print(" | Raw: ");
      for (int i = 0; i < len; i++) {
        if (data[i] < 16) Serial.print("0");
        Serial.print(data[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
  }
}

void parseAlternateData(unsigned char *data, unsigned char len) {
  Serial.print("📊 ALT [");
  Serial.print(data[0], HEX);
  Serial.print("] - Data: ");
  
  for (int i = 0; i < len; i++) {
    if (data[i] < 16) Serial.print("0");
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

void printRawMessage(unsigned long canId, unsigned char len, unsigned char *data) {
  Serial.print("📡 ID: 0x");
  Serial.print(canId, HEX);
  Serial.print(" | Data: ");
  
  for (int i = 0; i < len; i++) {
    if (data[i] < 16) Serial.print("0");
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}