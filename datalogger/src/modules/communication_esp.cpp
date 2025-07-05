#include <HardwareSerial.h>
#include "modules/communication_esp.h"
#include "logger.h"

class WROOMReceiver {
public:
    bool begin() {
        // Usar UART1 reasignado a GPIO1 (RX) y GPIO2 (TX)
        // -1 en TX porque no necesitamos transmitir
        Serial1.begin(WROOM_BAUD_RATE, SERIAL_8N1, WROOM_UART_RX_PIN, -1);
        
        LOG_INFO("WROOM_RX", "UART inicializado:");
        LOG_INFO("WROOM_RX", "  Puerto: UART1 reasignado");
        LOG_INFO("WROOM_RX", "  RX Pin: GPIO" + String(WROOM_UART_RX_PIN));
        LOG_INFO("WROOM_RX", "  Baudios: " + String(WROOM_BAUD_RATE));
        
        return true;
    }
    
    void update() {
        while (Serial1.available()) {
            String data = Serial1.readStringUntil('\n');
            data.trim();
            
            if (data.length() > 0) {
                LOG_INFO("WROOM_RX", "Recibido: " + data);
                processData(data);
            }
        }
    }
    
private:
    void processData(String data) {
        // Tu lógica de procesamiento aquí
        LOG_DEBUG("WROOM_RX", "Procesando: " + data);
    }
};