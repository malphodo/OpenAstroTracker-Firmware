#pragma once

/**
 * Language support for OpenAstroTracker
 * Translation file for menu titles and messages
 */

enum Language {
    LANG_FRENCH = 0,
    LANG_ENGLISH = 1,
    LANG_SPANISH = 2,
    LANG_ITALIAN = 3,
    LANG_GERMAN = 4,
    LANG_COUNT = 5
};

extern Language currentLanguage;

// Main menu titles
static const char* MENU_TITLES[LANG_COUNT][30] = {
    // French
    {
        "RA",           // 0
        "DEC",          // 1
        "GO",           // 2
        "HA",           // 3
        "CTRL",         // 4
        "CAL",          // 5
        "FOC",          // 6
        "Informations", // 7
        "Configuration", // 8
        "Aller à",      // 9 - GoTo
        "Sortir",       // 10 - Exit
        "Sauvegarder",  // 11 - Save
        "Accueil",      // 12 - Home
        "Parquer",      // 13 - Park
        "Déparquer",    // 14 - Unpark
        "Ajustement",   // 15 - Adjustment
        "Polar",        // 16 - Polar
        "Vitesse",      // 17 - Speed
        "Limites",      // 18 - Limits
        "Niveau",       // 19 - Level
        "Cible",        // 20 - Target
        "Actuel",       // 21 - Current
        "Afficher",     // 22 - Show
        "Stocké",       // 23 - Stored
        "Gyro",         // 24 - Gyro
        "AutoPA",       // 25 - Auto Polar Align
        "AutoHome",     // 26 - Auto Home
        "GPS",          // 27 - GPS
        "ON",           // 28 - ON
        "OFF"           // 29 - OFF
    },
    // English
    {
        "RA",           // 0
        "DEC",          // 1
        "GO",           // 2
        "HA",           // 3
        "CTRL",         // 4
        "CAL",          // 5
        "FOC",          // 6
        "INFO",         // 7
        "Configuration", // 8
        "GoTo",         // 9
        "Exit",         // 10
        "Save",         // 11
        "Home",         // 12
        "Park",         // 13
        "Unpark",       // 14
        "Adjustment",   // 15
        "Polar",        // 16
        "Speed",        // 17
        "Limits",       // 18
        "Level",        // 19
        "Target",       // 20
        "Current",      // 21
        "Show",         // 22
        "Stored",       // 23
        "Gyro",         // 24 - Gyro
        "AutoPA",       // 25 - Auto Polar Align
        "AutoHome",     // 26 - Auto Home
        "GPS",          // 27 - GPS
        "ON",           // 28 - ON
        "OFF"           // 29 - OFF
    },
    // Spanish
    {
        "RA",           // 0
        "DEC",          // 1
        "IR",           // 2 - GO becomes IR (Ir)
        "HA",           // 3
        "CTRL",         // 4
        "CAL",          // 5
        "FOC",          // 6
        "INFO",         // 7
        "Configuración", // 8
        "Ir a",         // 9 - GoTo
        "Salir",        // 10 - Exit
        "Guardar",      // 11 - Save
        "Inicio",       // 12 - Home
        "Aparcar",      // 13 - Park
        "Desaparcar",   // 14 - Unpark
        "Ajuste",       // 15 - Adjustment
        "Polar",        // 16 - Polar
        "Velocidad",    // 17 - Speed
        "Límites",      // 18 - Limits
        "Nivel",        // 19 - Level
        "Objetivo",     // 20 - Target
        "Actual",       // 21 - Current
        "Mostrar",      // 22 - Show
        "Guardado",     // 23 - Stored
        "Giroscopio",   // 24 - Gyro
        "AlinePA",      // 25 - Auto Polar Align
        "AutoInicio",   // 26 - Auto Home
        "GPS",          // 27 - GPS
        "ON",           // 28 - ON
        "OFF"           // 29 - OFF
    },
    // Italian
    {
        "RA",           // 0
        "DEC",          // 1
        "VAI",          // 2 - GO becomes VAI (Go)
        "HA",           // 3
        "CTRL",         // 4
        "CAL",          // 5
        "FOC",          // 6
        "INFO",         // 7
        "Configurazione", // 8
        "Vai a",        // 9 - GoTo
        "Esci",         // 10 - Exit
        "Salva",        // 11 - Save
        "Home",         // 12 - Home
        "Parcheggia",   // 13 - Park
        "Sblocca",      // 14 - Unpark
        "Regolazione",  // 15 - Adjustment
        "Polare",       // 16 - Polar
        "Velocità",     // 17 - Speed
        "Limiti",       // 18 - Limits
        "Livello",      // 19 - Level
        "Bersaglio",    // 20 - Target
        "Corrente",     // 21 - Current
        "Mostra",       // 22 - Show
        "Salvato",      // 23 - Stored
        "Giroscopio",   // 24 - Gyro
        "AlinePA",      // 25 - Auto Polar Align
        "AutoHome",     // 26 - Auto Home
        "GPS",          // 27 - GPS
        "ON",           // 28 - ON
        "OFF"           // 29 - OFF
    },
    // German
    {
        "RA",           // 0
        "DEC",          // 1
        "GEH",          // 2 - GO becomes GEH (Go)
        "HA",           // 3
        "CTRL",         // 4
        "CAL",          // 5
        "FOC",          // 6
        "INFO",         // 7
        "Konfiguration", // 8
        "Gehe zu",      // 9 - GoTo
        "Beenden",      // 10 - Exit
        "Speichern",    // 11 - Save
        "Home",         // 12 - Home
        "Parken",       // 13 - Park
        "Entparken",    // 14 - Unpark
        "Einstellung",  // 15 - Adjustment
        "Polar",        // 16 - Polar
        "Geschwindigkeit", // 17 - Speed
        "Grenzen",      // 18 - Limits
        "Niveau",       // 19 - Level
        "Ziel",         // 20 - Target
        "Aktuell",      // 21 - Current
        "Anzeigen",     // 22 - Show
        "Gespeichert",  // 23 - Stored
        "Gyroskop",     // 24 - Gyro
        "AutoPA",       // 25 - Auto Polar Align
        "AutoHome",     // 26 - Auto Home
        "GPS",          // 27 - GPS
        "AN",           // 28 - ON
        "AUS"           // 29 - OFF
    }
};

// Language names for the language selector
static const char* LANGUAGE_NAMES[LANG_COUNT] = {
    "Francais",     // French
    "English",      // English
    "Español",      // Spanish
    "Italiano",     // Italian
    "Deutsch"       // German
};

// Function to get translated string
inline const char* getTranslation(int index) {
    if (index < 0 || index >= 30) return "";
    return MENU_TITLES[currentLanguage][index];
}

// Helper macros for common translations
#define TR_RA getTranslation(0)
#define TR_DEC getTranslation(1)
#define TR_GO getTranslation(2)
#define TR_HA getTranslation(3)
#define TR_CTRL getTranslation(4)
#define TR_CAL getTranslation(5)
#define TR_FOC getTranslation(6)
#define TR_INFO getTranslation(7)
#define TR_CONFIG getTranslation(8)
#define TR_GOTO getTranslation(9)
#define TR_EXIT getTranslation(10)
#define TR_SAVE getTranslation(11)
#define TR_HOME getTranslation(12)
#define TR_PARK getTranslation(13)
#define TR_UNPARK getTranslation(14)
#define TR_ADJUSTMENT getTranslation(15)
#define TR_POLAR getTranslation(16)
#define TR_SPEED getTranslation(17)
#define TR_LIMITS getTranslation(18)
#define TR_LEVEL getTranslation(19)
#define TR_TARGET getTranslation(20)
#define TR_CURRENT getTranslation(21)
#define TR_SHOW getTranslation(22)
#define TR_STORED getTranslation(23)
#define TR_GYRO getTranslation(24)
#define TR_AUTOPA getTranslation(25)
#define TR_AUTOHOME getTranslation(26)
#define TR_GPS getTranslation(27)
#define TR_ON getTranslation(28)
#define TR_OFF getTranslation(29)