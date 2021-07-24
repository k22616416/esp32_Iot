#include <ESP32Servo.h>
#include <Bounce2.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <pthread.h>
//#include <pthread.h>

#define RGBLED_PIN 27
#define NUMPIXELS 2
#define SPEAKER_PIN 32
#define CLK 25 // Pin 9 to clk on encoder
#define DT 26  // Pin 8 to DT on encoder
#define SWITCH_PIN 33

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library
Adafruit_NeoPixel pixels(NUMPIXELS, RGBLED_PIN, NEO_GRB + NEO_KHZ800);
Bounce debouncer = Bounce(); // Instantiate a Bounce object

int tmpAddress = B1001000;
int ResolutionBits = B01100000;
int rotation;
int value;
int RotPosition = 0;
bool LeftRight;
bool switch_status = false;
int color[3] = {0, 0, 0};

// 各功能啟用狀態變數
bool Temperature_detection = false; // 溫度偵測功能

// pthread變數
pthread_t temperatureController; // 宣告 pthread 變數

float temperature = 0;
void setup()
{
  Serial.begin(115200); // start serial for output
  Wire.begin();         // join i2c bus (address optional for master)
  pinMode(CLK, INPUT);
  pinMode(DT, INPUT);
  pinMode(SWITCH_PIN, INPUT);
  pinMode(SPEAKER_PIN, OUTPUT);
  rotation = digitalRead(CLK);
  //Set up the display
  tft.init();
  tft.setRotation(4);
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(35, 5);
  tft.println(F("Starting"));
  delay(2000);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(0, 0);
  SetResolution();
  pixels.begin();

  /*防彈跳程式*/
  debouncer.attach(SWITCH_PIN);
  debouncer.interval(20); // 防彈跳偵測秒數 越大按鈕反應時間越長

  // 各功能之thread
  Temperature_detection = true;
  pthread_create(&temperatureController, NULL, GetTemperature, NULL); // 建立子執行緒
}

void loop()
{
  value = digitalRead(CLK);
  if (value != rotation)
  {
    if (digitalRead(DT) != value)
    {
      RotPosition++;
      if (RotPosition > 0 && RotPosition < 255)
      {
        color[0]++;
        color[1]++;
        color[2]++;
      }
      LeftRight = true;
    }
    else
    {
      LeftRight = false;
      RotPosition--;
      if (RotPosition > 0 && RotPosition < 255)
      {
        color[0]--;
        color[1]--;
        color[2]--;
      }
    }
    if (LeftRight)
    {
      Serial.println("clockwise");
    }
    else
    {
      Serial.println("counterclockwise");
    }
    Serial.print("Encoder RotPosition: ");
    Serial.println(RotPosition);
  }

  // 按鈕
  debouncer.update(); // Update the Bounce instance
  if (debouncer.fell())
  {
    // Call code if Bounce fell (transition from HIGH to LOW)
    Serial.println("Switch click!");
  }

  rotation = value;

  //pixels.clear();
  pixels.setPixelColor(0, pixels.Color(color[0], color[1], color[2]));
  pixels.setPixelColor(1, pixels.Color(color[0], color[1], color[2]));
  pixels.setPixelColor(2, pixels.Color(color[0], color[1], color[2]));
  pixels.show();
  // for (int i = 0; i < NUMPIXELS; i++)
  // {
  //   pixels.setPixelColor(i, pixels.Color(255, 0, 0));
  //   pixels.show();
  //   delay(500);
  //   pixels.setPixelColor(i, pixels.Color(0, 255, 0));
  //   pixels.show();
  //   delay(500);
  //   pixels.setPixelColor(i, pixels.Color(0, 0, 255));
  //   pixels.show();
  // }

  //  delay(350);
  //  tone(SPEAKER_PIN, 1000);
  //  digitalWrite(SPEAKER_PIN, HIGH);
  //  delay(500);
  //  digitalWrite(SPEAKER_PIN, LOW);
  //  noTone(SPEAKER_PIN);
  //  delay(500);
}

void *GetTemperature(void *arge)
{
  while (Temperature_detection)
  {
    Wire.requestFrom(tmpAddress, 2);
    // byte MSB = Wire.read();
    // byte LSB = Wire.read();
    // int TemperatureSum = ((MSB << 8) | LSB) >> 4;
    byte readByte = Wire.read();
    int TemperatureSum = ((readByte << 8) | readByte) >> 4;
    temperature = TemperatureSum * 0.0625;
    Serial.print("Celsius: ");
    Serial.println(temperature);
    delay(2000);
  }
  pthread_exit(NULL); // 離開子執行緒
}

void SetResolution()
{
  //  if (ResolutionBits < 9 || ResolutionBits > 12) exit;
  Wire.beginTransmission(tmpAddress);
  Wire.write(B00000001); //addresses the configuration register
                         //  Wire.write((ResolutionBits-9) << 5); //writes the resolution bits
  Wire.write(ResolutionBits);
  Wire.endTransmission();
  Wire.beginTransmission(tmpAddress); //resets to reading the temperature
  Wire.write((byte)0x00);
  Wire.endTransmission();
}

boolean CheckSwitch()
{
  // if (digitalRead(SWITCH_PIN) == HIGH)
  // {

  //   return false;
  // }
  // else if (digitalRead(SWITCH_PIN) == LOW)
  // {

  //   return true;
  // }

  if (digitalRead(SWITCH_PIN) == LOW && switch_status != true)
  {
    Serial.println("Switch clicked");
    switch_status = true;
    return true;
  }

  if (digitalRead(SWITCH_PIN) == LOW && switch_status == true)
  {
    return false;
  }
  if (digitalRead(SWITCH_PIN) == HIGH && switch_status == true)
  {
    switch_status = false;
    return false;
  }
  return false;
}
