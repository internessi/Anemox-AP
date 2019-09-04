#include <SHT1x.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
#include <SSD1306.h>
#include <FS.h>
#include <SPIFFS.h>
#include <Wire.h> 
#include <RtcDS1307.h>


#define dataPin  27
#define clockPin 26
#define ONE_WIRE_BUS 25
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  50        /* Time ESP32 will go to sleep (in seconds) */
#define FORMAT_SPIFFS_IF_FAILED true
#define countof(a) (sizeof(a) / sizeof(a[0]))

RTC_DATA_ATTR int bootCount = 0;

int SHT_C, SHT_H, DS18_C, SHT_C2, SHT_H2, DS18_C2;
String output [7];
 
 
SSD1306  display(0x3c, 21, 22);
SHT1x sht1x(dataPin, clockPin);
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);
RtcDS1307<TwoWire> Rtc(Wire);
 
void setup() {
  Serial.begin(115200);
  Serial.println("Starting up");
  
  display.init();
  display.clear();
  display.display();
  display.displayOff(); 

  
  sensors.begin(); 

  if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
      Serial.println("SPIFFS Mount Failed");
      return;
  }

RTC_init();
  

  display.displayOn(); 
  display.init();
  display.setFont(ArialMT_Plain_24);
  display.setColor(WHITE);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(12, 5, "ANEMOX");
  display.display();

}


void RTC_init() {
    Serial.print("compiled: ");
    Serial.print(__DATE__);
    Serial.println(__TIME__);
    Rtc.Begin();
    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    compiled = compiled + 30;
    Serial.println(compiled);
    printDateTime(compiled);
    Serial.println();

    if (!Rtc.IsDateTimeValid()) 
    {
        if (Rtc.LastError() != 0)
        {
            Serial.print("RTC communications error = ");
            Serial.println(Rtc.LastError());
        }
        else
        {
            Serial.println("RTC lost confidence in the DateTime!");
            Rtc.SetDateTime(compiled);
        }
    }
    if (!Rtc.GetIsRunning())
    {
        Serial.println("RTC was not actively running, starting now");
        Rtc.SetIsRunning(true);
    }
    RtcDateTime now = Rtc.GetDateTime();
    if (now < compiled) 
    {
        Serial.println("RTC is older than compile time!  (Updating DateTime)");
        Rtc.SetDateTime(compiled);
    }
    else if (now > compiled) 
    {
        Serial.println("RTC is newer than compile time. (this is expected)");
    }
    else if (now == compiled) 
    {
        Serial.println("RTC is the same as compile time! (not expected but all is fine)");
    }
    Rtc.SetSquareWavePin(DS1307SquareWaveOut_Low);    
  }


 
void loop() {


  float temp_c;
  float temp_DS18;
  float humidity;

  // Read values from the sensor
  temp_c = sht1x.readTemperatureC();
  temp_c = temp_c  + 0.05;  
  SHT_C = int(temp_c*10);  
  SHT_C2 = SHT_C-(int(SHT_C/10)*10);
  SHT_C = SHT_C/10; 
  
  humidity = sht1x.readHumidity();  
  humidity = humidity  + 0.05;  
  SHT_H = int(humidity*10);  
  SHT_H2 = SHT_H-(int(SHT_H/10)*10);
  SHT_H = SHT_H/10; 

  Serial.print("SHT: ");
  Serial.print(temp_c, DEC);
  Serial.print("/ ");
  Serial.println(humidity);

    if (!Rtc.IsDateTimeValid()) 
    {
        if (Rtc.LastError() != 0)
        {
            // we have a communications error
            // see https://www.arduino.cc/en/Reference/WireEndTransmission for 
            // what the number means
            Serial.print("RTC communications error = ");
            Serial.println(Rtc.LastError());
        }
        else
        {
            // Common Causes:
            //    1) the battery on the device is low or even missing and the power line was disconnected
            Serial.println("RTC lost confidence in the DateTime!");
        }
    }

    RtcDateTime now = Rtc.GetDateTime();

    printDateTime(now);
    Serial.println();

    delay(15000);
    
}

void printDateTime(const RtcDateTime& dt)
{
    char datestring[20];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
            dt.Month(),
            dt.Day(),
            dt.Year(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    Serial.print(datestring);
}
