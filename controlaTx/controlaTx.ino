#include <SPI.h>
#include <RF24.h>
#include <nRF24L01.h>

#define J_IX 4
#define J_IY 5
#define J_DX 6
#define J_DY 7

#define CE_PIN 38
#define CSN_PIN 39

#define SWITCH_PIN 8  // Pin para cambiar modo

// Configuración para recibir datos de emergencia
#define NRF_CHANNEL_NORMAL 124     // Canal para control normal
#define NRF_CHANNEL_EMERGENCY 76   // Canal del sistema de emergencia

const int IX_CT = 2045; // Valor central de IX
const int IX_DZ = 20;   // Zona muerta de IX

const int IY_CT = 2045; // Valor central de IY
const int IY_DZ = 20;   // Zona muerta de IY

const int DX_CT = 2045; // Valor central de DX
const int DX_DZ = 20;   // Zona muerta de DX

const int DY_CT = 2045; // Valor central de DY
const int DY_DZ = 20;   // Zona muerta de DY

// Transmisión normal
RF24 radio(CE_PIN, CSN_PIN);  // Constructor normal
const byte NRF_Address[6] = "USVRX"; // Dirección para la comunicación

typedef struct DataPacket {
  int CH1;
  int CH2;
  int CH3;
  int CH4;
} DataPacket;

DataPacket dataToSend;

// Estructura para recibir datos de emergencia
typedef struct __attribute__((packed)) EmergencyPacket {
  uint8_t header;      // 0xEE para identificar paquetes de emergencia
  float latitude;      // 4 bytes
  float longitude;     // 4 bytes
  float altitude;      // 4 bytes
  uint8_t satellites;  // 1 byte
  uint32_t timestamp;  // 4 bytes (millis)
  float voltage;       // 4 bytes (voltaje que causó la emergencia)
  uint8_t checksum;    // 1 byte
} EmergencyPacket;

int potPins[4] = {J_IX, J_IY, J_DX, J_DY};
int potValues[4];

unsigned long lastSent = 0;
unsigned long lastEmergencyReceived = 0;
const unsigned long interval = 20; // ms

// Recepción de emergencia
byte rxAddress[6] = {'E', 'M', 'R', 'G', '1', '\0'};
EmergencyPacket emergencyReceived;

// Variables para control de modo
bool currentSwitchState = HIGH;
bool lastSwitchState = HIGH;
bool isTransmitterMode = true;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50; // ms

void setTransmitterMode() {
  radio.stopListening();
  radio.setChannel(NRF_CHANNEL_NORMAL);
  radio.setDataRate(RF24_250KBPS);
  radio.openWritingPipe(NRF_Address);
  isTransmitterMode = true;
  Serial.println("=== MODO TRANSMISOR ACTIVADO ===");
}

void setReceiverMode() {
  radio.setChannel(NRF_CHANNEL_EMERGENCY);
  radio.setDataRate(RF24_1MBPS);
  radio.setAutoAck(false);
  radio.setRetries(0, 0);
  radio.openReadingPipe(1, rxAddress);
  radio.startListening();
  isTransmitterMode = false;
  Serial.println("=== MODO RECEPTOR EMERGENCIA ACTIVADO ===");
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESTE ES EL CÓDIGO DEL EMISOR V1.3");

  // Configura los pines personalizados para VSPI
  SPI.begin(36, 37, 35, CSN_PIN);  // SCK, MISO, MOSI, SS

  // Este paso es esencial para que la librería use el SPI ya inicializado
  radio.begin();
  radio.setPALevel(RF24_PA_MAX);
  radio.setAutoAck(false);
  radio.setRetries(0, 0);
  setTransmitterMode(); // Iniciar en modo transmisor

  pinMode(CSN_PIN, OUTPUT);

  // Configurar pin del switch como entrada con pull-up interno
  pinMode(SWITCH_PIN, INPUT_PULLUP);

  for (int i = 0; i < 4; i++) {
    pinMode(potPins[i], INPUT);
  }

  if (radio.isChipConnected()) {
    Serial.println("NRF24L01 conectado y SPI funcionando");
  } else {
    Serial.println("Error: NRF24L01 no conectado o SPI falló");
  }
}

void loop() {
  // Manejo del switch con debouncing
  bool reading = digitalRead(SWITCH_PIN);

  if (reading != lastSwitchState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != currentSwitchState) {
      currentSwitchState = reading;
      if (currentSwitchState == HIGH && !isTransmitterMode) {
        setTransmitterMode();
      } else if (currentSwitchState == LOW && isTransmitterMode) {
        setReceiverMode();
      }
    }
  }
  lastSwitchState = reading;
  // Ejecutar función según el modo actual
  if (isTransmitterMode) {
    transmitterLoop();
  } else {
    emergencyReceiverLoop();
  }
}

void transmitterLoop() {
  unsigned long now = millis();
  if (now - lastSent >= interval) {
    unsigned long delta = now - lastSent;
    lastSent = now;

    // Leer los valores de los 4 potenciómetros
    for (int i = 0; i < 4; i++) {
      potValues[i] = analogRead(potPins[i]);
    }

    int IX, IY, DX, DY;

    // Mapeo de IX (canal 1)
    if (potValues[0] < IX_CT - IX_DZ) {
      IX = map(potValues[0], 0, IX_CT - IX_DZ, 1000, 1500);
    }
    else if (potValues[0] > IX_CT + IX_DZ) {
      IX = map(potValues[0], IX_CT + IX_DZ, 4095, 1500, 2000);
    }
    else {
      IX = 1500;
    }

    // Mapeo de IY (canal 2)
    if (potValues[1] < IY_CT - IY_DZ) {
      IY = map(potValues[1], 0, IY_CT - IY_DZ, 1000, 1500);
    }
    else if (potValues[1] > IY_CT + IY_DZ) {
      IY = map(potValues[1], IY_CT + IY_DZ, 4095, 1500, 2000);
    }
    else {
      IY = 1500;
    }

    // Mapeo de DX (canal 3)
    if (potValues[2] < DX_CT - DX_DZ) {
      DX = map(potValues[2], 0, DX_CT - DX_DZ, 1000, 1500);
    }
    else if (potValues[2] > DX_CT + DX_DZ) {
      DX = map(potValues[2], DX_CT + DX_DZ, 4095, 1500, 2000);
    }
    else {
      DX = 1500;
    }

    // Mapeo de DY (canal 4)
    if (potValues[3] < DY_CT - DY_DZ) {
      DY = map(potValues[3], 0, DY_CT - DY_DZ, 1000, 1500);
    }
    else if (potValues[3] > DY_CT + DY_DZ) {
      DY = map(potValues[3], DY_CT + DY_DZ, 4095, 1500, 2000);
    }
    else {
      DY = 1500;
    }

    // Asignación de los valores PWM para cada canal
    int pwm_CH1 = IX;
    int pwm_CH2 = IY;
    int pwm_CH3 = DX;
    int pwm_CH4 = DY;

    // Constrain los valores PWM a un rango de 1000 a 2000
    pwm_CH1 = constrain(pwm_CH1, 1000, 2000);
    pwm_CH2 = constrain(pwm_CH2, 1000, 2000);
    pwm_CH3 = constrain(pwm_CH3, 1000, 2000);
    pwm_CH4 = constrain(pwm_CH4, 1000, 2000);

    // Asignación de los valores al paquete de datos
    dataToSend.CH1 = pwm_CH1;
    dataToSend.CH2 = pwm_CH2;
    dataToSend.CH3 = pwm_CH3;
    dataToSend.CH4 = pwm_CH4;

    // Enviar los datos a través del radio
    bool sent = radio.write(&dataToSend, sizeof(dataToSend));

    // Mostrar los valores leídos y enviados en el monitor serial
    Serial.print("Lecturas: ");
    for (int i = 0; i < 4; i++) {
      Serial.print(potValues[i]);
      if (i < 3) Serial.print(", ");
    }

    Serial.print(" (");
    Serial.print(pwm_CH1);
    Serial.print(", ");
    Serial.print(pwm_CH2);
    Serial.print(", ");
    Serial.print(pwm_CH3);
    Serial.print(", ");
    Serial.print(pwm_CH4);
    Serial.print(")");

    Serial.print(sent ? " - Datos enviados correctamente" : " - Fallo al enviar");
    Serial.print(" - Δt: ");
    Serial.print(delta);
    Serial.println(" ms");
  }
}

void emergencyReceiverLoop() {
  if (radio.available()) {
    radio.read(&emergencyReceived, sizeof(emergencyReceived));
    lastEmergencyReceived = millis();
    
    // Validar el paquete de emergencia
    if (validateEmergencyPacket()) {
      displayEmergencyData();
    } else {
      Serial.println("RX-EMERGENCY - Paquete inválido recibido");
    }
  } else {
    // Mostrar estado de espera
    unsigned long now = millis();
    if (now - lastEmergencyReceived > 10000 && lastEmergencyReceived > 0) {
      // Sin datos por más de 10 segundos
      static unsigned long lastTimeoutMsg = 0;
      if (now - lastTimeoutMsg > 5000) {
        Serial.println("RX-EMERGENCY - Esperando señal de emergencia...");
        lastTimeoutMsg = now;
      }
    } else if (lastEmergencyReceived == 0) {
      // Primer arranque
      static unsigned long lastWaitingMsg = 0;
      if (now - lastWaitingMsg > 3000) {
        Serial.println("RX-EMERGENCY - Modo emergencia activo. Esperando datos...");
        lastWaitingMsg = now;
      }
    }
  }
}

void displayEmergencyData() {
  Serial.println("=== DATOS DE EMERGENCIA RECIBIDOS ===");
  
  // Información de voltaje
  Serial.println("Voltaje crítico: " + String(emergencyReceived.voltage, 3) + "V");
  
  // Información GPS
  if (emergencyReceived.satellites > 0) {
    Serial.println("   UBICACIÓN GPS:");
    Serial.println("   Latitud: " + String(emergencyReceived.latitude, 6) + "°");
    Serial.println("   Longitud: " + String(emergencyReceived.longitude, 6) + "°");
    Serial.println("   Altitud: " + String(emergencyReceived.altitude, 1) + "m");
    Serial.println("   Satélites: " + String(emergencyReceived.satellites));
    
    // Mostrar coordenadas en formato fácil de copiar
    Serial.println("  Coordenadas: " + String(emergencyReceived.latitude, 6) + 
                   "," + String(emergencyReceived.longitude, 6));
  } else {
    Serial.println("  GPS: Sin señal de satélites");
  }
  
  Serial.println("==========================================");
}

bool validateEmergencyPacket() {
  // Verificar header
  if (emergencyReceived.header != 0xEE) {
    return false;
  }
  
  // Verificar checksum
  uint8_t calculatedChecksum = 0;
  uint8_t* data = (uint8_t*)&emergencyReceived;
  
  for (size_t i = 0; i < sizeof(EmergencyPacket) - 1; i++) {
    calculatedChecksum ^= data[i];
  }
  
  return (calculatedChecksum == emergencyReceived.checksum);
}