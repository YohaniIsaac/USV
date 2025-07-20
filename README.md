# Sistema de Datalogger ESP32-S3

## Datos Guardados y Transformaciones

### Formato del Archivo CSV
```
Timestamp,SonarDepth,WaterTemperature,SonarValid,pH,DO,EC,Latitude,Longitude,Altitude
```

### 1. **Timestamp**
- **Unidad**: Milisegundos desde el arranque del sistema.

### 2. **Datos del Sonar (desde ESP-WROOM)**

#### 2.1 SonarDepth
- **Unidad**: metros
- **Transformación**: Los datos llegan ya procesados desde la librería NMEA2000.

#### 2.2 WaterTemperature  
- **Unidad**: °C (grados Celsius).
- **Transformación**: Dato directo del sensor Garmin vía NMEA2000.

#### 2.3 SonarValid
- **Unidad**: 1=válido, 0=inválido.
- **Transformación**: Estado de validez de los datos del sonar.


### 3. **Sensores Analógicos DFRobot Gravity**
Este sistema utiliza transformaciones lineales aproximadas para los sensores DFRobot Gravity. Esta aproximación es válida y ampliamente utilizada en sistemas embebidos.

#### 3.1 Sensor de pH
- **Unidad**: Valor de pH (escala 0-14)
- **Sensor**: [DFRobot Gravity Analog pH Sensor V2](https://www.dfrobot.com/product-1782.html)
- **Principio**: [Ecuación de Nernst](https://atlas-scientific.com/blog/ph-slope/) - El voltaje cambia ~59.16 mV por unidad de pH a 25°C
- **Justificación técnica**: Los electrodos de pH generan un voltaje proporcional a la concentración de iones hidrógeno según la [ecuación de Nernst](https://www.hamiltoncompany.com/process-analytics/ph-and-orp-knowledge/ph-probe-operation-principles/voltage-potentials)

**Transformación implementada:**
```cpp
float phVoltage = (lastRawPH * 3.3) / 4095.0;   // ADC (0-4095) → Voltios
lastPH = 7.0 + ((phVoltage - 2.5) / phSlope) + phOffset;  // → pH
```

**Fórmula:** `pH = 7.0 + ((Voltaje - 2.5V) / Pendiente) + Offset`
- **2.5V**: Voltaje de referencia para pH neutro (7.0)
- **Pendiente**: Factor de conversión V→pH (calibrable)
- **Offset**: Corrección por asimetría del electrodo

#### 3.2 Sensor de Oxígeno Disuelto (DO)
- **Unidad**: mg/L (miligramos por litro)
- **Sensor**: [DFRobot Gravity Analog Dissolved Oxygen Sensor](https://www.dfrobot.com/product-1628.html)
- **Principio**: [Sensor galvánico/polarográfico](https://www.fondriest.com/environmental-measurements/measurements/measuring-water-quality/dissolved-oxygen-sensors-and-methods/) - Reacción electroquímica proporcional a la concentración de O₂
- **Justificación técnica**: Los sensores de DO tipo Clark generan una corriente proporcional al oxígeno que difunde a través de una membrana

**Transformación implementada:**
```cpp
float doVoltage = (lastRawDO * 3.3) / 4095.0;   // ADC → Voltios  
lastDO = doVoltage * doSlope + doOffset;         // → mg/L
```

**Fórmula:** `DO = Voltaje × Pendiente + Offset`
- **Pendiente**: Factor de conversión V → mg/L  (determinado por calibración)
- **Offset**: Corrección de calibración

#### 3.3 Sensor de Conductividad Eléctrica (EC)
- **Unidad**: μS/cm (microsiemens por centímetro)
- **Sensor**: [DFRobot Gravity Analog EC Sensor V2 (K=1)](https://www.dfrobot.com/product-1123.html)
- **Principio**: [Constante de celda](https://atlas-scientific.com/blog/conductivity-probe-cell-constants/) - Conductividad específica = Conductividad medida × Constante de celda

**Transformación:**
```cpp
float ecVoltage = (lastRawEC * 3.3) / 4095.0;   // ADC → Voltios
lastEC = ecVoltage * ecK * ecSlope + ecOffset;   // → μS/cm
```

**Fórmula:** `EC = Voltaje × K × Pendiente + Offset`
- **K (ecK)**: Constante de celda del sensor (1/cm)
- **Pendiente**: Factor de conversión del sensor
- **Offset**: Corrección de calibración

### 4. **Datos de Pixhawk (MAVLink)**

#### 4.1 Coordenadas GPS
- **Latitude/Longitude**: Grados decimales
- **Altitude**: metros

**Transformación desde protocolo MAVLink:**
```cpp
// Los datos MAVLink vienen como int32 en grados * 1E7
int32_t lat = *((int32_t*)&payload[8]);
int32_t lon = *((int32_t*)&payload[12]);
int32_t alt = *((int32_t*)&payload[16]);

// Conversión a unidades estándar
latitude = lat / 1e7;        // int32*1E7 → grados decimales
longitude = lon / 1e7;       // int32*1E7 → grados decimales  
altitude = alt / 1000.0;     // mm → metros
```

## Valores de Calibración por Defecto

### Sin Calibración (Valores por Defecto)
Los sensores analógicos usan estos valores cuando no hay calibración guardada en EEPROM:

```cpp
phOffset = 0.0;
phSlope = 3.3 / 4095.0 * 3.5;   // ≈ 0.00282 V/pH
doOffset = 0.0;
doSlope = 1.0;                  // 1.0 mg/L por Volt
ecOffset = 0.0;
ecSlope = 1.0;
ecK = 10.0;                     // Constante de celda típica
```

## Cómo Usar el Sistema

### Paso 1: Preparar la Computadora
1. **Descargar Arduino IDE**: Ir a [arduino.cc](https://www.arduino.cc/en/software) y descargar el programa
2. **Instalar Arduino IDE**: Ejecutar el instalador y seguir las instrucciones
3. **Conectar el datalogger**: Usar cable USB para conectar el ESP32-S3 a la computadora, esto energizará únicamente la ESP (no los sensores)
4. **Energizar los sensores**: para poder usar calibrar, es necesario energizar únicamente los sensores (NO CONECTAR ALIMENTACIÓN DE BATERÍAS EN LA ESP), luego de esto, reinciar la esp para que se configuren correctamente los sensores.
5. **Abrir Monitor Serial**: En Arduino IDE, ir a Herramientas → Monitor Serie
6. **Configurar velocidad**: Seleccionar 115200 baudios en la esquina inferior derecha

### Paso 2: Ver Comandos Disponibles
Escribir `help` en el monitor serial y presionar Enter para ver todos los comandos disponibles:

```
help
```

### Lista Completa de Comandos

####    **Comandos de Información**
```
show_data       - Muestra las lecturas actuales de pH, DO y EC
show_cal        - Muestra los valores de calibración guardados
help            - Muestra esta lista de comandos
```

####    **Comandos de Calibración Simple**
```
cal_ph 7.0      - Calibra el sensor de pH (poner sensor en solución pH 7.0)
cal_ph 4.0      - Calibra el sensor de pH (poner sensor en solución pH 4.0)
cal_do 8.5      - Calibra oxígeno disuelto (poner sensor en agua con oxígeno conocido)
cal_ec 1413     - Calibra conductividad (poner sensor en solución 1413 μS/cm)
```

####    **Comandos de Calibración Avanzada**
```
set_ph_offset 0.1    - Ajusta manualmente el offset del pH
set_ph_slope 0.059   - Ajusta manualmente la pendiente del pH
set_do_offset 0.0    - Ajusta manualmente el offset del oxígeno disuelto
set_do_slope 4.0     - Ajusta manualmente la pendiente del oxígeno disuelto
set_ec_offset 0.0    - Ajusta manualmente el offset de conductividad
set_ec_slope 1.0     - Ajusta manualmente la pendiente de conductividad
set_ec_k 1.0         - Ajusta la constante de celda del sensor EC
```

####    **Comandos de Gestión**
```
reset_cal       - Borra toda la calibración y vuelve a valores originales
clear_eeprom    - Borra completamente la memoria del sistema
```

### Ejemplo de Uso Paso a Paso

1. **Ver estado actual**: Escribir `show_data` para ver las lecturas
2. **Calibrar pH**: 
   - Poner sensor en solución pH 7.0
   - Escribir `cal_ph 7.0`
   - Poner sensor en solución pH 4.0  
   - Escribir `cal_ph 4.0`
3. **Verificar calibración**: Escribir `show_cal` para confirmar que se guardó
4. **Probar**: Escribir `show_data` para ver nuevas lecturas calibradas



