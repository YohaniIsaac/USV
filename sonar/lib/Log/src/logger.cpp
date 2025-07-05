#include "logger.h"
#include "config.h"

#if USE_LOGGER

// Inicialización de variables estáticas
LogLevel Logger::_defaultLevel = INFO;
bool Logger::_logToSerial = true;
String Logger::_moduleNames[MAX_MODULES] = {};
LogLevel Logger::_moduleLevels[MAX_MODULES] = {};
int Logger::_moduleCount = 0;

void Logger::init(LogLevel defaultLevel, bool useSerial) {
    _defaultLevel = defaultLevel;
    _logToSerial = useSerial;
    _moduleCount = 0;
}

void Logger::setLogLevel(LogLevel level) {
    _defaultLevel = level;
}

void Logger::setModuleLogLevel(const String& module, LogLevel level) {
    // Buscar si el módulo ya existe
    for (int i = 0; i < _moduleCount; i++) {
        if (_moduleNames[i] == module) {
            _moduleLevels[i] = level;
            return;
        }
    }
    
    // Si hay espacio, añadir nuevo módulo
    if (_moduleCount < MAX_MODULES) {
        _moduleNames[_moduleCount] = module;
        _moduleLevels[_moduleCount] = level;
        _moduleCount++;
    }
}

bool Logger::isLevelEnabled(const String& module, LogLevel level) {
    return level <= getModuleLevel(module);
}

LogLevel Logger::getModuleLevel(const String& module) {
    // Buscar nivel específico para este módulo
    for (int i = 0; i < _moduleCount; i++) {
        if (_moduleNames[i] == module) {
            return _moduleLevels[i];
        }
    }
    
    // Si no se encuentra, usar el nivel predeterminado
    return _defaultLevel;
}

void Logger::setLogToSerial(bool enable) {
    _logToSerial = enable;
}

void Logger::error(const String& module, const String& message) {
    log(ERROR, module, message);
}

void Logger::warn(const String& module, const String& message) {
    log(WARN, module, message);
}

void Logger::info(const String& module, const String& message) {
    log(INFO, module, message);
}

void Logger::debug(const String& module, const String& message) {
    log(DEBUG, module, message);
}

void Logger::verbose(const String& module, const String& message) {
    log(VERBOSE, module, message);
}

void Logger::log(LogLevel level, const String& module, const String& message) {
    // Verificar si este nivel de log está habilitado para este módulo
    LogLevel moduleLevel = getModuleLevel(module);
    if (level > moduleLevel) {
        return;
    }
    
    // Obtener tiempo actual en milisegundos
    unsigned long now = millis();
    
    // Formatear mensaje de log
    String logMessage = "[" + String(now) + "] ";
    logMessage += levelToString(level) + " ";
    logMessage += "[" + module + "] ";
    logMessage += message;
    
    // Enviar a salidas habilitadas
    if (_logToSerial) {
        Serial.println(logMessage);
    }
}

String Logger::levelToString(LogLevel level) {
    switch (level) {
        case ERROR:
            return "ERROR";
        case WARN:
            return "WARN ";
        case INFO:
            return "INFO ";
        case DEBUG:
            return "DEBUG";
        case VERBOSE:
            return "VERB ";
        default:
            return "NONE ";
    }
}

#endif // USE_LOGGER