
// Adafruit GFX Library - Version: Latest (1.11.3)
#include <Adafruit_GFX.h>
#include <Adafruit_GrayOLED.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <gfxfont.h>

// ArduinoJson - Version: 6.12.0
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

// NTPClient
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <TimeLib.h> // Include Arduino time library

#include <ESP8266WiFi.h>
#include <Adafruit_ST7735.h>

#include <WiFiManager.h> // V0.16.0  https://github.com/tzapu/WiFiManager

#include <RotaryEncoder.h>

// Color definitions
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
#define GREY 0xC618

#define cs D2
#define dc D3
#define rst D4

//------------------------Rotary Encoder----------------------------------
// #define ENC_A D6
// #define ENC_B D8
// #define BUTTON D0

// Example for ESP8266 NodeMCU with input signals on pin D5 and D6
#define PIN_IN1 D5
#define PIN_IN2 D6

//------------------------touch sensor----------------------------------
#define TOUCH_PIN D8
int touchVal = 0;
int menuPosition = 0;

const int menuSize = 3;
String menuViews[menuSize];

//------------------------Rotary Encoder----------------------------------

// // Setup a RotaryEncoder with 2 steps per latch for the 2 signal input pins:
// RotaryEncoder encoder(PIN_IN1, PIN_IN2, RotaryEncoder::LatchMode::TWO03);

// A pointer to the dynamic created rotary encoder instance.
// This will be done in setup()
// RotaryEncoder *encoder = nullptr;

// /**
//  * @brief The interrupt service routine will be called on any change of one of the input signals.
//  */
// IRAM_ATTR void checkPosition()
// {
//     encoder->tick(); // just call tick() to check the state.
// }
int rCounter = 0;
int currentState;
int initState;

int angle = 0;
int aState;
int aLastState;

//----------------------------------------------------------------

const long utcOffsetInSeconds = 7200; // (GMT time) and an offset of 1 hour ( ==> GMT + 2 time zone) which is equal to 7200 seconds

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

const char *ssid = "yourSSID";         // SSID of local network
const char *password = "yourPassword"; // Password on network
String APIKEY = "7a782bf03ae2fc6a4af0c33356c9dba3";
String CityID = "3029392"; // Bussy-Saint-Georges, FR
int TimeZone = 2;          // GMT +2

WiFiClient client;
char servername[] = "api.openweathermap.org"; // remote server we will connect to
String result;

boolean night = false;
int counter = 600;
String weatherDescription = "";
String weatherLocation = "";
String Temperature = "";

int weatherID = 0;

extern unsigned char cloud[];
extern unsigned char thunder[];
extern unsigned char wind[];

Adafruit_ST7735 tft = Adafruit_ST7735(cs, dc, rst);

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

String hours = "00";
String minutes = "00";
String seconds = "00";

String formattedDate;
String timeStamp;

unsigned long unix_epoch;

void setup()
{

    Serial.begin(115200);

    pinMode(TOUCH_PIN, INPUT_PULLUP);

    pinMode(1, FUNCTION_3);
    pinMode(3, FUNCTION_3);

    pinMode(1, INPUT);
    pinMode(3, INPUT);

    aLastState = digitalRead(1);

    // use TWO03 mode when PIN_IN1, PIN_IN2 signals are both LOW or HIGH in latch position.
    // encoder = new RotaryEncoder(1, 3, RotaryEncoder::LatchMode::TWO03);

    // // register interrupt routine
    // attachInterrupt(digitalPinToInterrupt(1), checkPosition, CHANGE);
    // attachInterrupt(digitalPinToInterrupt(3), checkPosition, CHANGE);

    // // Read the initial state of CLK
    // initState = encoder->getPosition();

    // r.setLeftRotationHandler(showDirection);
    // r.setRightRotationHandler(showDirection);

    // pinMode(ENC_A, INPUT);
    // pinMode(ENC_B, INPUT);
    // attachInterrupt(digitalPinToInterrupt(ENC_A), read_encoder, CHANGE);
    // attachInterrupt(digitalPinToInterrupt(ENC_B), read_encoder, CHANGE);

    // pinMode(TOUCH_PIN, INPUT);
    menuViews[0] = "settings";
    menuViews[1] = "weather";
    menuViews[2] = "music";

    tft.initR(INITR_BLACKTAB); // initialize a ST7735S chip, black tab
    tft.fillScreen(BLACK);

    Serial.println("Connecting");

    tft.setCursor(30, 80);
    tft.setTextColor(WHITE);
    tft.setTextSize(1);
    tft.print("Connecting...");

    // WiFiManager
    // Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    // reset saved settings
    // wifiManager.resetSettings();

    // set custom ip for portal
    wifiManager.setAPStaticIPConfig(IPAddress(10, 0, 1, 1), IPAddress(10, 0, 1, 1), IPAddress(255, 255, 255, 0));

    // fetches ssid and pass from eeprom and tries to connect
    // if it does not connect it starts an access point with the specified name
    // here  "AutoConnectAP"
    // and goes into a blocking loop awaiting configuration
    wifiManager.autoConnect("you.nes-8fi-manager");
    // or use this for auto generated name ESP + ChipID
    // wifiManager.autoConnect();

    // if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
    tft.print("connected...yeey");

    timeClient.begin();

    // menu
    MenuChanged();
    drawOnOFF();
}

String dir = "";

void loop()
{

    //--------------touch------------------
    touchVal = digitalRead(TOUCH_PIN);

    if (touchVal == HIGH)
    {
        Serial.println("Next!");
        menuPosition++;
        Serial.println(menuPosition);
        if (menuPosition == 3)
        {
            menuPosition = 0;
        }

        MenuChanged();
    }

    if (counter == 600 && menuViews[menuPosition] == "weather") // Get new data every 30 minutes
    {
        counter = 0;
        getWeatherData();
    }

    timeClient.update();

    unix_epoch = timeClient.getEpochTime(); // get UNIX Epoch time

    if (menuViews[menuPosition] == "weather")
    {
        RTC_display();
    }
    // increment counter
    counter++;
    // Serial.println(counter);
    // delay(1000);
    // delay(200);    // wait 200ms

    //--------------touch------------------

    // static int pos = 0;

    // encoder->tick(); // just call tick() to check the state.

    // int newPos = encoder->getPosition();
    // if (pos != newPos)
    // {

    //     if ((int)(encoder->getDirection()) > 0)
    //     {
    //         dir = "left";
    //     }
    //     else
    //     {
    //         dir = "right";
    //     }
    //     // tft.setCursor(16, 136);
    //     tft.setCursor(15, 50);
    //     tft.setTextColor(GREEN, BLACK); // set text color to green and black background
    //     tft.print(newPos);

    //     tft.setCursor(15, 80);
    //     tft.setTextColor(RED, BLACK); // set text color to green and black background
    //     tft.print(dir);
    //     pos = newPos;
    // } // if

    // encoder_value();
    compute_angle();
}

void compute_angle()
{
    aState = digitalRead(1);

    if (aState != aLastState)
    {
        if (digitalRead(3) != aState)
        {
            rCounter++;
            angle++;
        }
        else
        {
            rCounter--;
            angle--;
        }
        if (rCounter >= 30)
        {
            rCounter = 0;
        }

        tft.setCursor(15, 110);
        tft.setTextColor(YELLOW, BLACK); // set text color to green and black background
        tft.print("Position: ");
        tft.print(int(angle * (-1.8)));
        tft.print("deg");
    }
    aLastState = aState;
}

// void encoder_value()
// {
//     encoder->tick(); // just call tick() to check the state.
//     // Read the current state of CLK
//     currentState = encoder->getPosition();
//     // If last and current state of CLK are different, then we can be sure that the pulse occurred
//     if (currentState != initState)
//     {
//         // Encoder is rotating counterclockwise so we decrement the counter
//         if (encoder->getPosition() != currentState)
//         {
//             rCounter++;
//         }
//         else
//         {
//             // Encoder is rotating clockwise so we increment the counter
//             rCounter--;
//         }
//         // print the value in the serial monitor window
//         tft.setCursor(15, 120);
//         tft.setTextColor(RED, BLACK); // set text color to green and black background
//         tft.print(rCounter);
//     }
//     // Remember last CLK state for next cycle
//     initState = currentState;
// }

void drawOnOFF()
{
    String view = menuViews[menuPosition];
    tft.fillRect(19, 51, 50, 16, BLACK);
    tft.drawCircle(29, 58, 7, WHITE);
    tft.drawCircle(49, 58, 7, WHITE);
    tft.drawLine(29, 51, 49, 51, WHITE);
    tft.drawLine(29, 65, 49, 65, WHITE);
    tft.fillRect(30, 52, 18, 13, BLACK);
    if (view == "settings")
        tft.fillCircle(49, 58, 4, CYAN);
    else
        tft.fillCircle(29, 58, 4, CYAN);
}

void MenuChanged()
{

    String view = menuViews[menuPosition];
    Serial.println(view);
    if (view == "weather")
    {
        /* code */
        clearScreen();
        RTC_display();
        getWeatherData();
    }

    if (view == "settings")
    {
        /* code */
        clearScreen();
        DisplaySettingsView();
        drawOnOFF();
    }
    if (view == "music")
    {
        /* code */
        clearScreen();
        DisplayMusicView();
        drawOnOFF();
    }
}

void DisplayMusicView()
{
    tft.setCursor(15, 20);
    tft.setTextColor(RED, BLACK); // set text color to green and black background
    tft.print("Let Play Music!");

    tft.setCursor(15, 40);
    tft.setTextColor(YELLOW, BLACK);
    tft.print("ON/OFF");
    tft.print("VOLUME");
}

void DisplaySettingsView()
{
    tft.setCursor(15, 20);
    tft.setTextColor(CYAN, BLACK); // set text color to green and black background
    tft.print("Settings!");
}

void RTC_display()
{
    char dow_matrix[7][10] = {"SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY"};
    byte x_pos[7] = {29, 29, 23, 11, 17, 29, 17};
    static byte previous_dow = 0;
    // print day of the week
    //  if( previous_dow != weekday(unix_epoch) )
    //  {
    //    previous_dow = weekday(unix_epoch);
    //    tft.fillRect(11, 55, 108, 14, ST7735_BLACK);     // draw rectangle (erase day from the display)
    //    tft.setCursor(x_pos[previous_dow-1], 55);
    //    tft.setTextColor(ST7735_CYAN, BLACK);     // set text color to cyan and black background
    //    tft.print( dow_matrix[previous_dow-1] );
    //  }
    //
    //  // print date
    //  tft.setCursor(4, 79);
    //  tft.setTextColor(ST7735_YELLOW, ST7735_BLACK);     // set text color to yellow and black background
    //  tft.printf( "%02u-%02u-%04u", day(unix_epoch), month(unix_epoch), year(unix_epoch) );
    // print time
    // tft.setCursor(16, 136);
    tft.setCursor(15, 20);
    tft.setTextColor(GREEN, BLACK); // set text color to green and black background
    tft.printf("%02u:%02u:%02u", hour(unix_epoch), minute(unix_epoch), second(unix_epoch));
}

void getWeatherData() // client function to send/receive GET request data.
{
    tft.print("Getting Weather Data");
    //  Serial.println("Getting Weather Data");
    if (client.connect(servername, 80))
    { // starts client connection, checks for connection
        client.println("GET /data/2.5/forecast?id=" + CityID + "&units=metric&cnt=1&APPID=" + APIKEY);
        client.println("Host: api.openweathermap.org");
        client.println("User-Agent: ArduinoWiFi/1.1");
        client.println("Connection: close");
        client.println();
        tft.print("connection to server success");
    }
    else
    {
        tft.print("connection to server fail!");
        Serial.println("connection failed"); // error message if no client connect
        Serial.println();
    }
    //
    while (client.connected() && !client.available())
        delay(1); // waits for data

    Serial.println("Waiting for data");

    while (client.connected() || client.available())
    {                           // connected or data available
        char c = client.read(); // gets byte from ethernet buffer
        result = result + c;
    }

    client.stop(); // stop client
    result.replace('[', ' ');
    result.replace(']', ' ');
    Serial.println(result);

    char jsonArray[result.length() + 1];
    result.toCharArray(jsonArray, sizeof(jsonArray));
    jsonArray[result.length() + 1] = '\0';

    DynamicJsonDocument doc(1024);
    deserializeJson(doc, jsonArray);

    String location = doc["city"]["name"];
    String temperature = doc["list"]["main"]["temp"];
    String weather = doc["list"]["weather"]["main"];
    String description = doc["list"]["weather"]["description"];
    String idString = doc["list"]["weather"]["id"];
    String timeS = doc["list"]["dt_txt"];

    timeS = convertGMTTimeToLocal(timeS);

    int length = temperature.length();
    if (length == 5)
    {
        temperature.remove(length - 1);
    }

    Temperature = temperature;
    Serial.println(location);
    Serial.println(weather);
    Serial.println(temperature);
    Serial.println(description);
    Serial.println(temperature);
    Serial.println(timeS);

    clearScreen();

    weatherID = idString.toInt();
    printData(timeS, temperature, timeS, weatherID);
}

void printData(String timeString, String temperature, String time, int weatherID)
{

    Serial.println("weatherID : ");
    Serial.println(weatherID);

    printWeatherIcon(weatherID);

    tft.setCursor(27, 132);
    tft.setTextColor(CYAN);
    tft.setTextSize(2);
    tft.print(temperature);

    tft.setCursor(83, 130);
    tft.setTextColor(RED);
    tft.setTextSize(1);
    tft.print("o");
    tft.setCursor(93, 132);
    tft.setTextColor(YELLOW);
    tft.setTextSize(2);
    tft.print("C");
}

void printWeatherIcon(int id)
{
    switch (id)
    {
    case 800:
        drawClearWeather();
        break;
    case 801:
        drawFewClouds();
        break;
    case 802:
        drawFewClouds();
        break;
    case 803:
        drawCloud();
        break;
    case 804:
        drawCloud();
        break;

    case 200:
        drawThunderstorm();
        break;
    case 201:
        drawThunderstorm();
        break;
    case 202:
        drawThunderstorm();
        break;
    case 210:
        drawThunderstorm();
        break;
    case 211:
        drawThunderstorm();
        break;
    case 212:
        drawThunderstorm();
        break;
    case 221:
        drawThunderstorm();
        break;
    case 230:
        drawThunderstorm();
        break;
    case 231:
        drawThunderstorm();
        break;
    case 232:
        drawThunderstorm();
        break;

    case 300:
        drawLightRain();
        break;
    case 301:
        drawLightRain();
        break;
    case 302:
        drawLightRain();
        break;
    case 310:
        drawLightRain();
        break;
    case 311:
        drawLightRain();
        break;
    case 312:
        drawLightRain();
        break;
    case 313:
        drawLightRain();
        break;
    case 314:
        drawLightRain();
        break;
    case 321:
        drawLightRain();
        break;

    case 500:
        drawLightRainWithSunOrMoon();
        break;
    case 501:
        drawLightRainWithSunOrMoon();
        break;
    case 502:
        drawLightRainWithSunOrMoon();
        break;
    case 503:
        drawLightRainWithSunOrMoon();
        break;
    case 504:
        drawLightRainWithSunOrMoon();
        break;
    case 511:
        drawLightRain();
        break;
    case 520:
        drawModerateRain();
        break;
    case 521:
        drawModerateRain();
        break;
    case 522:
        drawHeavyRain();
        break;
    case 531:
        drawHeavyRain();
        break;

    case 600:
        drawLightSnowfall();
        break;
    case 601:
        drawModerateSnowfall();
        break;
    case 602:
        drawHeavySnowfall();
        break;
    case 611:
        drawLightSnowfall();
        break;
    case 612:
        drawLightSnowfall();
        break;
    case 615:
        drawLightSnowfall();
        break;
    case 616:
        drawLightSnowfall();
        break;
    case 620:
        drawLightSnowfall();
        break;
    case 621:
        drawModerateSnowfall();
        break;
    case 622:
        drawHeavySnowfall();
        break;

    case 701:
        drawFog();
        break;
    case 711:
        drawFog();
        break;
    case 721:
        drawFog();
        break;
    case 731:
        drawFog();
        break;
    case 741:
        drawFog();
        break;
    case 751:
        drawFog();
        break;
    case 761:
        drawFog();
        break;
    case 762:
        drawFog();
        break;
    case 771:
        drawFog();
        break;
    case 781:
        drawFog();
        break;

    default:
        break;
    }
}

String convertGMTTimeToLocal(String timeS)
{
    int length = timeS.length();
    timeS = timeS.substring(length - 8, length - 6);
    int time = timeS.toInt();
    time = time + TimeZone;

    if (time > 21 || time < 7)
    {
        night = true;
    }
    else
    {
        night = false;
    }
    timeS = String(time) + ":00";
    return timeS;
}

void clearScreen()
{
    tft.fillScreen(BLACK);
}

void drawClearWeather()
{
    if (night)
    {
        drawTheMoon();
    }
    else
    {
        drawTheSun();
    }
}

void drawFewClouds()
{
    if (night)
    {
        drawCloudAndTheMoon();
    }
    else
    {
        drawCloudWithSun();
    }
}

void drawTheSun()
{
    tft.fillCircle(64, 80, 26, YELLOW);
}

void drawTheFullMoon()
{
    tft.fillCircle(64, 80, 26, GREY);
}

void drawTheMoon()
{
    tft.fillCircle(64, 80, 26, GREY);
    tft.fillCircle(75, 73, 26, BLACK);
}

void drawCloud()
{
    tft.drawBitmap(0, 35, cloud, 128, 90, GREY);
}

void drawCloudWithSun()
{
    tft.fillCircle(73, 70, 20, YELLOW);
    tft.drawBitmap(0, 36, cloud, 128, 90, BLACK);
    tft.drawBitmap(0, 40, cloud, 128, 90, GREY);
}

void drawLightRainWithSunOrMoon()
{
    if (night)
    {
        drawCloudTheMoonAndRain();
    }
    else
    {
        drawCloudSunAndRain();
    }
}

void drawLightRain()
{
    tft.drawBitmap(0, 35, cloud, 128, 90, GREY);
    tft.fillRoundRect(50, 105, 3, 13, 1, BLUE);
    tft.fillRoundRect(65, 105, 3, 13, 1, BLUE);
    tft.fillRoundRect(80, 105, 3, 13, 1, BLUE);
}

void drawModerateRain()
{
    tft.drawBitmap(0, 35, cloud, 128, 90, GREY);
    tft.fillRoundRect(50, 105, 3, 15, 1, BLUE);
    tft.fillRoundRect(57, 102, 3, 15, 1, BLUE);
    tft.fillRoundRect(65, 105, 3, 15, 1, BLUE);
    tft.fillRoundRect(72, 102, 3, 15, 1, BLUE);
    tft.fillRoundRect(80, 105, 3, 15, 1, BLUE);
}

void drawHeavyRain()
{
    tft.drawBitmap(0, 35, cloud, 128, 90, GREY);
    tft.fillRoundRect(43, 102, 3, 15, 1, BLUE);
    tft.fillRoundRect(50, 105, 3, 15, 1, BLUE);
    tft.fillRoundRect(57, 102, 3, 15, 1, BLUE);
    tft.fillRoundRect(65, 105, 3, 15, 1, BLUE);
    tft.fillRoundRect(72, 102, 3, 15, 1, BLUE);
    tft.fillRoundRect(80, 105, 3, 15, 1, BLUE);
    tft.fillRoundRect(87, 102, 3, 15, 1, BLUE);
}

void drawThunderstorm()
{
    tft.drawBitmap(0, 40, thunder, 128, 90, YELLOW);
    tft.drawBitmap(0, 35, cloud, 128, 90, GREY);
    tft.fillRoundRect(48, 102, 3, 15, 1, BLUE);
    tft.fillRoundRect(55, 102, 3, 15, 1, BLUE);
    tft.fillRoundRect(74, 102, 3, 15, 1, BLUE);
    tft.fillRoundRect(82, 102, 3, 15, 1, BLUE);
}

void drawLightSnowfall()
{
    tft.drawBitmap(0, 30, cloud, 128, 90, GREY);
    tft.fillCircle(50, 100, 3, GREY);
    tft.fillCircle(65, 103, 3, GREY);
    tft.fillCircle(82, 100, 3, GREY);
}

void drawModerateSnowfall()
{
    tft.drawBitmap(0, 35, cloud, 128, 90, GREY);
    tft.fillCircle(50, 105, 3, GREY);
    tft.fillCircle(50, 115, 3, GREY);
    tft.fillCircle(65, 108, 3, GREY);
    tft.fillCircle(65, 118, 3, GREY);
    tft.fillCircle(82, 105, 3, GREY);
    tft.fillCircle(82, 115, 3, GREY);
}

void drawHeavySnowfall()
{
    tft.drawBitmap(0, 35, cloud, 128, 90, GREY);
    tft.fillCircle(40, 105, 3, GREY);
    tft.fillCircle(52, 105, 3, GREY);
    tft.fillCircle(52, 115, 3, GREY);
    tft.fillCircle(65, 108, 3, GREY);
    tft.fillCircle(65, 118, 3, GREY);
    tft.fillCircle(80, 105, 3, GREY);
    tft.fillCircle(80, 115, 3, GREY);
    tft.fillCircle(92, 105, 3, GREY);
}

void drawCloudSunAndRain()
{
    tft.fillCircle(73, 70, 20, YELLOW);
    tft.drawBitmap(0, 32, cloud, 128, 90, BLACK);
    tft.drawBitmap(0, 35, cloud, 128, 90, GREY);
    tft.fillRoundRect(50, 105, 3, 13, 1, BLUE);
    tft.fillRoundRect(65, 105, 3, 13, 1, BLUE);
    tft.fillRoundRect(80, 105, 3, 13, 1, BLUE);
}

void drawCloudAndTheMoon()
{
    tft.fillCircle(94, 60, 18, GREY);
    tft.fillCircle(105, 53, 18, BLACK);
    tft.drawBitmap(0, 32, cloud, 128, 90, BLACK);
    tft.drawBitmap(0, 35, cloud, 128, 90, GREY);
}

void drawCloudTheMoonAndRain()
{
    tft.fillCircle(94, 60, 18, GREY);
    tft.fillCircle(105, 53, 18, BLACK);
    tft.drawBitmap(0, 32, cloud, 128, 90, BLACK);
    tft.drawBitmap(0, 35, cloud, 128, 90, GREY);
    tft.fillRoundRect(50, 105, 3, 11, 1, BLUE);
    tft.fillRoundRect(65, 105, 3, 11, 1, BLUE);
    tft.fillRoundRect(80, 105, 3, 11, 1, BLUE);
}

void drawWind()
{
    tft.drawBitmap(0, 35, wind, 128, 90, GREY);
}

void drawFog()
{
    tft.fillRoundRect(45, 60, 40, 4, 1, GREY);
    tft.fillRoundRect(40, 70, 50, 4, 1, GREY);
    tft.fillRoundRect(35, 80, 60, 4, 1, GREY);
    tft.fillRoundRect(40, 90, 50, 4, 1, GREY);
    tft.fillRoundRect(45, 100, 40, 4, 1, GREY);
}

void clearIcon()
{
    tft.fillRect(0, 40, 128, 100, BLACK);
}