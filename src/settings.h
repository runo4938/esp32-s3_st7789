#ifndef SETTINGS_H
#define SETTINGS_H
#include <Arduino.h>
#include <EncButton.h>

int volume = 15;

Audio audio;

#define I2S_DOUT 16 // 27 // 18 // DIN connection
#define I2S_BCLK 17 // // Bit clock
#define I2S_LRC 18  //  // Left Right Clock
                    // end audio

// encoder
#define CLK 4 // 35 //
#define DT 3  //  //
#define SW 8  //  //

// Radio
uint8_t NEWStation = 0;
uint8_t OLDStation = 1;
int numbStations = 30;     // количество радиостанций
String displayStations[8]; // Массив для станций на дисплее
String StationList[30];    // Всего станций
String nameStations[30];   // Наименования станций
EncButton enc1(CLK, DT, SW);

// Weather
String apikey = F("30a9767f8d62e95546ea92a956457507"); // API key
String Latitude = F("44.0511");                        //
String Longitude = F("44.5408");                       //
String lang = F("&lang=ru");                           // this is your language

bool first = true; // Вывести дату и день недели
// bool http_acp = true;
//int y1_random;
//int y2_random;
bool rnd = true; // для случ числа
                 // Прошлое значение индикатора
int x1_lev;
int x2_lev;
int x1_prev = 162;
int x2_prev = 162;
int y_show = 122;

unsigned long lastTime = 0;
unsigned long lastTime_ssid = 0;
unsigned long timerDelay_ssid = 4000;

uint32_t vumetersDelay = 250;
// unsigned long previousMillis = 0;
// const long interval = 20; // интервал прокрутки мсек
/*----- Weather Json -----*/
typedef struct
{
    float lon;          // 37.71
    float lat;          // 51.84
    String description; // "небольшая облачность"
    String icon;        // "03n"
    float temp;         // 12.56
    float feels_like;   // 11.96
    int pressure;       // 1022     // приведённое к уровню моря давление.
    int humidity;       // 80
    int grnd_level;     // 1001   // давление на местности
    int visibility;     // 10000
    float speed;        // 2.1
    int deg;            // 354
    float gust;         // 2.15
    long dt;            // 1686079705
    String country;     // "RU"
    long sunrise;       // 1686013976
    long sunset;        // 1686073370
    long id;            // 540121
    String name;        // "Кшенский"
} weather_t;
weather_t weather;

// Ветер
const String Wind_N = F("Северный");
const String Wind_NE = F("Северо-Восточный");
const String Wind_E = F("Восточный");
const String Wind_SE = F("Юго-Восточный");
const String Wind_S = F("Южный");
const String Wind_SW = F("Юго-Западный");
const String Wind_W = F("Западный");
const String Wind_NW = F("Северо-Западный");
#endif
