#include <SPI.h>
#include <SD.h>

const int chipSelect = 5;  // Pin CS conectado a GPIO 5

void setup() {
  // Inicializa la comunicación serial
  Serial.begin(115200);
  
  // Inicializa la tarjeta SD
  Serial.print("Inicializando SD card...");

  if (!SD.begin(chipSelect)) {
    Serial.println("¡Fallo en la inicialización!");
    return;
  }
  Serial.println("Inicialización correcta.");

  // Abre el archivo (si no existe, lo crea)
  File dataFile = SD.open("/datalog.txt", FILE_WRITE);

  // Si el archivo se abrió correctamente, escribe en él
  if (dataFile) {
    Serial.println("Escribiendo en datalog.txt...");
    dataFile.println("Este es un dato de prueba.");
    dataFile.close();
    Serial.println("Escritura completada.");
  } else {
    // Si el archivo no se pudo abrir, muestra un error
    Serial.println("Error al abrir datalog.txt");
  }
}

void loop() {
  // No es necesario hacer nada en el loop para este ejemplo
}