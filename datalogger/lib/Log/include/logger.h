#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include "config.h"

// Niveles de Log
enum LogLevel {
    NONE = 0,   // Sin logs
    ERROR = 1,  // Solo errores críticos
    WARN = 2,   // Errores y advertencias
    INFO = 3,   // Información general (default)
    DEBUG = 4,  // Información detallada para depuración
    VERBOSE = 5 // Información muy detallada
};



class Logger {
// Declaración condicional de la clase Logger
#if USE_LOGGER
public:
    // Inicializar logger
    static void init(LogLevel defaultLevel = INFO, bool useSerial = true);
    
    // Establecer nivel de log global
    static void setLogLevel(LogLevel level);
    
    // Establecer nivel de log por módulo
    static void setModuleLogLevel(const String& module, LogLevel level);
    
    // Verificar si un nivel está habilitado para un módulo
    static bool isLevelEnabled(const String& module, LogLevel level);
    
    // Establecer destinos de log
    static void setLogToSerial(bool enable);
    
    // Métodos de logging
    static void error(const String& module, const String& message);
    static void warn(const String& module, const String& message);
    static void info(const String& module, const String& message);
    static void debug(const String& module, const String& message);
    static void verbose(const String& module, const String& message);
    
private:
    static LogLevel _defaultLevel;
    static bool _logToSerial;
    
    // Mapa para almacenar niveles por módulo
    static const int MAX_MODULES = 10;
    static String _moduleNames[MAX_MODULES];
    static LogLevel _moduleLevels[MAX_MODULES];
    static int _moduleCount;
    
    // Método interno para realizar el log
    static void log(LogLevel level, const String& module, const String& message);
    
    // Método para convertir nivel a texto
    static String levelToString(LogLevel level);
    
    // Obtener nivel para un módulo específico
    static LogLevel getModuleLevel(const String& module);
#endif // USE_LOGGER
};


// Funciones de envoltorio (wrapper) disponibles independientemente de USE_LOGGER
// Estas funciones siempre existen pero compilan a nada cuando USE_LOGGER=0

// Inicializar el sistema de logging
inline void LogInit(LogLevel level = INFO, bool useSerial = true) {
    #if USE_LOGGER
    Logger::init(level, useSerial);
    #endif
}

// Establecer nivel para un módulo
inline void LogSetModuleLevel(const String& module, LogLevel level) {
    #if USE_LOGGER
    Logger::setModuleLogLevel(module, level);
    #endif
}

// Establecer nivel global
inline void LogSetLevel(LogLevel level) {
    #if USE_LOGGER
    Logger::setLogLevel(level);
    #endif
}

// Macros para logging (funcionan independientemente de USE_LOGGER)
#if USE_LOGGER
#define LOG_ERROR(module, message) \
    do { \
        if (Logger::isLevelEnabled(module, ERROR)) { \
            Logger::error(module, message); \
        } \
    } while(0)

#define LOG_WARN(module, message) \
    do { \
        if (Logger::isLevelEnabled(module, WARN)) { \
            Logger::warn(module, message); \
        } \
    } while(0)

#define LOG_INFO(module, message) \
    do { \
        if (Logger::isLevelEnabled(module, INFO)) { \
            Logger::info(module, message); \
        } \
    } while(0)

#define LOG_DEBUG(module, message) \
    do { \
        if (Logger::isLevelEnabled(module, DEBUG)) { \
            Logger::debug(module, message); \
        } \
    } while(0)

#define LOG_VERBOSE(module, message) \
    do { \
        if (Logger::isLevelEnabled(module, VERBOSE)) { \
            Logger::verbose(module, message); \
        } \
    } while(0)

#else // USE_LOGGER == 0
// Si logger está deshabilitado, las macros no hacen nada
#define LOG_ERROR(module, message)
#define LOG_WARN(module, message)
#define LOG_INFO(module, message)
#define LOG_DEBUG(module, message)
#define LOG_VERBOSE(module, message)

#endif // USE_LOGGER

#endif // LOGGER_H