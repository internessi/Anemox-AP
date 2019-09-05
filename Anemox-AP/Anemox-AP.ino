#include <FS.h>                       /* File System */
#include <SPIFFS.h>                   /* SPIFFS */
#include <Wire.h>                     /* Wire für Echtzeituhr und OLED Display */
#include <RtcDS1307.h>                /* Echtzeituhr */
#include <SSD1306.h>                  /* OLED Display */
#include <SHT1x.h>                    /* SHT11 Feuchte und Temperatur */
#include <OneWire.h>                  /* Wire für DS18B20 Temperatursensor */
#include <DallasTemperature.h>        /* DS18B20 Temperatursensor */


#define uS_TO_S_FACTOR 1000000        /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  50             /* Time ESP32 will go to sleep (in seconds) */
#define FORMAT_SPIFFS_IF_FAILED true
#define countof(a) (sizeof(a) / sizeof(a[0]))


RTC_DATA_ATTR int bootCount = 0;
int SHT_C, SHT_H, DS18_C, SHT_C2, SHT_H2, DS18_C2;
float temp_c, temp_DS18, humidity;
unsigned int  progress, total;
String out, in, output [7];

SSD1306  display(0x3c, 21, 22);       /* OLED Display */
SHT1x sht1x(27, 26);                  /* SHT11 Feuchte und Temperatur */
OneWire oneWire(25);                  /* Wire für DS18B20 Temperatursensor */
DallasTemperature sensors(&oneWire);  /* DS18B20 Temperatursensor */
RtcDS1307<TwoWire> Rtc(Wire);         /* Echtzeituhr */


void setup() {
  pinMode       (12, OUTPUT);       // LED1
  pinMode       (14, OUTPUT);       // LED2
  pinMode       (18, OUTPUT);       // RELAIS1
  pinMode       (19, OUTPUT);       // RELAIS2
  digitalWrite  (12, LOW);          // LED1
  digitalWrite  (14, LOW);          // LED2
  digitalWrite  (19, LOW);          // RELAIS2
  digitalWrite  (18, HIGH);          // RELAIS1
  delay(100);
  digitalWrite  (18, LOW);           // RELAIS1
  
  progress = 0;
  total = 100;
  Serial.begin(115200);   
  display_aus();
  delay(1000);
  display_text("Selbstest", "ANEMOX", 1);
  
  delay(1000);  
  progress_bar(20);
  
  sensors.begin();
  progress_bar(40);
  
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  progress_bar(60);
  
  RTC_init();
  progress_bar(80);
  
  SHT11_read();
  progress_bar(100);
  delay(500);
  SHT11_read();
  
  out = "Temp: ";
  out = out + String(SHT_C);
  display_text(out, "LUFT", 100);
  delay(1500);
  
  out = "Feuchte: ";
  out = out + String(SHT_H);
  display_text(out, "LUFT", 100);
  delay(1500);
  
  DS18B20_read();
  delay(500);
  DS18B20_read();

  out = "Temp: ";
  out = out + String(DS18_C);
  display_text(out, "WAND", 100);
  delay(1500);

  display_text(" ", "ANEMOX", 100);
  delay(1500);
}

// relais_aus();
// relais_an();
//LED(1, 500);

void anemox() {

  DS18B20_read();
  SHT11_read();  
  float r, T, TK, TD, DD, SDD, v;
  r = humidity; // SHT11 Luftfeuchtigkeit
  T = temp_c;   // SHT11 Temperatur
  
  // r = relative Luftfeuchte
  // T = Temperatur in °C
  // TK = Temperatur in Kelvin (TK = T + 273.15)
  // TD = Taupunkttemperatur in °C
  // DD = Dampfdruck in hPa
  // SDD = Sättigungsdampfdruck in hPa
  // a = 7.5, b = 237.3 für T >= 0

  // SDD(T) = 6.1078 * 10^((a*T)/(b+T))
  SDD = 6.1078 * pow(10,((7.5*SHT_C)/(237.3+SHT_C)));
  // DD(r,T) = r/100 * SDD(T)
  DD = r/100 * SDD;
  // v(r,T) = log10(DD(r,T)/6.1078)
  v = log10(DD)/6.1078; 
  // TD(r,T) = b*v/(a-v) 
  TD = 237.3*v / (7.5-v);

  



  out = String(SHT_C);
  Serial.println(out);
  
  TD = taupunkt(humidity,temp_c);
  Serial.println(String(TD));
}

float taupunkt(float humi, float temp) {
  float k;
  k = log(humi/100) + (17.62 * temp) / (243.12 + temp);
  return 243.12 * k / (17.62 - k);
}

void loop() {
  anemox();
  SHT11_read();
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


// START  SHT11_read()
void SHT11_read() {
  temp_c = sht1x.readTemperatureC();
  temp_c = temp_c  + 0.05;
  SHT_C = int(temp_c * 10);
  SHT_C2 = SHT_C - (int(SHT_C / 10) * 10);
  SHT_C = SHT_C / 10;
  humidity = sht1x.readHumidity();
  humidity = humidity  + 0.05;
  SHT_H = int(humidity * 10);
  SHT_H2 = SHT_H - (int(SHT_H / 10) * 10);
  SHT_H = SHT_H / 10;
} // END  SHT11_read()


// START  DS18B20_read()
void DS18B20_read() {
  sensors.requestTemperatures(); 
  temp_DS18 = sensors.getTempCByIndex(0);
  temp_DS18 = temp_DS18  + 0.05;  
  DS18_C = int(temp_DS18*10);  
  DS18_C2 = DS18_C-(int(DS18_C/10)*10);
  DS18_C = DS18_C/10;
} // END  DS18B20_read() 

void progress_bar(int barprogress){
  display.drawProgressBar(4, 29, 120, 8, barprogress);
  display.display();
  delay(250);
}

// START  display_text()
void display_text(String text, String text1, int barprogress) {
  display.init();
  display.setContrast(255);
  display.clear();
  display.setFont(ArialMT_Plain_24);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(display.getWidth() / 2, 1, text1);
  display.drawProgressBar(4, 29, 120, 8, barprogress);  
  display.setFont(ArialMT_Plain_24);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(display.getWidth() / 2, 38, text);
  display.display();
} // END  display_text()


// START  display_top()
void display_top() {
  display.setFont(ArialMT_Plain_24);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(display.getWidth() / 2, 1, "ANEMOX");
} // END  display_top()


void display_aus() {
  display.init();
  display.clear();
  display.display();
  display.displayOff();
}

int LED(int LED, double zeit){
  if (LED == 1){
  digitalWrite  (12, HIGH);          // LED1
    }
  if (LED == 2){
  digitalWrite  (14, HIGH);          // LED2
    }
  delay(zeit+10); 
  digitalWrite  (12, LOW);           // LED1
  digitalWrite  (14, LOW);           // LED2
}// END   LED(int LED, double zeit) 

void relais_aus() {
  digitalWrite  (18, HIGH);          // RELAIS1
  delay(100);
  digitalWrite  (18, LOW);           // RELAIS1
}

void relais_an() {
  digitalWrite  (19, HIGH);          // RELAIS2
  delay(100);
  digitalWrite  (19, LOW);           // RELAIS2
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
