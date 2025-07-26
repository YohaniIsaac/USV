# Tutorial: Instalación de VS Code, PlatformIO y Configuración del Proyecto

Este tutorial te guiará paso a paso para instalar Visual Studio Code, PlatformIO y configurar el proyecto para trabajar con ESP32.

##  Paso 1: Instalación de Visual Studio Code

### 1.1 Descargar VS Code
1. Ve a la página oficial: [https://code.visualstudio.com/](https://code.visualstudio.com/)
2. Haz clic en **"Download for [tu sistema operativo]"**
3. Descarga el instalador apropiado para tu sistema

### 1.2 Instalar VS Code

**En Windows:**
1. Ejecuta el archivo `.exe` descargado
2. Acepta los términos y condiciones
3. **IMPORTANTE**: Marca la opción "Add to PATH" durante la instalación
4. Completa la instalación con las opciones predeterminadas

### 1.3 Verificar Instalación
1. Abre Visual Studio Code
2. Deberías ver la pantalla de bienvenida
3.  **VS Code instalado correctamente**

---

##   Paso 2: Instalación de PlatformIO

### 2.1 Instalar la Extensión PlatformIO
1. **Abre VS Code**
2. Haz clic en el icono de **Extensiones** en la barra lateral (o presiona `Ctrl+Shift+X`)
3. En el cuadro de búsqueda, escribe: `PlatformIO IDE`
4. Busca la extensión oficial **"PlatformIO IDE"** (desarrollada por PlatformIO)
5. Haz clic en **"Install"**

### 2.2 Esperar la Instalación Completa
- La instalación puede tomar varios minutos
- PlatformIO descargará automáticamente:
  - Compiladores para ESP32
  - Frameworks de Arduino
  - Herramientas de depuración
- **NO cierres VS Code** durante este proceso

### 2.3 Verificar Instalación
1. Después de la instalación, verás el icono de **PlatformIO** en la barra lateral izquierda
2. Haz clic en el icono de PlatformIO
3. Deberías ver el "PlatformIO Home" con opciones como:
   - Projects
   - Libraries
   - Boards
   - Platforms

4.   **PlatformIO instalado correctamente**

---

##   Paso 3: Configuración del Proyecto

### 3.1 Clonar o Descargar el Repositorio

**Opción A: Usando Git (recomendado)**
```bash
git clone https://github.com/YohaniIsaac/USV.git
cd USV
```

Nota: debes tener `git` instalado.

**Opción B: Descarga directa**
1. Descarga el archivo ZIP del repositorio
2. Extrae el contenido en una carpeta de tu elección

### 3.2 Abrir el Proyecto en PlatformIO

#### Método 1: Desde PlatformIO Home
1. Abre VS Code
2. Haz clic en el icono de **PlatformIO**
3. En PlatformIO Home, haz clic en **"Open Project"**
4. Navega hasta la carpeta del proyecto clonado
5. Selecciona la carpeta **"datalogger"** o **"sonar"** (según el proyecto que quieras abrir)
6. Haz clic en **"Open"**

#### Método 2: Desde File Menu
1. En VS Code, ve a **File > Open Folder** (`Ctrl+K Ctrl+O`)
2. Navega hasta la carpeta del proyecto
3. Selecciona la carpeta **"datalogger"** o **"sonar"**
4. Haz clic en **"Select Folder"**

### 3.3 Verificar la Estructura del Proyecto
Una vez abierto, deberías ver una estructura similar a:
```
datalogger/
├── src/
│   └── main.cpp
├── include/
├── lib/
├── platformio.ini
└── .gitignore
```

### 3.4 Verificar platformio.ini
1. Abre el archivo `platformio.ini` en la raíz del proyecto
2. Verifica que contenga algo como:
```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
```

---

##   Paso 4: Compilar y Flashear el Código

### 4.1 Conectar la ESP32
1. **Conecta tu ESP32** al computador usando un cable USB en el conector para flashear, LA ESP DEBE ESTAR DESENERGIZADA.
2. **Verificar conexión**:
   - En Windows: Ve a Administrador de Dispositivos > Puertos (COM y LPT)
   - En macOS/Linux: Ejecuta `ls /dev/tty*` en terminal

### 4.2 Compilar el Proyecto
1. En VS Code, verás la **barra de herramientas de PlatformIO** en la parte inferior
2. Haz clic en el icono **"Build"** (✓) o presiona `Ctrl+Alt+B`
3. Espera a que compile (primera vez puede tardar varios minutos)
4. Si todo está bien, verás: `SUCCESS`

### 4.3 Subir el Código (Flash)
1. **Asegúrate de que la ESP32 esté conectada**
2. En la barra de PlatformIO, haz clic en **"Upload"** (→) o presiona `Ctrl+Alt+U`
3. El código se compilará y subirá automáticamente
4. Si todo está bien, verás: `SUCCESS`

### 4.4 Monitorear la Salida Serial
1. Haz clic en **"Serial Monitor"** en la barra de PlatformIO
2. Deberías ver los mensajes de debug del programa
3. Para cerrar el monitor: `Ctrl+C`

---

##   Solución de Problemas Comunes

### Problema: "Device not found" al flashear
**Soluciones:**
1. Verifica que el cable USB transmita datos (no solo carga)
2. Instala los drivers CH340/CP2102 según tu ESP32
3. Cambia el puerto COM en `platformio.ini`:
```ini
upload_port = COM3  ; Windows
```

### Problema: PlatformIO no aparece
**Soluciones:**
1. Reinicia VS Code completamente
2. Ve a Extensions y verifica que PlatformIO esté habilitado
3. Reinstala la extensión PlatformIO

### Problema: Librerías faltantes
**Soluciones:**
1. PlatformIO debería instalar automáticamente las librerías listadas en `platformio.ini`
2. Si faltan, ve a PlatformIO Home > Libraries y busca la librería específica

---

##   Verificación Final

Cuando hayas completado todos los pasos, deberías poder:

- [x] Abrir VS Code
- [x] Ver el icono de PlatformIO en la barra lateral
- [x] Abrir cualquiera de los dos proyectos (datalogger/sonar)
- [x] Compilar el código sin errores
- [x] Subir código a la ESP32
- [x] Ver mensajes en el monitor serial