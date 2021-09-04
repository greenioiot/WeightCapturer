


#define TFT_GREY 0x2104 // Dark grey 16 bit colour

#define _TASK_SLEEP_ON_IDLE_RUN
#define _TASK_PRIORITY
#define _TASK_WDT_IDS
#define _TASK_TIMECRITICAL


#define WIFI_AP ""
#define WIFI_PASSWORD ""



#include <Adafruit_MLX90614.h>


#include <WiFi.h>

#include <DNSServer.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <TaskScheduler.h>
#include <PubSubClient.h>


#include <ArduinoOTA.h>


#include "Alert.h" // Out of range alert icon
#include "wifi1.h" // Signal Streng WiFi
#include "wifi2.h" // Signal Streng WiFi
#include "wifi3.h" // Signal Streng WiFi
#include "wifi4.h" // Signal Streng WiFi

#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>


#include "Free_Fonts.h" // Include the header file attached to this sketch





#include "RTClib.h"
#include "Free_Fonts.h"
#include "EEPROM.h"

#define HOSTNAME "TAT"
#define PASSWORD "7650"

int PORT = 8883;
String deviceToken = "";
uint64_t chipId = 0;

String wifiName = "@WC";
float DP = 0.0;
unsigned long ms;
struct Settings
{
  char TOKEN[40] = "";
  char SERVER[40] = "mqtt.thingcontrol.io";
  int PORT = 8883;
  uint32_t ip;
} sett;


HardwareSerial hwSerial(1);
#define SERIAL1_RXPIN 25
#define SERIAL1_TXPIN 26

//unsigned long drawTime = 0;
TFT_eSPI tft = TFT_eSPI(); // Invoke custom library with default width and height
TFT_eSprite weightSprite = TFT_eSprite(&tft); // Invoke custom library with default width and height

TFT_eSprite headerSprite = TFT_eSprite(&tft); // Invoke custom library with default width and height
TFT_eSprite wifiSprite = TFT_eSprite(&tft); // Invoke custom library with default width and height
TFT_eSprite timeSprite = TFT_eSprite(&tft); // Invoke custom library with default width and height
TFT_eSprite updateSprite = TFT_eSprite(&tft); // Invoke custom library with default width and height




Scheduler runner;

float value = 0.0;
String json = "";

String str = "";
WiFiClientSecure wifiClient;
PubSubClient client(wifiClient);


Adafruit_MLX90614 mlx = Adafruit_MLX90614();


int rssi = 0; ;

// # Add On
#include <TimeLib.h>
#include <ArduinoJson.h>
#include "time.h"

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600 * 7;

const int   daylightOffset_sec = 3600;

bool connectWifi = false;
StaticJsonDocument<400> doc;
bool validEpoc = false;
String dataJson = "";
unsigned long _epoch = 0;
WiFiManager wifiManager;
unsigned long time_s = 0;

struct tm timeinfo;




int reading = 0; // Value to be displayed
int d = 0; // Variable used for the sinewave test waveform
boolean range_error = 0;
int8_t ramp = 1;



#define HEADER_WIDTH  160
#define HEADER_HEIGHT 30
#define WEIGHT_WIDTH 160
#define WEIGHT_HEIGHT 60
#define TIME_WIDTH  100
#define TIME_HEIGHT 35

#define WiFi_WIDTH  30
#define WiFi_HEIGHT 30


// Pause in milliseconds between screens, change to 0 to time font rendering
#define WAIT 100

// Callback methods prototypes
void tCallback();

void t2CallShowEnv();
void t3CallSendData();
//void t4CallPrintPMS7003();
void t5CallSendAttribute();
void t7showTime();
// Tasks

Task t2(5000, TASK_FOREVER, &t2CallShowEnv);
Task t3(60000, TASK_FOREVER, &t3CallSendData);
//
//Task t4(2000, TASK_FOREVER, &t4CallPrintPMS7003);  //adding task to the chain on creation
Task t5(10400000, TASK_FOREVER, &t5CallSendAttribute);  //adding task to the chain on creation
Task t7(1000, TASK_FOREVER, &t7showTime);

//flag for saving data
bool shouldSaveConfig = false;


class IPAddressParameter : public WiFiManagerParameter
{
  public:
    IPAddressParameter(const char *id, const char *placeholder, IPAddress address)
      : WiFiManagerParameter("")
    {
      init(id, placeholder, address.toString().c_str(), 16, "", WFM_LABEL_BEFORE);
    }

    bool getValue(IPAddress &ip)
    {
      return ip.fromString(WiFiManagerParameter::getValue());
    }
};

class IntParameter : public WiFiManagerParameter
{
  public:
    IntParameter(const char *id, const char *placeholder, long value, const uint8_t length = 10)
      : WiFiManagerParameter("")
    {
      init(id, placeholder, String(value).c_str(), length, "", WFM_LABEL_BEFORE);
    }

    long getValue()
    {
      return String(WiFiManagerParameter::getValue()).toInt();
    }
};



char  char_to_byte(char c)
{
  if ((c >= '0') && (c <= '9'))
  {
    return (c - 0x30);
  }
  if ((c >= 'A') && (c <= 'F'))
  {
    return (c - 55);
  }
}

String read_String(char add)
{
  int i;
  char data[100]; //Max 100 Bytes
  int len = 0;
  unsigned char k;
  k = EEPROM.read(add);
  while (k != '\0' && len < 500) //Read until null character
  {
    k = EEPROM.read(add + len);
    data[len] = k;
    len++;
  }
  data[len] = '\0';

  return String(data);
}


void getChipID() {
  chipId = ESP.getEfuseMac(); //The chip ID is essentially its MAC address(length: 6 bytes).
  Serial.printf("ESP32ChipID=%04X", (uint16_t)(chipId >> 32)); //print High 2 bytes
  Serial.printf("%08X\n", (uint32_t)chipId); //print Low 4bytes.

}

void setupOTA()
{
  //Port defaults to 8266
  //ArduinoOTA.setPort(8266);

  //Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(HOSTNAME);

  //No authentication by default
  ArduinoOTA.setPassword(PASSWORD);

  //Password can be set with it's md5 value as well
  //MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  //ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]()
  {
    Serial.println("Start Updating....");

    Serial.printf("Start Updating....Type:%s\n", (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem");
  });

  ArduinoOTA.onEnd([]()
  {

    Serial.println("Update Complete!");

    ESP.restart();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
  {
    String pro = String(progress / (total / 100)) + "%";
    int progressbar = (progress / (total / 100));
    //int progressbar = (progress / 5) % 100;
    //int pro = progress / (total / 100);

    drawUpdate(progressbar, 110, 0);


    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error)
  {
    Serial.printf("Error[%u]: ", error);
    String info = "Error Info:";
    switch (error)
    {
      case OTA_AUTH_ERROR:
        info += "Auth Failed";
        Serial.println("Auth Failed");
        break;

      case OTA_BEGIN_ERROR:
        info += "Begin Failed";
        Serial.println("Begin Failed");
        break;

      case OTA_CONNECT_ERROR:
        info += "Connect Failed";
        Serial.println("Connect Failed");
        break;

      case OTA_RECEIVE_ERROR:
        info += "Receive Failed";
        Serial.println("Receive Failed");
        break;

      case OTA_END_ERROR:
        info += "End Failed";
        Serial.println("End Failed");
        break;
    }


    Serial.println(info);
    ESP.restart();
  });

  ArduinoOTA.begin();
}

String uint64ToString(uint64_t input) {
  String result = "";
  uint8_t base = 10;

  do {
    char c = input % base;
    input /= base;

    if (c < 10)
      c += '0';
    else
      c += 'A' - 10;
    result = c + result;
  } while (input);
  return result;
}


void setupWIFI()
{
  WiFi.setHostname(uint64ToString(chipId).c_str());

  byte count = 0;
  while (WiFi.status() != WL_CONNECTED && count < 10)
  {
    count ++;
    delay(500);
    Serial.print(".");
  }


  if (WiFi.status() == WL_CONNECTED)
    Serial.println("Connecting...OK.");
  else
    Serial.println("Connecting...Failed");

}

void writeString(char add, String data)
{
  int _size = data.length();
  int i;
  for (i = 0; i < _size; i++)
  {
    EEPROM.write(add + i, data[i]);
  }
  EEPROM.write(add + _size, '\0'); //Add termination null character for String Data
  EEPROM.commit();
}

void _writeEEPROM(String data) {
  //Serial.print("Writing Data:");
  //Serial.println(data);
  writeString(10, data);  //Address 10 and String type data
  delay(10);
}


void splash() {

  tft.init();
  // Swap the colour byte order when rendering
  tft.setSwapBytes(true);
  tft.setRotation(1);  // landscape

  tft.fillScreen(TFT_BLACK);
  tft.setCursor(5, (tft.height() / 2) - 50, 1);

  tft.setTextColor(TFT_YELLOW);


  tft.setTextFont(4);
  tft.println("Weight");
  tft.println("    Capture");
  tft.println("        by");
  tft.println("            TAT");

  delay(5000);
  //  tft.setTextPadding(180);


  Serial.println(F("Start..."));
  for ( int i = 0; i < 250; i++)
  {
    tft.drawString(".", 1 + 2 * i, 300, GFXFF);
    delay(10);
    //    Serial.println(i);
  }
  Serial.println("end");
}


void _initLCD() {
  tft.fillScreen(TFT_BLACK);
  // TFT
  splash();
  // MLX
  mlx.begin();
}


void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
  Serial.print("saveConfigCallback:");
  Serial.println(sett.TOKEN);
}



void setup(void) {

  Serial.begin(115200);
  // make the pins outputs:
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);

  delay(1000);
  Serial.println("Start");
  hwSerial.begin(4800, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN);

  EEPROM.begin(512);
  _initLCD();
  if (EEPROM.read(10) == 255 ) {
    _writeEEPROM("147.50.151.130");
  }
  getChipID();
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  tft.drawString("WiFi Setting", 5, (tft.height() / 2) - 30, 4);
  tft.drawString("(120 Sec)", 40, (tft.height() / 2) + 5, 4);
  tft.setTextColor(TFT_BLUE);


  wifiManager.setTimeout(120);

  wifiManager.setAPCallback(configModeCallback);
  std::vector<const char *> menu = {"wifi", "info", "sep", "restart", "exit"};
  wifiManager.setMenu(menu);
  wifiManager.setClass("invert");
  wifiManager.setConfigPortalTimeout(120); // auto close configportal after n seconds
  wifiManager.setAPClientCheck(true); // avoid timeout if client connected to softap
  wifiManager.setBreakAfterConfig(true);   // always exit configportal even if wifi save fails

  WiFiManagerParameter blnk_Text("<b>Device setup.</b> <br>");
  sett.TOKEN[39] = '\0';   //add null terminator at the end cause overflow
  sett.SERVER[39] = '\0';   //add null terminator at the end cause overflow
  WiFiManagerParameter blynk_Token( "blynktoken", "device Token",  sett.TOKEN, 40);
  WiFiManagerParameter blynk_Server( "blynkserver", "Server",  sett.SERVER, 40);
  IntParameter blynk_Port( "blynkport", "Port",  sett.PORT);

  wifiManager.addParameter( &blnk_Text );
  wifiManager.addParameter( &blynk_Token );
  wifiManager.addParameter( &blynk_Server );
  wifiManager.addParameter( &blynk_Port );


  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);




  wifiName.concat(uint64ToString(chipId));
  if (!wifiManager.autoConnect(wifiName.c_str())) {
    Serial.println("failed to connect and hit timeout");
    //    ESP.reset();
    //delay(1000);
  }
  deviceToken = wifiName.c_str();


  configTime(gmtOffset_sec, 0, ntpServer);
  client.setServer( sett.SERVER, sett.PORT );


  setupWIFI();
  setupOTA();



  runner.init();
  Serial.println("Initialized scheduler");


  runner.addTask(t2);
  //  Serial.println("added t2");
  runner.addTask(t3);
  //  Serial.println("added t3");
  //  runner.addTask(t4);
  //  Serial.println("added t4");
  //  runner.addTask(t6);
  runner.addTask(t7);
  delay(2000);

  t2.enable();
  //  Serial.println("Enabled t2");
  t3.enable();
  //  Serial.println("Enabled t3");
  //  t4.enable();
  //  Serial.println("Enabled t4");
  //  t6.enable();
  t7.enable();

  //  Serial.println("Initialized scheduler");
  tft.begin();

  tft.setRotation(1);

  headerSprite.createSprite(HEADER_WIDTH, HEADER_HEIGHT);
  headerSprite.fillSprite(TFT_BLACK);
  wifiSprite.createSprite(WiFi_WIDTH, WiFi_HEIGHT);
  wifiSprite.fillSprite(TFT_BLACK);
  timeSprite.createSprite(TIME_WIDTH, TIME_HEIGHT);
  timeSprite.fillSprite(TFT_BLACK);

  weightSprite.createSprite(WEIGHT_WIDTH, WEIGHT_HEIGHT);
  weightSprite.fillSprite(TFT_BLACK);
  tft.fillScreen(TFT_BLACK);

  drawTime();

  header(deviceToken.c_str());

}



void loop() {

  if (ms % 300 == 0)
  {

    //    char readData[14];//The character array is used as buffer to read into.
    //    int x = hwSerial.readBytesUntil('\r\n',readData,13);//It require two things, variable name to read into, number of bytes to read.
    //    Serial.println(x);
    //    String s = String(readData);
    //    s.trim();
    //    Serial.print("W:");
    //    Serial.println(s);//send back the 10 bytes of data.
  
    while (hwSerial.available()) {
      char c = hwSerial.read();
      Serial.print(c); Serial.print(" ");
      str += c;
      delay(2);
    }
    Serial.print("len:");Serial.println(str.length());
    if (str.length() > 0){
      
      Serial.println(str);
    }else{
      str = "";
    }
    drawWeight();
  }
  ArduinoOTA.handle();
  runner.execute();

  ms = millis();
  if (ms % 60000 == 0)
  {

    Serial.println("Attach WiFi for，OTA "); Serial.println(WiFi.RSSI() );

    setupWIFI();

    setupOTA();
    tCallback();






  }

}

void drawWeight() {

  char buf[10];
  byte len = 3;

  len = 5;
  Serial.print(millis()); Serial.print(":");
  Serial.println(value++);
  dtostrf(value, len, 1, buf);
  buf[len] = ' '; buf[len + 1] = 0; // Add blanking space and terminator, helps to centre text too!

  weightSprite.fillScreen(TFT_BLACK);
  weightSprite.setTextColor(TFT_WHITE);
  weightSprite.drawString(buf, 0, 0, 7); // Value in middle

  weightSprite.drawString("Kg", 140, 15, 2); // Value in middle

  weightSprite.pushSprite(0, 45);


}
String a0(int n) {
  return (n < 10) ? "0" + String(n) : String(n);
}

void drawTime() {
  unsigned long NowTime = _epoch + ((millis() - time_s) / 1000) + (7 * 3600);
  String dateS = "";
  String timeS = "";

  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return;
  }
  dateS = a0(timeinfo.tm_mday) + "/" + a0(timeinfo.tm_mon + 1) + "/" + String(timeinfo.tm_year + 1900);
  timeS = a0(timeinfo.tm_hour) + ":" + a0(timeinfo.tm_min)  + ":" + a0(timeinfo.tm_sec);
  timeSprite.fillScreen(TFT_BLACK);

  timeSprite.setTextColor(TFT_ORANGE);
  timeSprite.drawString(dateS, 0, 0, 2); // Font 4 for fast drawing with background
  timeSprite.drawString(timeS, 0, 15, 2); // Font 4 for fast drawing with background

  timeSprite.pushSprite(0, 0);

}

void drawUpdate(int num, int x, int y)
{
  updateSprite.createSprite(25, 20);
  updateSprite.fillScreen(TFT_BLACK);
  //  updateSprite.setFreeFont(FSB9);
  updateSprite.setTextColor(TFT_YELLOW);
  updateSprite.setTextSize(1);
  updateSprite.drawNumber(num, 0, 4);
  updateSprite.drawString("%", 20, 4);
  updateSprite.pushSprite(x, y);
  updateSprite.deleteSprite();
}



//====================================================================================
// This is the function to draw the icon stored as an array in program memory (FLASH)
//====================================================================================

// To speed up rendering we use a 64 pixel buffer
#define BUFF_SIZE 64

// Draw array "icon" of defined width and height at coordinate x,y
// Maximum icon size is 255x255 pixels to avoid integer overflow

void drawIcon(const unsigned short* icon, int16_t x, int16_t y, int8_t width, int8_t height) {

  uint16_t  pix_buffer[BUFF_SIZE];   // Pixel buffer (16 bits per pixel)

  tft.startWrite();

  // Set up a window the right size to stream pixels into
  tft.setAddrWindow(x, y, width, height);

  // Work out the number whole buffers to send
  uint16_t nb = ((uint16_t)height * width) / BUFF_SIZE;

  // Fill and send "nb" buffers to TFT
  for (int i = 0; i < nb; i++) {
    for (int j = 0; j < BUFF_SIZE; j++) {
      pix_buffer[j] = pgm_read_word(&icon[i * BUFF_SIZE + j]);
    }
    tft.pushColors(pix_buffer, BUFF_SIZE);
  }

  // Work out number of pixels not yet sent
  uint16_t np = ((uint16_t)height * width) % BUFF_SIZE;

  // Send any partial buffer left over
  if (np) {
    for (int i = 0; i < np; i++) pix_buffer[i] = pgm_read_word(&icon[nb * BUFF_SIZE + i]);
    tft.pushColors(pix_buffer, np);
  }

  tft.endWrite();
}

// Print the header for a display screen
void header(const char *string)
{
  headerSprite.setTextSize(1);


  headerSprite.setTextColor(TFT_BLUE);

  headerSprite.drawString(string, 5, 2, 2); // Font 4 for fast drawing with background

  headerSprite.pushSprite(0, tft.height() - 15);


}

void drawWiFi( )
{

  wifiSprite.fillScreen(TFT_BLACK);
  Serial.print("RSSI:");
  Serial.println(rssi);
  if (rssi == 1)
    wifiSprite.pushImage(0 , 0, wifi1Width, wifi1Height, wifi4);
  if (rssi == 2)
    wifiSprite.pushImage(0 , 0, wifi1Width, wifi1Height, wifi3);
  if (rssi == 3)
    wifiSprite.pushImage(0 , 0, wifi1Width, wifi1Height, wifi2);
  if (rssi >= 4)
    wifiSprite.pushImage(0 , 0, wifi1Width, wifi1Height, wifi1);

  wifiSprite.pushSprite(140, 0);
}


// Draw a + mark centred on x,y
void drawDatum(int x, int y)
{
  tft.drawLine(x - 5, y, x + 5, y, TFT_GREEN);
  tft.drawLine(x, y - 5, x, y + 5, TFT_GREEN);
}

void composeJson() {


  json = "";

  //  json.concat(" {\"temp\":");
  //
  //  json.concat(temp);
  //  json.concat(",\"hum\":");
  //  json.concat(hum);
  //  json.concat(",\"pres\":");
  //  json.concat(pres);
  //  json.concat(",\"DP\":");
  //  json.concat(DP);
  //
  //  json.concat(",\"co2\":");
  //  json.concat(sgp.eCO2);
  //  json.concat(",\"voc\":");
  //  json.concat(sgp.TVOC);

  json.concat(",\"rssi\":");
  json.concat(WiFi.RSSI());

  json.concat("}");
  Serial.println(json);



}



void tCallback() {
  Scheduler &s = Scheduler::currentScheduler();
  Task &t = s.currentTask();

  //  Serial.print("Task: "); Serial.print(t.getId()); Serial.print(":\t");
  //  Serial.print(millis()); Serial.print("\tStart delay = "); Serial.println(t.getStartDelay());
  //  delay(10);

  //  if (t1.isFirstIteration()) {
  //    runner.addTask(t2);
  //    t2.enable();
  //    //    Serial.println("t1CallgetProbe: enabled t2CallshowEnv and added to the chain");
  //  }


}



void t2CallShowEnv() {
  drawWiFi();

  Serial.print("deviceToken:");
  Serial.println(deviceToken);
}
void t3CallSendData() {
  composeJson();

  tft.setTextColor(0xFFFF);
  int mapX = 315;
  int mapY = 30;
  Serial.println(WL_CONNECTED); Serial.print("(WiFi.status():"); Serial.println(WiFi.status());

  rssi = map(WiFi.RSSI(), -100, -30, 1, 4);
  Serial.println(rssi);



  if (client.connect("weightCapture", deviceToken.c_str(), NULL)) {
    Serial.println("******************************************************Connected!");

    client.publish("v1/devices/me/telemetry", json.c_str());
  }

}
//void t4CallPrintPMS7003();
void t5CallSendAttribute() {}

void t7showTime() {


  drawTime();

}
