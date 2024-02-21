// #include <Arduino.h>
#include <TFT_eSPI.h>
#include <Audio.h>
#include <EEPROM.h>
#include <settings.h>
#include <Update.h>
#include <ESPmDNS.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <GyverNTP.h>
#include <HTTPClient.h>
#include <ArduinoJSON.h>
#include <UnixTime.h>
#include <ImgWea60.h>
// #include <SPI.h>

#include "../lib/fonts.h"
#include "../lib/CourierCyr10.h"
#include "../lib/Bahamas18.h"
#include "../lib/CourierCyr12.h" //для меню станций
#include "../lib/Free_Fonts.h"
#define RU10 &FreeSansBold10pt8b
#define BAHAMAS &Bahamas18pt8b
#define DIG20 &DIG_Bold_20

#define LED_BRIGHTNESS 90 // яркость дисплея при старте
#define LED_BUILT 5       // управление яркостью дисплея

#define U_PART U_SPIFFS
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite txtSprite = TFT_eSprite(&tft);  // Create Sprite
TFT_eSprite WeatherSpr = TFT_eSprite(&tft); // Create Sprite
UnixTime stamp(3);                          // указать GMT (3 для Москвы)
bool online_w = true;
#define FIRMWARE_VERSION "1"
uint16_t ind;
String newSt;
const String space = " ";

WiFiManager wifiManager;

HTTPClient http;
WiFiClient client;

//-------OTA---
/* this info will be read by the python script */
#define FORMAT_SPIFFS_IF_FAILED true

int currentVersion = 0; // increment currentVersion in each release

String baseUrl = "https://raw.githubusercontent.com/runo4938/esp32-s3_st7789/main/";
String checkFile = "src/update.json";
// https://raw.githubusercontent.com/runo4938/esp32-s3_st7789/main/.pio/build/upesy_wrover/firmware.bin

int fwVersion = 0;
bool fwCheck = false;
String fwUrl = "", fwName = "";

TaskHandle_t myTaskHandle = NULL;

int ypos = 190; // position title
int xpos = 0;

String bitrate;

bool calendar = false;
bool stations;                 // Станция вверх или вниз (true or false)
bool showRadio = true;         // show radio or menu of station,
bool getClock = true;          // Получать время только при запуске
unsigned long currentMillis;   // To return from the menu after the time has expired
unsigned long intervalForMenu; // Для возврата из меню по истечении времении
bool f_startProgress = true;   // for initial values
void notFound(AsyncWebServerRequest *request);
String CurrentDate;
uint8_t CurrentWeek;
String days[8] = {"Воскресенье", "ПН", "ВТ", "СР", "ЧТ", "ПТ", "СБ", "ВС"};

// Scrolling
String MessageToScroll_1 = F(" 1-> For scrolling text 23 23 23 45 2-> ");
String MessageToScroll_2 = F("It's two string for scrolling");
int16_t width_txt;
int16_t width_txtW;

int x_scroll_L;
int x_scroll_R;

int x_scroll_LW;
int x_scroll_RW;

GyverNTP ntp(3);

AsyncWebServer server(80);

//--filesystem --------------

String filelist = "";
String listRadio; // радиостанции на странице
const char *PARAM = "file";
size_t content_len;
File file;
bool opened = false;
const char *host = "esp32";

//---------------------------------
TaskHandle_t Task1;
// weather
void Get_Weather_http(String &MSG_http);
void getWeather();
String WindDeg_Direction(int Wind_direction);
bool decode_json(Stream &jsonStr);

// void scrollMainWeather(bool directTo, int left_coner_x, int left_coner_y, int speed_scroll);
void scrollMain(bool directTo, int left_coner_x, int left_coner_y, int speed_scroll);
void scrollMainWeather(bool directTo, int left_coner_x, int left_coner_y, int speed_scroll);
void Task1code(void *pvParameters);
void printStation(uint8_t indexOfStation);
void printCodecAndBitrate();
void nextStation(bool stepStation);
void initSpiffs();
void readEEprom();
void initWiFi();
void wifiLevel();
void myEncoder();
void menuStation();
void showStation(int s); // пришли с номером станции по экрану
void stationDisplay(int st);
void clock_on_core0();
void drawlineClock();
void soundShow();
void lineondisp();
int startDayOfWeek(int y, int m, int d);
void drawCalendar();

static void rebootEspWithReason(String reason);
void performUpdate(Stream &updateSource, size_t updateSize);
void updateFromFS(fs::FS &fs);
bool downloadFirmware();
bool checkFirmware();
void newrelease();
String make_str(String str);
String utf8rus(String source);
String readFile(fs::FS &fs, const char *path);

void deleteFile(fs::FS &fs, const String &path);
void listDir(fs::FS &fs, const char *dirname, uint8_t levels);
void notFound(AsyncWebServerRequest *request);
void printProgress(size_t prg, size_t sz);
void handleDoUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
void handleDoUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final);
void listStaton();
String processor_update(const String &var);
String processor_playlst(const String &var);
void print_Img(int x, int y, String WeaIcon);
void newrelease();
void newVer();
//--- START ---
void setup()
{
  pinMode(LED_BUILT, OUTPUT);
  analogWrite(LED_BUILT, LED_BRIGHTNESS); // первоначальная яркость дисплея

  Serial.begin(115200);
  tft.begin();
  tft.setRotation(3);

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(40, 60);
  tft.println("Starting Radio...");

  readEEprom();
  initSpiffs();
  initWiFi();
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(40, 90);
  tft.println("Connected to SSID: ");
  tft.setCursor(40, 120);
  tft.println(WiFi.SSID());
  delay(1000);

  newVer();

  getWeather();
  MessageToScroll_2 = "";
  MessageToScroll_2.reserve(350);
  MessageToScroll_2 += F(" ");
  MessageToScroll_2 += F(" Погода: ");
  MessageToScroll_2 += weather.description;
  MessageToScroll_2 += F(" Давл:");
  MessageToScroll_2 += String(weather.grnd_level * 0.750062, 0); // давление на местности
  MessageToScroll_2 += F(" мм, Влажн:");
  MessageToScroll_2 += String(weather.humidity);
  MessageToScroll_2 += F("%, Ветер ");
  MessageToScroll_2 += String(WindDeg_Direction(weather.deg));
  MessageToScroll_2 += F(", ");
  MessageToScroll_2 += String(weather.speed, 1);
  MessageToScroll_2 += F("м/c ");
  stamp.getDateTime(weather.sunrise);
  MessageToScroll_2 += F(" всх:");
  MessageToScroll_2 += stamp.hour;
  MessageToScroll_2 += F(":");
  MessageToScroll_2 += stamp.minute;
  stamp.getDateTime(weather.sunset);
  MessageToScroll_2 += F(" зах:");
  MessageToScroll_2 += stamp.hour;
  MessageToScroll_2 += F(":");
  MessageToScroll_2 += stamp.minute;
  MessageToScroll_2 += F(" ");
  MessageToScroll_2 = utf8rus(MessageToScroll_2);
  width_txtW = tft.textWidth(MessageToScroll_2);
  // Serial.println(MessageToScroll_2);

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(15);

  tft.fillScreen(TFT_BLACK);
  // firs raw
  lineondisp();

  if (!MDNS.begin(host))
  {
    Serial.println("Error setting up MDNS responder!");
    while (1)
    {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

  listDir(SPIFFS, "/", 0);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", String(), false, processor_playlst); });

  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/upload.html", String(), false); });

  server.on("/newrelease", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(204);
               newrelease(); });

  // request->send(SPIFFS, "/playlist.html", String(), false, processor_playlst);

  server.on("/filesystem", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/fs.html", String(), false, processor_update); });

  server.on("/filelist", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/plain", filelist.c_str()); });

  server.on("/testpage", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/testpage.html", String(), false); });

  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    request->send(200, "text/plain", "Device will reboot in 2 seconds");
    delay(2000);
    ESP.restart(); });

  server.on(
      "/doUpdate", HTTP_POST,
      [](AsyncWebServerRequest *request) {},
      [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
      {
        handleDoUpdate(request, filename, index, data, len, final);
      });

  server.on(
      "/doUpload", HTTP_POST, [](AsyncWebServerRequest *request)
      { opened = false; },
      [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
      {
        handleDoUpload(request, filename, index, data, len, final);
      });

  server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/update?state=<inputMessage>
    if (request->hasParam(PARAM)) {
      inputMessage = request->getParam(PARAM)->value();
      inputParam = PARAM;

      deleteFile(SPIFFS, inputMessage);

      Serial.println("-inputMessage-");
      Serial.print("File=");
      Serial.println(inputMessage);
      Serial.println(" has been deleted");

    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    request->send(200, "text/plain", "OK"); });
  //---  filesystem ------
  //------------------------------------
  // Если переключили станцию вперед
  ////------------------------------------
  server.on("/off", HTTP_ANY, [](AsyncWebServerRequest *request)
            {
             request->send(204);//SPIFFS, "/index.html", String(), false, processor_playlst);
             vTaskSuspend(myTaskHandle);
             stations = false;
             nextStation(stations);
             vTaskResume(myTaskHandle); });
  //    Если переключили станцию назад
  //----------------------------------
  server.on("/on", HTTP_ANY, [](AsyncWebServerRequest *request)
            {
                request->send(204);
               // audio.stopSong();
               vTaskSuspend(myTaskHandle);
               stations = true;
               nextStation(stations);
               vTaskResume(myTaskHandle); });

  ntp.begin();
  server.onNotFound(notFound);
  server.begin();
  Update.onProgress(printProgress);

  // The first connection
  ind = StationList[NEWStation].indexOf(space);
  newSt = StationList[NEWStation].substring(ind + 1, StationList[NEWStation].length());
  const char *sl = newSt.c_str();
  audio.setVolume(volume);
  audio.connecttohost(sl); // переключаем станцию
  Serial.println(sl);
  OLDStation = NEWStation;  //
  printStation(NEWStation); // display the name of the station on the screen
  printCodecAndBitrate();

  // For CORE0
  xTaskCreatePinnedToCore(
      Task1code,     /* Функция задачи. */
      "Task1",       /* Ее имя. */
      4000,          /* Размер стека функции */
      NULL,          /* Параметры */
      1,             /* Приоритет */
      &myTaskHandle, /* Дескриптор задачи для отслеживания */
      0);            /* Указываем пин для данного ядра */
  delay(1);

  tft.setSwapBytes(true);

  txtSprite.createSprite(240, 26); // Ширина и высота спрайта
  txtSprite.setTextSize(1);
  txtSprite.setTextColor(0x9772, TFT_BLACK);
  txtSprite.fillSprite(TFT_BLACK);
  txtSprite.setFreeFont(&CourierCyr12pt8b);

  WeatherSpr.createSprite(240, 26); // Ширина и высота спрайта
  WeatherSpr.setTextSize(1);
  WeatherSpr.setTextColor(TFT_YELLOW, TFT_BLACK);
  WeatherSpr.fillSprite(TFT_BLACK);
  WeatherSpr.setFreeFont(RU10);

} // End Setup

//----------------------------------------
//*** Task for core 0 ***
//--------------------------------------
void Task1code(void *pvParameters)
{
  Serial.print("Task1 running on core ");
  Serial.println(xPortGetCoreID());
  for (;;)
  {
    audio.loop();
    vTaskDelay(2);
  }
}

//*******************************
// START loop
//*******************************
unsigned long timer_prev = 0;
int timer_interval = 3000;
bool allow = true;
int timer_interval_W = 4000;
bool allow_W = true;
bool direct, direct1;
int x_sprite = 65;
uint8_t ssid_show = 1;
void loop()
{
  if (enc1.tick())
    myEncoder();
  // для возврата из меню
  intervalForMenu = millis() - currentMillis;
  if (intervalForMenu > 10000 && showRadio == false && calendar == false) // если время истекло
  {
    // stations = false;
    tft.fillRect(0, 0, 320, ypos + 8, TFT_BLACK);
    NEWStation = OLDStation;
    printStation(NEWStation);
    wifiLevel();
    getClock = true; // получить время при переходе от меню станций
    showRadio = true;
    lineondisp();
    printCodecAndBitrate();
    first = true;
  }
  if (showRadio)
  {
    clock_on_core0();
    // drawlineClock();

    if (first && CurrentDate != "Not sync" && CurrentDate != "20.02.1611")
    { // выввод даты после меню станций
      tft.setTextSize(1);
      tft.setTextColor(0x9772);
      tft.setFreeFont(&CourierCyr12pt8b);
      tft.setCursor(272, 108);
      tft.print(utf8rus(days[CurrentWeek]));

      tft.setTextSize(1);
      tft.setFreeFont(RU10);
      tft.setTextColor(TFT_YELLOW);
      tft.setCursor(5, 225);
      tft.print(CurrentDate);

      wifiLevel();
      printCodecAndBitrate();
      first = false;
    }

    if (NEWStation != OLDStation)
    {
      // StationList[NEWStation].replace("_", space);
      ind = StationList[NEWStation].indexOf(space);
      newSt = StationList[NEWStation].substring(ind + 1, StationList[NEWStation].length());
      const char *sl = newSt.c_str();
      audio.pauseResume();
      printStation(NEWStation);
      delay(100);
      audio.setVolume(volume);
      audio.connecttohost(sl); // новая станция
      OLDStation = NEWStation;
    }

    // vuMeter
    uint16_t volByte = audio.getVUlevel();
    if (rnd) // для выборки всех значений длины индикатора уровня
    {
      x1_lev = highByte(volByte); // получить число и отработать
      x2_lev = lowByte(volByte);  //
      if (x1_lev % 2 > 0)         //
        x1_lev--;                 // for parity
      if (x2_lev % 2 > 0)
        x2_lev--; // на четность
      x1_lev = x1_lev + 162;
      x2_lev = x2_lev + 162;
      rnd = false; // Пройти до конца
    }
    if (vumetersDelay < millis())
    {
      if (f_startProgress)
      {
        x1_lev = 162;
        x2_lev = 162;
        x1_prev = 162;
        x2_prev = 162;
        rnd = false;
        f_startProgress = false;
      }
      soundShow();
      vumetersDelay = millis() + 25;
    } //-----end vumeter
      // Scrolling
    unsigned long timer_curr = millis();
    if (timer_curr - timer_prev >= timer_interval) // 2 sec
    {
      allow = !allow;
      timer_prev = timer_curr;
      direct = false; // random(0, 2);
      wifiLevel();
    }
    // Time for weather
    if (timer_curr - timer_prev >= timer_interval_W) // 4sec
    {
      allow_W = !allow_W;
      timer_prev = timer_curr;
      direct1 = false; // random(0, 3);
    }
    if (allow)
    {
      if (MessageToScroll_1 == "  " || MessageToScroll_1 == "   ")
      {
        scrollMainWeather(direct1, 71, 23, 3);
      }
      else
      {
        scrollMain(direct, 71, 23, 3);
      }
    }

    if ((millis() - lastTime_ssid) > timerDelay_ssid)
    {
      printCodecAndBitrate();
      switch (ssid_show)
      {
      case 1:
        tft.setFreeFont(&CourierCyr10pt8b);
        tft.setTextSize(1);
        tft.setTextColor(TFT_DARKGREEN, TFT_BLACK);
        tft.fillRect(135, 212, 183, 20, TFT_BLACK);
        tft.drawString(WiFi.SSID(), 135, 212);
        lastTime_ssid = millis();
        ssid_show = 2;
        break;
      case 2:
        tft.setFreeFont(&CourierCyr10pt8b);
        tft.setTextSize(1);
        tft.setTextColor(TFT_BROWN, TFT_BLACK);
        tft.fillRect(135, 212, 183, 20, TFT_BLACK);
        tft.drawString(WiFi.localIP().toString(), 135, 212);
        lastTime_ssid = millis();
        ssid_show = 3;
        break;
      case 3:
        tft.setTextSize(1);
        tft.setFreeFont(RU10);
        tft.setTextColor(TFT_CYAN);
        tft.setCursor(142, 226);
        tft.fillRect(135, 212, 183, 20, TFT_BLACK);
        tft.print(utf8rus(weather.name) + ", " + String(weather.temp, 1) + "`" + "C");
        lastTime_ssid = millis();
        ssid_show = 1;
        break;
      }
    }
  }
} // end LOOP

//*********************************
// Scrolling
//*********************************
void scrollMain(bool directTo, int left_coner_x, int left_coner_y, int speed_scroll) // to loop
{                                                                                    // в loop
  delay(speed_scroll);
  // Влево
  if (!directTo)
  {
    txtSprite.drawString(MessageToScroll_1, x_scroll_L, 2);
    txtSprite.pushSprite(left_coner_x, left_coner_y); // Верхний левый угол спрайта
    x_scroll_L--;
    x_scroll_R = x_scroll_L;                      // Влево
    if (abs(x_scroll_L) > width_txt + TFT_HEIGHT) //+ tft.width()
    {
      x_scroll_L = TFT_HEIGHT;
      x_scroll_R = -width_txt - TFT_HEIGHT;
    }
  }
  // Вправо
  if (directTo)
  {
    txtSprite.drawString(MessageToScroll_1, x_scroll_R, 2);
    txtSprite.pushSprite(left_coner_x, left_coner_y); // Верхний левый угол спрайта
    x_scroll_R++;
    x_scroll_L = x_scroll_R;     // Вправо
    if (x_scroll_R > TFT_HEIGHT) //+ tft.width());
    {
      x_scroll_L = TFT_HEIGHT;
      x_scroll_R = -width_txt - TFT_HEIGHT; // - tft.width();
    }
  }
}

void scrollMainWeather(bool directTo, int left_coner_x, int left_coner_y, int speed_scroll) // to loop
{                                                                                           // в loop
  delay(speed_scroll);
  // Влево
  if (!directTo)
  {
    // MessageToScroll_2 += CurrentDate + "  " + weather.name + weather.temp;
    WeatherSpr.drawString(MessageToScroll_2, x_scroll_LW, 2);
    WeatherSpr.pushSprite(left_coner_x, left_coner_y); // Верхний левый угол спрайта
    x_scroll_LW--;
    x_scroll_RW = x_scroll_LW;                            // Влево
    if (abs(x_scroll_LW) > width_txtW + TFT_HEIGHT + 300) //+ tft.width()
    {
      x_scroll_LW = TFT_HEIGHT + 300;
      x_scroll_RW = -width_txtW - TFT_HEIGHT - 300;
    }
  }
  // Вправо
  if (directTo)
  {
    WeatherSpr.drawString(MessageToScroll_2, x_scroll_RW, 2);
    WeatherSpr.pushSprite(left_coner_x, left_coner_y); // Верхний левый угол спрайта
    x_scroll_RW++;
    x_scroll_LW = x_scroll_RW;    // Вправо
    if (x_scroll_RW > TFT_HEIGHT) //+ tft.width());
    {
      x_scroll_LW = TFT_HEIGHT;
      x_scroll_RW = -width_txtW - TFT_HEIGHT - 300; // - tft.width();
    }
  }
}

// Показать VUmeter
void soundShow()
{
  int y_show = 130;

  if (x1_prev > x1_lev)
  {
    tft.fillRect(x1_prev, y_show, 4, 23, TFT_BLACK);
    x1_prev = x1_prev - 7;
  }
  if (x1_prev < x1_lev)
  {
    tft.fillRect(x1_prev, y_show, 4, 23, TFT_GREEN);
    x1_prev = x1_prev + 7;
  }

  if (x2_prev > x2_lev)
  {
    tft.fillRect(x2_prev, y_show + 28, 4, 23, TFT_BLACK);
    x2_prev = x2_prev - 7;
  }
  if (x2_prev < x2_lev)
  {
    tft.fillRect(x2_prev, y_show + 28, 4, 23, TFT_GREEN);
    x2_prev = x2_prev + 7;
  }
  rnd = true;
}
//---------------------
//  Clock
//--------------------
byte omm = 99, oss = 99;
uint32_t targetTime_clock = 0; // update clock every second
byte xcolon = 0, xsecs_clock = 0;
uint8_t hh, mm, ss; // for new clock
void clock_on_core0()
{
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setFreeFont(DIG20); // DIG20
  tft.setTextSize(2);     // 3

  ntp.tick();
  delay(2);
  hh = ntp.hour();
  mm = ntp.minute();
  ss = ntp.second();
  CurrentDate = ntp.dateString();
  CurrentWeek = ntp.dayWeek();

  if (targetTime_clock < millis())
  {
    // Set next update for 1 second later
    targetTime_clock = millis() + 1000;
    getClock = true;
    // Adjust the time values by adding 1 second
    ss++; // Advance second
    if (ss == 60)
    {           // Check for roll-over
      ss = 0;   // Reset seconds to zero
      omm = mm; // Save last minute time for display update
      mm++;     // Advance minute
      if (mm > 59)
      { // Check for roll-over
        mm = 0;
        hh++; // Advance hour
        if (hh > 23)
        {         // Check for 24hr roll-over (could roll-over on 13)
          hh = 0; // 0 for 24 hour clock, set to 1 for 12 hour clock
        }
      }
    }
    // Update digital time
    int xpos_clock = 105;
    int ypos_clock = 65; // Top left corner ot clock text, about half way down
    int ysecs_clock = ypos_clock;
    if (omm != mm || getClock == true)
    { // Redraw hours and minutes time every minute
      omm = mm;
      // Draw hours and minutes
      if (hh < 10)
        xpos_clock += tft.drawNumber(0, xpos_clock, ypos_clock); // Add hours leading zero for 24 hr clock
      xpos_clock += tft.drawNumber(hh, xpos_clock, ypos_clock);  // Draw hours
      xcolon = xpos_clock;                                       // Save colon coord for later to flash on/off later
      xpos_clock += tft.drawChar(':', xpos_clock, ypos_clock + 40);
      if (mm < 10)
        xpos_clock += tft.drawNumber(0, xpos_clock, ypos_clock); // Add minutes leading zero
      xpos_clock += tft.drawNumber(mm, xpos_clock, ypos_clock);  // Draw minutes
      xsecs_clock = xpos_clock;                                  // Sae seconds 'x' position for later display updates
    }
    if (oss != ss || getClock == true)
    { // Redraw seconds time every second
      oss = ss;
      xpos_clock = xsecs_clock;
      tft.setTextSize(2);
      if (ss % 2)
      {                                             // Flash the colons on/off
        tft.setTextColor(0x39C4, TFT_BLACK);        // Set colour to grey to dim colon
        tft.drawChar(':', xcolon, ypos_clock + 40); // Hour:minute colon
                                                    //  xpos += tft.drawChar(':', xsecs, ysecs); // Seconds colon
        tft.setTextColor(0x9772, TFT_BLACK);        // Set colour back to yellow
      }
      else
      {
        tft.drawChar(':', xcolon, ypos_clock + 40); // Hour:minute colon
                                                    // xpos += tft.drawChar(':', xsecs, ysecs); // Seconds colon
      }
      // Draw seconds
      tft.setTextSize(1);
      if (ss < 10)
        xpos_clock += tft.drawNumber(0, xpos_clock + 16, ysecs_clock - 3); // Add leading zero
      tft.drawNumber(ss, xpos_clock + 14, ysecs_clock - 3);                // Draw seconds
    }
  }
  tft.drawRect(260, 50, 60, 37, 0x9772);
  tft.drawRect(260, 87, 60, 33, 0x9772);
  getClock = false;
}

// //----------------------------
// // File position
// //---------------------------
// // uint8_t audioVolume1;
// int x_FP = 65, y_FP = ypos + 15; // position in line
// // Уровень громкости
// void filePosition()
// {
//   volume = audio.getVolume() * 12;
//   tft.fillRect(x_FP, y_FP - 2, 250, 8, TFT_BLACK);
//   tft.drawRect(x_FP, y_FP, 252, 6, 0x9772);
//   tft.fillRect(x_FP, y_FP, volume, 6, 0x9772);
//   // Serial.println("filePositio = " + String(audioVolume1));
// }

//-------------------
// Encoder
//-------------------
void myEncoder()
{
  // enc1.tick();
  if (enc1.right() && calendar == false)
  {
    if (showRadio)
    {
      stations = false;
      nextStation(stations);
      printStation(NEWStation);
    }
    if (!showRadio)
    {
      stations = false;
      nextStation(stations);
      // menuStation();
      stationDisplay(NEWStation);
      currentMillis = millis(); // Пока ходим по меню
    }
    // если меню
  }
  if (enc1.left() && calendar == false)
  {
    if (showRadio)
    {
      stations = true;
      nextStation(stations);
      printStation(NEWStation);
    }
    if (!showRadio)
    {
      stations = true;
      nextStation(stations);
      // menuStation();
      stationDisplay(NEWStation);
      currentMillis = millis(); // Пока ходим по меню
    }
  }
  if (enc1.click())
  { // Меню станций
    showRadio = !showRadio;
    calendar = false;
    f_startProgress = true; // for starting
    if (!showRadio)
    {
      currentMillis = millis(); // начало отсчета времени простоя
      tft.fillRect(3, 3, 315, 190, TFT_BLACK);
      stationDisplay(NEWStation);
    }
    if (showRadio)
    {
      first = true;
      tft.fillRect(0, 0, 320, ypos + 8, TFT_BLACK);
      printStation(NEWStation);
      getClock = true; // получить время при переходе от меню станций
      lineondisp();
      printCodecAndBitrate();
    }
  }

  if (enc1.rightH())
  {
    calendar = true;
    showRadio = false;
    drawCalendar();
  }
  if (enc1.step(2))
  {
    WiFi.disconnect(false, true);
    wifiManager.resetSettings();
    Serial.println("Reseting creditals password.");
    delay(1000);
    ESP.restart();
  }
}

//----------------------
//   Menu Stations
// Complect menu stations
//------------------------
void menuStation()
{
  int i = 0;
  int ind = 0;
  while (i <= numbStations)
  { // list stations
    delay(1);
    ind = StationList[i].indexOf(space);
    nameStations[i] = make_str(utf8rus(StationList[i].substring(0, ind))); // Получили наименования станций
    i++;
  }
}
//----------------------------------
// ******* Menu stations ***********
//----------------------------------
void stationDisplay(int st)
{
  uint8_t i;
  i = 0;
  while (i < 6)
  {
    displayStations[i] = "";
    i++;
  }
  tft.setTextSize(1);
  tft.setFreeFont(&CourierCyr12pt8b);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  // счетчик для меню
  int stanonMenu = 3; // Положение текущей станции в меню
  int k;              //
  int p;              // счечик по листу станций
  k = st - 3;

  if (k < 0 && k != -3)
  {
    p = numbStations + k + 1;
    i = 0;
    while (p <= numbStations)
    {
      displayStations[i] = nameStations[p];
      i++;
      p++;
    }
    p = 0;
    while (i <= 6)
    {
      displayStations[i] = nameStations[p];
      i++;
      p++;
    }
  }

  if (k == -3)
  {
    i = 0;
    p = numbStations - 2;
    while (i <= 2)
    {
      displayStations[i] = nameStations[p];
      i++;
      p++;
    }
    p = 0;
    while (i <= 6)
    {
      displayStations[i] = nameStations[p];
      i++;
      p++;
    }
  }
  p = k;
  if (k >= 0)
  {
    i = 0;
    while (i <= 6)
    {
      displayStations[i] = nameStations[p];
      p++;
      i++;
      if (p == numbStations + 1)
        p = 0;
    }
  }
  // выводим на дисплей
  i = 0;
  k = 10;
  while (i <= 6)
  {
    tft.fillRect(65, k, 246, 25, TFT_BLACK);
    tft.drawString(utf8rus(displayStations[i]), 65, k);
    i++;
    k = k + 25;
  }
  tft.fillRect(65, (stanonMenu * 25) + 10, 246, 25, TFT_YELLOW);
  tft.setTextColor(TFT_BLACK, TFT_YELLOW);
  tft.drawString(utf8rus(displayStations[stanonMenu]), 65, (stanonMenu * 25) + 10);
}

// Разделитель минут и секунд
void drawlineClock()
{ //             x    y    x    y
  tft.drawRect(260, 50, 60, 37, 0x9772);
  tft.drawRect(260, 87, 60, 33, 0x9772);
}

// Дополнить строку пробелами
String make_str(String str)
{
  for (int i = 0; i < (18 - str.length()); i++)
    str += char(32);
  return str;
}
//----------------------------
// Вывод наименования станции
//----------------------------
void printStation(uint8_t indexOfStation)
{
  // Serial.println("indexofstation" + String(indexOfStation));
  uint8_t localIndex;
  String StName;
  // String space = " ";
  localIndex = StationList[indexOfStation].indexOf(space);
  StName = StationList[indexOfStation].substring(0, localIndex + 1);
  tft.setTextColor(0x9773, TFT_BLACK);
  // tft.setCursor(132, 15);
  tft.setTextSize(1);
  tft.setFreeFont(RU10);
  tft.fillRect(75, 5, 240, 20, TFT_BLACK);
  tft.drawString(utf8rus(StName), 75, 5);
} // end PrintStation

//----------------------------
// CodecName Bitrate
//----------------------------
void printCodecAndBitrate()
{
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setFreeFont(&CourierCyr10pt8b);
  tft.setTextSize(1);
  tft.fillRect(10, 125, 62, 28, 0x9772);
  tft.fillRect(12, 127, 58, 23, TFT_BLACK);
  tft.drawString(String(audio.getCodecname()).substring(0, 3), 18, 130);
  tft.fillRect(10, 157, 62, 28, 0x9772);
  tft.fillRect(12, 159, 58, 24, TFT_BLACK);
  int bit = bitrate.toInt();
  if (bit < 128000)
  {
    tft.drawString(String(bit).substring(0, 2) + "k ", 15, 162);
  }
  else
  {
    tft.drawString(bitrate.substring(0, 3) + "k", 15, 162);
  }
  EEPROM.write(2, NEWStation);
  EEPROM.commit();
}

// Next station
void nextStation(bool stepStation)
{
  if (stepStation)
  {
    if (NEWStation != numbStations)
    {
      NEWStation++;
    }
    else
    {
      NEWStation = 0;
    }
  }
  else
  {
    if (NEWStation != 0)
    {
      NEWStation--;
    }
    else
    {
      NEWStation = numbStations;
    }
  }
  EEPROM.write(2, NEWStation);
  EEPROM.commit();
}

// SPIFFS
void initSpiffs()
{
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  File myFile;
  File root = SPIFFS.open("/");

  File file = root.openNextFile();
  while (file)
  {
    Serial.print("FILE: ");
    Serial.println(file.name());
    file = root.openNextFile();
  }
  myFile = SPIFFS.open("/playlist.txt", FILE_READ);
  if (!myFile)
  {
    Serial.println("------File does not exist!------");
  }
  int i = 0;
  while (myFile.available())
  {
    StationList[i] = myFile.readStringUntil('\n');
    Serial.println(StationList[i]);
    i++;
  }
  myFile.close();
  numbStations = i - 1; // start numb station
  menuStation();
  listStaton();
}

// EEPROM
void readEEprom()
{
  if (!EEPROM.begin(5))
  {
    Serial.println("failed to initialise EEPROM");
    delay(1000);
  }
  Serial.println(" bytes read from Flash . Values are:");

  if (EEPROM.read(2) > 200)
  {
    NEWStation = 0;
  }
  else
  {
    NEWStation = EEPROM.read(2);
  }
}
//****************************
//    WiFi
//****************************
void initWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  Serial.println("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED && millis() < 15 * 1000)
  {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("WiFi not connected, starting WiFiManager");
    tft.println("WiFi not connected..");
    tft.println("Starting WiFiManager");
    tft.println("SSID: ESP32-Clock");
    tft.println("IP: 192.168.4.1");
    wifiManager.autoConnect("ESP32-Clock");
    delay(2000);
  }

  Serial.println("\tConnecting Wifi...");
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\tWiFi connected");
    Serial.print("\tIP address: = ");
    Serial.println(WiFi.localIP());
    Serial.print("\tSSID = ");
    Serial.println(WiFi.SSID());
    Serial.println("\tRSSI :" + String(WiFi.RSSI()));

    String bootlog;
    bootlog += F("\n\t");
    bootlog += F("CHIPmodel  = ");
    bootlog += ESP.getChipModel();
    bootlog += F("\n\t");
    bootlog += F("Revision ");
    bootlog += ESP.getChipRevision();
    bootlog += F("\n\t");
    bootlog += F("Cores = ");
    bootlog += ESP.getChipCores();
    bootlog += F("\n\t");
    bootlog += F("PSRAM = ");
    bootlog += ESP.getPsramSize();
    bootlog += F("\n\t");
    bootlog += F("FlashChipSize = ");
    bootlog += ESP.getFlashChipSize();
    bootlog += F("\n\t");
    bootlog += F("HeapSize = ");
    bootlog += ESP.getHeapSize();
    bootlog += F("\n\t");
    bootlog += F("FreeHeap = ");
    bootlog += ESP.getFreeHeap();
    bootlog += F("\n\t");
    bootlog += F("SkethSize = ");
    bootlog += ESP.getSketchSize();
    bootlog += F("\n\t");
    bootlog += F("FreeSkethSpace = ");
    bootlog += ESP.getFreeSketchSpace();
    bootlog += F("\n\t");
    bootlog += F("FreePsram = ");
    bootlog += ESP.getFreePsram();
    Serial.print('\t');
    Serial.println(bootlog);
  }
}
// WiFi level
void wifiLevel()
{
  uint16_t x_wifi = 85, y_wifi = ypos - 7, y_lev_wifi = 3;
  if (WiFi.RSSI() >= -60)
  {
    tft.fillRect(x_wifi + 8, y_wifi, 3, y_lev_wifi + 3, TFT_GREENYELLOW);
    tft.fillRect(x_wifi + 13, y_wifi - 2, 3, y_lev_wifi + 5, TFT_GREENYELLOW);
    tft.fillRect(x_wifi + 18, y_wifi - 2 * 2, 3, y_lev_wifi + 7, TFT_GREENYELLOW);
    tft.fillRect(x_wifi + 23, y_wifi - 2 * 3, 3, y_lev_wifi + 9, TFT_GREENYELLOW);
    tft.fillRect(x_wifi + 28, y_wifi - 2 * 4, 3, y_lev_wifi + 11, TFT_GREENYELLOW);
    tft.fillRect(x_wifi + 33, y_wifi - 2 * 5, 3, y_lev_wifi + 13, TFT_GREENYELLOW);
    tft.fillRect(x_wifi + 38, y_wifi - 2 * 6, 3, y_lev_wifi + 15, TFT_GREENYELLOW);
  }
  if (WiFi.RSSI() < -60 && WiFi.RSSI() >= -70)
  {
    tft.fillRect(x_wifi + 8, y_wifi, 3, y_lev_wifi + 3, TFT_GREENYELLOW);
    tft.fillRect(x_wifi + 13, y_wifi - 2, 3, y_lev_wifi + 5, TFT_GREENYELLOW);
    tft.fillRect(x_wifi + 18, y_wifi - 2 * 2, 3, y_lev_wifi + 7, TFT_GREENYELLOW);
    tft.fillRect(x_wifi + 23, y_wifi - 2 * 3, 3, y_lev_wifi + 9, TFT_GREENYELLOW);
    tft.fillRect(x_wifi + 28, y_wifi - 2 * 4, 3, y_lev_wifi + 11, TFT_GREENYELLOW);
    tft.fillRect(x_wifi + 33, y_wifi - 2 * 5, 3, y_lev_wifi + 13, TFT_GREENYELLOW);
    tft.fillRect(x_wifi + 38, y_wifi - 2 * 6, 3, y_lev_wifi + 15, 0x39C4);
  }
  if (WiFi.RSSI() < -70 && WiFi.RSSI() > -80)
  {
    tft.fillRect(x_wifi + 8, y_wifi, 3, y_lev_wifi + 3, TFT_GREENYELLOW);
    tft.fillRect(x_wifi + 13, y_wifi - 2, 3, y_lev_wifi + 5, TFT_GREENYELLOW);
    tft.fillRect(x_wifi + 18, y_wifi - 2 * 2, 3, y_lev_wifi + 7, TFT_GREENYELLOW);
    tft.fillRect(x_wifi + 23, y_wifi - 2 * 3, 3, y_lev_wifi + 9, TFT_GREENYELLOW);
    tft.fillRect(x_wifi + 28, y_wifi - 2 * 4, 3, y_lev_wifi + 11, TFT_GREENYELLOW);
    tft.fillRect(x_wifi + 33, y_wifi - 2 * 5, 3, y_lev_wifi + 13, 0x39C4);
    tft.fillRect(x_wifi + 38, y_wifi - 2 * 6, 3, y_lev_wifi + 15, 0x39C4);
  }

  if (WiFi.RSSI() < -80 && WiFi.RSSI() > -90)
  {
    tft.fillRect(x_wifi + 8, y_wifi, 3, y_lev_wifi + 3, TFT_GREENYELLOW);
    tft.fillRect(x_wifi + 13, y_wifi - 2, 3, y_lev_wifi + 5, TFT_GREENYELLOW);
    tft.fillRect(x_wifi + 18, y_wifi - 2 * 2, 3, y_lev_wifi + 7, TFT_GREENYELLOW);
    tft.fillRect(x_wifi + 23, y_wifi - 2 * 3, 3, y_lev_wifi + 9, TFT_GREENYELLOW);
    tft.fillRect(x_wifi + 28, y_wifi - 2 * 4, 3, y_lev_wifi + 11, 0x39C4);
    tft.fillRect(x_wifi + 33, y_wifi - 2 * 5, 3, y_lev_wifi + 13, 0x39C4);
    tft.fillRect(x_wifi + 38, y_wifi - 2 * 6, 3, y_lev_wifi + 15, 0x39C4);
  }
  if (WiFi.RSSI() < -90)
  {
    tft.fillRect(x_wifi + 8, y_wifi, 3, y_lev_wifi + 3, TFT_GREENYELLOW);
    tft.fillRect(x_wifi + 13, y_wifi - 2, 3, y_lev_wifi + 5, 0x39C4);
    tft.fillRect(x_wifi + 18, y_wifi - 2 * 2, 3, y_lev_wifi + 7, 0x39C4);
    tft.fillRect(x_wifi + 23, y_wifi - 2 * 3, 3, y_lev_wifi + 9, 0x39C4);
    tft.fillRect(x_wifi + 28, y_wifi - 2 * 4, 3, y_lev_wifi + 11, 0x39C4);
    tft.fillRect(x_wifi + 33, y_wifi - 2 * 5, 3, y_lev_wifi + 13, 0x39C4);
    tft.fillRect(x_wifi + 38, y_wifi - 2 * 6, 3, y_lev_wifi + 15, 0x39C4);
  }
}

String readFile(fs::FS &fs, const char *path)
{
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path);
  if (!file || file.isDirectory())
  {
    Serial.println("- failed to open file for reading");
    return String();
  }
  String fileContent;
  while (file.available())
  {
    fileContent = file.readStringUntil('\n');
    break;
  }
  return fileContent;
}

String utf8rus(String source)
{
  int i, k;
  String target;
  unsigned char n;
  char m[2] = {'0', '\0'};
  k = source.length();
  i = 0;
  while (i < k)
  {
    delay(1);
    n = source[i];
    i++;
    if (n >= 127)
    {
      switch (n)
      {
      case 208:
      {
        n = source[i];
        i++;
        if (n == 129)
        {
          n = 192; // перекодируем букву Ё
          break;
        }
        break;
      }
      case 209:
      {
        n = source[i];
        i++;
        if (n == 145)
        {
          n = 193; // перекодируем букву ё
          break;
        }
        break;
      }
      }
    }
    m[0] = n;
    target = target + String(m);
  }
  return target;
}

void audio_showstreamtitle(const char *info)
{
  Serial.println(info);
  // txtSprite.fillSprite(TFT_BLACK);
  MessageToScroll_1 = F(" ");
  MessageToScroll_1 += info;
  MessageToScroll_1 += F(" ");
  MessageToScroll_1 = utf8rus(MessageToScroll_1);
  // txtSprite.setTextSize(1);
  width_txt = tft.textWidth(MessageToScroll_1);
  x_scroll_R = -width_txt;
  x_scroll_L = width_txt;
}

//--weather ---
void Get_Weather_http(String &MSG_http)
{
  Serial.println(MSG_http);
  http.begin(client, MSG_http);
}

bool decode_json(Stream &jsonStr)
{
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, jsonStr);
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return false;
  }
  else
  {
    Serial.println("deserializeJson() без ошибок.");
    JsonObject doc_OPW = doc.as<JsonObject>();
    weather.lon = doc_OPW["coord"]["lon"].as<float>();
    weather.lat = doc_OPW["coord"]["lat"].as<float>();
    weather.description = doc_OPW["weather"][0]["description"].as<const char *>();
    weather.icon = doc_OPW["weather"][0]["icon"].as<const char *>();
    weather.temp = doc_OPW["main"]["temp"].as<float>();
    weather.feels_like = doc_OPW["main"]["feels_like"].as<float>();
    weather.pressure = doc_OPW["main"]["pressure"].as<int>();
    weather.humidity = doc_OPW["main"]["humidity"].as<int>();
    weather.grnd_level = doc_OPW["main"]["grnd_level"].as<int>();
    weather.visibility = doc_OPW["visibility"].as<int>();
    weather.speed = doc_OPW["wind"]["speed"].as<float>();
    weather.deg = doc_OPW["wind"]["deg"].as<int>();
    weather.gust = doc_OPW["wind"]["gust"].as<float>();
    weather.dt = doc_OPW["dt"].as<long>();
    weather.country = doc_OPW["sys"]["country"].as<const char *>();
    weather.sunrise = doc_OPW["sys"]["sunrise"].as<long>();
    weather.sunset = doc_OPW["sys"]["sunset"].as<long>();
    weather.id = doc_OPW["id"].as<long>();
    weather.name = doc_OPW["name"].as<const char *>();
    return true;
  } // end else
} // end decode_json

void getWeather()
{
  if (WiFi.status() == WL_CONNECTED)
  { // проверяем соединение WiFi
    String host_uri;
    host_uri.reserve(150);
    host_uri += F("http://api.openweathermap.org/data/2.5/");
    host_uri += F("weather?lat=");
    host_uri += Latitude;
    host_uri += F("&lon=");
    host_uri += Longitude;
    host_uri += lang;
    host_uri += F("&appid=");
    host_uri += apikey;
    host_uri += F("&mode=json&units=metric&cnt=1");
    client.stop();
    Get_Weather_http(host_uri);
    int httpCode = http.GET();
    Serial.println(httpCode);
    if (httpCode > 0)
    {
      Stream &response = http.getStream(); // ответ
      decode_json(response);               // парсинг данных из JsonObject                    // парсинг данных из JsonObject
#ifdef Serial_Print                        //  отладка
      Serial.println(" - - weather - - ");
      Serial.print("weather.lon ");
      Serial.println(weather.lon);
      Serial.print("weather.lat ");
      Serial.println(weather.lat);
      Serial.print("weather.description ");
      Serial.println(weather.description);
      Serial.print("weather.icon ");
      Serial.println(weather.icon);
      Serial.print("weather.temp ");
      Serial.println(weather.temp);
      Serial.print("weather.feels_like ");
      Serial.println(weather.feels_like);
      Serial.print("weather.pressure ");
      Serial.println(weather.pressure);
      Serial.print("weather.humidity ");
      Serial.println(weather.humidity);
      Serial.print("weather.grnd_level ");
      Serial.println(weather.grnd_level);
      Serial.print("weather.visibility ");
      Serial.println(weather.visibility);
      Serial.print("weather.speed ");
      Serial.println(weather.speed);
      Serial.print("weather.deg ");
      Serial.println(weather.deg);
      Serial.print("weather.gust ");
      Serial.println(weather.gust);
      Serial.print("weather.dt ");
      Serial.println(weather.dt);
      Serial.print("weather.country ");
      Serial.println(weather.country);
      Serial.print("weather.sunrise ");
      Serial.println(weather.sunrise);
      Serial.print("weather.sunset ");
      Serial.println(weather.sunset);
      Serial.print("weather.id ");
      Serial.println(weather.id);
      Serial.print("weather.name ");
      Serial.println(weather.name);
#endif
      client.stop();
      http.end();
    }
    else
    {
      Serial.println("Connection failed");
      client.stop();
      http.end();
    }
  }
} // end getWeather

String WindDeg_Direction(int Wind_direction)
{
  if (Wind_direction >= 338 || Wind_direction < 22)
    return Wind_N; //"Северный";
  if (Wind_direction >= 22 && Wind_direction < 68)
    return Wind_NE; //"Северо-Восточный";
  if (Wind_direction >= 68 && Wind_direction < 112)
    return Wind_E; //"Восточный";
  if (Wind_direction >= 112 && Wind_direction < 158)
    return Wind_SE; //"Юго-Восточный";
  if (Wind_direction >= 158 && Wind_direction < 202)
    return Wind_S; //"Южный";
  if (Wind_direction >= 202 && Wind_direction < 248)
    return Wind_SW; //"Юго-Западный";
  if (Wind_direction >= 248 && Wind_direction < 292)
    return Wind_W; //"Западный";
  if (Wind_direction >= 292 && Wind_direction < 338)
    return Wind_NW; //"Северо-Западный";
  return " ?";
} // end WindDeg_Direction

// optional
/* void audio_info(const char *info)
{
  Serial.print("info        ");
  Serial.println(info);
}
void audio_id3data(const char *info)
{ // id3 metadata
  Serial.print("id3data     ");
  Serial.println(info);
}
void audio_eof_audio(const char *info)
{ // end of file
  Serial.print("eof_audio     ");
  Serial.println(info);
}

*/
void audio_bitrate(const char *info)
{
  bitrate = info;
}
/*
void audio_commercial(const char *info)
{ // duration in sec
  Serial.print("commercial  ");
  Serial.println(info);
}
void audio_icyurl(const char *info)
{ // homepage
  Serial.print("icyurl      ");
  Serial.println(info);

}
void audio_lasthost(const char *info)
{ // stream URL played
  Serial.print("lasthost    ");
  Serial.println(info);
}
void audio_eof_speech(const char *info)
{
  Serial.print("eof_speech  ");
  Serial.println(info);
}
*/

//----------filesystem
String processor_playlst(const String &var)
{
  // Serial.println(var);
  // Serial.println(listRadio);
  if (var == "nameST")
  {
    return listRadio;
  }
  if(var == "version"){
    return version;
  }
  return String();
}

String processor_update(const String &var)
{
  Serial.println(var);
  if (var == "list")
  {
    return filelist;
  }
  return String();
}

void listStaton()
{
  String partlistStation;
  uint8_t i = 0;
  while (i <= numbStations)
  {
    int ind_to_scace = StationList[i].indexOf(space);
    String nameStat = StationList[i].substring(0, ind_to_scace);

    String newStat = StationList[i].substring(ind_to_scace + 1, StationList[i].length());

    partlistStation += String("<tr><td>") + String(i) + String("</td>") + String("<td>") + nameStat + String("</td>") + String("<td>") + newStat + String("</td></tr>");
    listRadio = String("<table class=\"table table-success table-striped\"> <thead><tr><th>№</th><th>Station name</th><th>Station url</th></tr></thead><tbody>") + partlistStation + String("</tbody></table>");
    i++;
  }
}
// class=\"table table-success table-striped\"
void handleDoUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if (!index)
  {
    // Serial.println("Update");
    content_len = request->contentLength();
    // if filename includes spiffs, update the spiffs partition
    int cmd = (filename.indexOf("spiffs") > -1) ? U_PART : U_FLASH;
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd))
    {
      Update.printError(Serial);
    }
  }

  if (Update.write(data, len) != len)
  {
    Update.printError(Serial);
    Serial.printf("Progress: %d%%\n", (Update.progress() * 100) / Update.size());
  }
  if (final)
  {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Ok");
    response->addHeader("Refresh", "30");
    response->addHeader("Location", "/");
    request->send(response);
    if (!Update.end(true))
    {
      Update.printError(Serial);
    }
    else
    {
      Serial.println("Update complete");
      Serial.flush();
      ESP.restart();
    }
  }
}

void handleDoUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if (!index)
  {
    content_len = request->contentLength();
    Serial.printf("UploadStart: %s\n", filename.c_str());
  }
  if (opened == false)
  {
    opened = true;
    file = SPIFFS.open(String("/") + filename, FILE_WRITE);
    if (!file)
    {
      Serial.println("- failed to open file for writing");
      return;
    }
  }
  if (file.write(data, len) != len)
  {
    Serial.println("- failed to write");
    return;
  }
  if (final)
  {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Ok");
    response->addHeader("Refresh", "20");
    response->addHeader("Location", "/filesystem");
    request->send(response);
    file.close();
    opened = false;
    Serial.println("---------------");
    Serial.println("Upload complete");
  }
}

void printProgress(size_t prg, size_t sz)
{
  Serial.printf("Progress: %d%%\n", (prg * 100) / content_len);
}

//------------------
void notFound(AsyncWebServerRequest *request)
{
  if (request->url().startsWith("/"))
  {
    request->send(SPIFFS, request->url(), String(), true);
  }
  else
  {
    request->send(404);
  }
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels)
{
  // filelist = "";
  int i = 0;
  String partlist;
  // Serial.printf("Listing directory: %s\r\n", dirname);
  File root = fs.open(dirname);
  if (!root)
  {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      if (levels)
      {
        listDir(fs, file.name(), levels - 1);
      }
    }
    else
    {
      i++;
      String st_after_symb = String(file.name()).substring(String(file.name()).indexOf("/") + 1);

      partlist += String("<tr><td>") + String(i) + String("</td><td>") + String("<a href='") + String(file.name()) + String("'>") + st_after_symb + String("</td><td>") + String(file.size() / 1024) + String("</td><td>") + String("<input type='button' class='btndel' onclick=\"deletef('") + String(file.name()) + String("')\" value='X'>") + String("</td></tr>");
      filelist = String("<table><tbody><tr><th>#</th><th>File name</th><th>Size(KB)</th><th></th></tr>") + partlist + String(" </tbody></table>");
    }
    file = root.openNextFile();
  }
  filelist = String("<table><tbody><tr><th>#</th><th>File name</th><th>Size(KB)</th><th></th></tr>") + partlist + String(" </tbody></table>");
}

void deleteFile(fs::FS &fs, const String &path)
{
  Serial.printf("Deleting file: %s\r\n", path);
  if (fs.remove(path))
  {
    Serial.println("- file deleted");
  }
  else
  {
    Serial.println("- delete failed");
  }
}
//----------filesystem
void print_Img(int x, int y, String WeaIcon)
{
  int w, h;
  w = 60;
  h = 60;
  if (WeaIcon == "01d")
    tft.pushImage(x, y, w, h, img_01d);
  else if (WeaIcon == "01n")
    tft.pushImage(x, y, w, h, img_01n);
  else if (WeaIcon == "02d")
    tft.pushImage(x, y, w, h, img_02d);
  else if (WeaIcon == "02n")
    tft.pushImage(x, y, w, h, img_02n);
  else if (WeaIcon == "03d" || WeaIcon == "03n")
    tft.pushImage(x, y, w, h, img_03dn);
  else if (WeaIcon == "04d" || WeaIcon == "04n")
    tft.pushImage(x, y, w, h, img_04dn);
  else if (WeaIcon == "09d" || WeaIcon == "09n")
    tft.pushImage(x, y, w, h, img_09dn);
  else if (WeaIcon == "10d" || WeaIcon == "10n")
    tft.pushImage(x, y, w, h, img_10dn);
  else if (WeaIcon == "11d" || WeaIcon == "11n")
    tft.pushImage(x, y, w, h, img_11dn);
  else if (WeaIcon == "13d" || WeaIcon == "13n")
    tft.pushImage(x, y, w, h, img_13dn);
  else if (WeaIcon == "50d" || WeaIcon == "50n")
    tft.pushImage(x, y, w, h, img_50dn);
  Serial.println("Icon name: " + WeaIcon);
} // print_Img

void lineondisp()
{
  tft.drawRect(0, 0, 70, 50, TFT_CYAN);
  tft.fillCircle(35, 25, 20, TFT_RED);
  tft.setFreeFont();
  tft.setTextSize(2);
  tft.setTextColor(TFT_LIGHTGREY, TFT_RED);
  tft.drawString("C", 30, 17);
  tft.drawRect(70, 0, 250, 50, TFT_CYAN);
  // tft.drawRect(130, 0, 190, 50, TFT_CYAN);
  //  weather
  tft.drawRect(0, 50, 90, 70, TFT_CYAN);
  tft.setSwapBytes(true);
  print_Img(10, 55, weather.icon);
  // clock
  tft.drawRect(90, 50, 230, 70, TFT_CYAN);
  // vulevel
  tft.drawRect(0, 120, 160, 75, TFT_CYAN);
  tft.drawRect(160, 120, 160, 75, TFT_CYAN);

  tft.drawRect(0, 120, 80, 75, TFT_CYAN);
  tft.drawLine(80, 157, 160, 157, TFT_CYAN);
  tft.setTextColor(TFT_BLUE);

  tft.drawString("CSS", 97, 132);
  tft.drawRect(0, 202, 320, 38, TFT_CYAN);
}

int startDay = 1; // Sunday's value is 0, Saturday is 6
String week1 = "";
String week2 = "";
String week3 = "";
String week4 = "";
String week5 = "";
int newWeekStart = 1; // used to show start of next week of the month
char monthString2[37] = {"JanFebMarAprMayJunJulAugSepOctNovDec"};
int monthIndex2[122] = {0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33};
char monthName2[3] = "";
int monthLength = 0;
String months[13] = {"", "Январь", "Февраль", "Март", "Апрель", "Май", "Июнь", "Июль", "Август", "Сентябрь", "Октябрь", "Ноябрь", "Декабрь"};

void drawCalendar()
{
  showRadio = false;
  tft.fillRect(5, 52, 313, 140, TFT_LIGHTGREY);
  tft.drawRect(5, 52, 313, 140, 0x9772);
  // display a full month on a calendar
  tft.setFreeFont(&CourierCyr10pt8b);
  tft.setTextSize(1);
  tft.setTextColor(TFT_BLACK);
  tft.drawString(utf8rus(months[ntp.month()]) + " " + ntp.year() + utf8rus(" г."), 30, 55);
  tft.drawString(utf8rus("Пн Вт Ср Чт Пт Сб Вс"), 15, 75);
  // display this month

  // get number of days in month
  if (ntp.month() == 1 || ntp.month() == 3 || ntp.month() == 5 || ntp.month() == 7 || ntp.month() == 8 || ntp.month() == 10 || ntp.month() == 12)
  {
    monthLength = 31;
  }
  else
  {
    monthLength = 30;
  }
  if (ntp.month() == 2)
  {
    monthLength = 28;
  }
  startDay = startDayOfWeek(ntp.year(), ntp.month(), 1); // Sunday's value is 0
  // now build first week string
  switch (startDay)
  {
  case 1:
    // Sunday
    week1 = " 1  2  3  4  5  6  7";
    break;
  case 2:
    // Monday
    week1 = "    1  2  3  4  5  6";
    break;
  case 3:
    // Tuesday
    week1 = "       1  2  3  4  5";
    break;
  case 4:
    // Wednesday
    week1 = "          1  2  3  4";
    break;
  case 5:
    // Thursday
    week1 = "             1  2  3";
    break;
  case 6:
    // Friday
    if (monthLength == 28 || monthLength == 30)
    {
      week1 = "                1  2";
    }
    if (monthLength == 31)
    {
      week1 = "31              1  2";
    }
    break;
  case 0:
    // Saturday
    if (monthLength == 28)
    {
      week1 = "                   1";
    }
    if (monthLength == 30)
    {
      week1 = "30                 1";
    }
    if (monthLength == 31)
    {
      week1 = "30 31              1";
    }

    break;
  } // end first week
  newWeekStart = (7 - startDay) + 2;
  const char *newWeek1 = (const char *)week1.c_str();
  tft.drawString(newWeek1, 15, 95);
  // display week 2
  week2 = "";
  for (int f = newWeekStart; f < newWeekStart + 7; f++)
  {
    if (f < 10)
    {
      week2 = week2 + " " + String(f) + " ";
    }
    else
    {
      week2 = week2 + String(f) + " ";
    }
  }
  const char *newWeek2 = (const char *)week2.c_str();
  tft.drawString(newWeek2, 15, 115);
  // display week 3
  newWeekStart = (14 - startDay) + 2;
  week3 = "";
  for (int f = newWeekStart; f < newWeekStart + 7; f++)
  {
    if (f < 10)
    {
      week3 = week3 + " " + String(f) + " ";
    }
    else
    {
      week3 = week3 + String(f) + " ";
    }
  }
  const char *newWeek3 = (const char *)week3.c_str();
  tft.drawString(newWeek3, 15, 135);
  // display week 4
  newWeekStart = (21 - startDay) + 2;
  week4 = "";
  for (int f = newWeekStart; f < newWeekStart + 7; f++)
  {
    if (f < 10)
    {
      week4 = week4 + " " + String(f) + " ";
    }
    else
    {
      week4 = week4 + String(f) + " ";
    }
  }
  const char *newWeek4 = (const char *)week4.c_str();
  tft.drawString(newWeek4, 15, 155);
  // do we need a fifth week
  week5 = "";
  newWeekStart = (28 - startDay) + 2;
  // is is February?
  if (newWeekStart > 28 && ntp.month() == 2)
  {
    // do nothing unless its a leap year
    if (ntp.year() == (ntp.year() / 4) * 4)
    { // its a leap year
      week5 = "29";
    }
  }
  else
  { // print up to 30 anyway
    if (ntp.month() == 2)
    { // its February
      for (int f = newWeekStart; f < 29; f++)
      {
        week5 = week5 + String(f) + " ";
      }
      // is it a leap year
      if (ntp.year() == (ntp.year() / 4) * 4)
      { // its a leap year
        week5 = week5 + "29";
      }
    }
    else
    {
      for (int f = newWeekStart; f < 31; f++)
      {
        week5 = week5 + String(f) + " ";
      }
      // are there 31 days
      if (monthLength == 31 && week5.length() < 7)
      {
        week5 = week5 + "31";
      }
    }
  }
  const char *newWeek5 = (const char *)week5.c_str();
  tft.drawString(newWeek5, 15, 175);
  //
  // To print to Serial Monitor instead of OLED un-REM the following
  // if data is sent to Serial Monitor the OLED will not display data!!!
  /*
  Serial.println("Su Mo Tu We Th Fr Sa");
  Serial.println(week1);
  Serial.println(week2);
  Serial.println(week3);
  Serial.println(week4);
  Serial.println(week5);
  while(1);  // wait indefinitly
  */
}
/********************************************************/

// calculate first day of month
int startDayOfWeek(int y, int m, int d)
{
  static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  y -= m < 3;
  return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}
/********************************************************/
static void rebootEspWithReason(String reason)
{
  Serial.println(reason);
  delay(1000);
  ESP.restart();
}

void performUpdate(Stream &updateSource, size_t updateSize)
{
  String result = "";
  if (Update.begin(updateSize))
  {
    size_t written = Update.writeStream(updateSource);
    if (written == updateSize)
    {
      Serial.println("Written : " + String(written) + " successfully");
    }
    else
    {
      Serial.println("Written only : " + String(written) + "/" + String(updateSize) + ". Retry?");
    }
    result += "Written : " + String(written) + "/" + String(updateSize) + " [" + String((written / updateSize) * 100) + "%] \n";
    if (Update.end())
    {
      Serial.println("OTA done!");
      result += "OTA Done: ";
      if (Update.isFinished())
      {
        Serial.println("Update successfully completed. Rebooting...");
        result += "Success!\n";
      }
      else
      {
        Serial.println("Update not finished? Something went wrong!");
        result += "Failed!\n";
      }
    }
    else
    {
      Serial.println("Error Occurred. Error #: " + String(Update.getError()));
      result += "Error #: " + String(Update.getError());
    }
  }
  else
  {
    Serial.println("Not enough space to begin OTA");
    result += "Not enough space for OTA";
  }
  // http send 'result'
}

void updateFromFS(fs::FS &fs)
{
  File updateBin = fs.open("/firmware.bin");
  if (updateBin)
  {
    if (updateBin.isDirectory())
    {
      Serial.println("Error, update.bin is not a file");
      updateBin.close();
      return;
    }

    size_t updateSize = updateBin.size();

    if (updateSize > 0)
    {
      Serial.println("Trying to start update");
      performUpdate(updateBin, updateSize);
    }
    else
    {
      Serial.println("Error, file is empty");
    }

    updateBin.close();

    // when finished remove the binary from spiffs to indicate end of the process
    Serial.println("Removing update file");
    fs.remove("/firmware.bin");

    rebootEspWithReason("Rebooting to complete OTA update");
  }
  else
  {
    Serial.println("Could not load update.bin from spiffs root");
  }
}

bool downloadFirmware()
{
  HTTPClient http;
  bool stat = false;
  Serial.println(fwUrl);
  File f = SPIFFS.open("/firmware.bin", "w");
  if (f)
  {
    http.begin(fwUrl);
    int httpCode = http.GET();
    if (httpCode > 0)
    {
      if (httpCode == HTTP_CODE_OK)
      {
        Serial.println("Downloading...");
        http.writeToStream(&f);
        stat = true;
      }
    }
    else
    {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    f.close();
  }
  else
  {
    Serial.println("failed to open /firmware.bin");
  }
  http.end();

  return stat;
}

bool checkFirmware()
{
  HTTPClient http;
  http.begin(baseUrl + checkFile);
  int httpCode = http.GET();
  bool stat = false;
  String payload = http.getString();
  Serial.println(payload);
  DynamicJsonDocument json(1024);
  deserializeJson(json, payload);
  if (httpCode == HTTP_CODE_OK)
  {
    fwVersion = json["versionCode"].as<int>();
    fwName = json["fileName"].as<String>();
    fwUrl = baseUrl + fwName;
    if (fwVersion > currentVersion)
    {
      Serial.println("Firmware update available");
      stat = true;
    }
    else
    {
      Serial.println("You have the latest version");
    }
    Serial.print("Version: ");
    Serial.print(fwName);
    Serial.print("\tCode: ");
    Serial.println(fwVersion);
  }
  http.end();

  return stat;
}
void newrelease()
{
  EEPROM.write(3, 1); // UPDATE
  EEPROM.commit();
  ESP.restart();
}

void newVer()
{
  if (EEPROM.read(3) == 1)
  {
    EEPROM.write(3, 0); // don't update
    EEPROM.commit();

    tft.println("Updating. Please wait...");
    Serial.println("Firmware Updates from Github");
    if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
    {
      Serial.println("SPIFFS Mount Failed");
      rebootEspWithReason("SPIFFS Mount Failed, rebooting...");
    }

    Serial.println("Wifi connected. Checking for updates");
    if (checkFirmware())
    {
      if (SPIFFS.exists("/firmware.bin"))
      {
        SPIFFS.remove("/firmware.bin");
        Serial.println("Removed existing update file");
      }
      if (downloadFirmware())
      {
        Serial.println("Download complete");
        updateFromFS(SPIFFS);
      }
      else
      {
        tft.println("You have the latest version");
        delay(2000);
        Serial.println("Download failed");
      }
    }
    }
}
