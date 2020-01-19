#include <ArduinoJson.h>
#include <GxEPD.h>
#include <GxGDEH029A1/GxGDEH029A1.h>      // 2.9" b/w
#include <Fonts/FreeSans9pt7b.h>
#define DEFAULT_FONT FreeSans9pt7b
#include <Fonts/FreeSansBold9pt7b.h>
#define BOLD_FONT FreeSansBold9pt7b
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

#define ELINK_BUSY 4
#define ELINK_RESET 12
#define ELINK_DC 19
#define ELINK_SS 5

#define SPI_MOSI 23
#define SPI_MISO 2
#define SPI_CLK 18

#define SDCARD_SS 13
#define BUTTON_1 38
#define BUTTON_2 37
#define BUTTON_3 39

GxIO_Class io(SPI, ELINK_SS, ELINK_DC, ELINK_RESET);
GxEPD_Class display(io, ELINK_RESET, ELINK_BUSY);

#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "";
const char* password = "";

HTTPClient client;

struct busTable{
  String line;
  String hour;
  bool realtime;
};

void setup()
{
  WiFi.begin(ssid, password);
  int i = 0;
  while (WiFi.status() != WL_CONNECTED && i < 5) {
    delay(1000);
    i++;
  }
  SPI.begin(SPI_CLK, SPI_MISO, SPI_MOSI, -1);
  display.init();
  display.setRotation(3);
  display.eraseDisplay();
  display.setTextColor(GxEPD_BLACK);
  if(i >= 5){
    display.setFont(&BOLD_FONT);
    display.setCursor(52,67);
    display.print("No Wi-Fi Connection!");
    PowerOff();
  }
  busTable stop_1841[9]; 
  busTable stop_1842[9]; 
  busTable stop_2622[9];
  requestTo("1841",stop_1841);
  requestTo("1842",stop_1842);
  requestTo("2622",stop_2622);
  display.fillScreen(GxEPD_WHITE);
  printBusLine("62","SOFIA",16,stop_1841);
  printBusLine("62","CAIO MARIO",38,stop_1842);
  printBusLine("32","TASSONI",60,stop_2622);
  printBusLine("59","SOLFERINO",82,stop_2622);
  printBusLine("VE1","MASSAUA",104,stop_1842);
  printBusLine("VE1","VENARIA",126,stop_1841);
  PowerOff();
}
void loop(){}

void PowerOff(){
  display.update();
  display.powerDown();
  esp_sleep_enable_timer_wakeup(6e8);
  delay(10);
  esp_deep_sleep_start();
}
void printBusLine(String line, String endstop, int pos, busTable* table){
  display.setFont(&BOLD_FONT);
  display.setCursor(2,pos);
  display.print(endstop);
  display.setCursor(150,pos);
  int bus_n = 0;
  for(int i = 0; i < 9; i++){
    if(table[i].line == line){
      bus_n ++;
      if(table[i].realtime){
        display.setFont(&BOLD_FONT);
      }else{
        display.setFont(&DEFAULT_FONT);
      }
      if(bus_n < 4) display.print(table[i].hour+" ");
    }
  }
  if(bus_n ==0){
    display.setFont(&DEFAULT_FONT);
    display.print("NO SERVIZIO");
  }
}

void requestTo(String stopN, busTable *table){
  client.begin("https://gpa.madbob.org/query.php?stop="+stopN);
  if(client.GET()>0){
    String payload = client.getString();
    DynamicJsonDocument doc(1500);
    deserializeJson(doc, payload);
    uint8_t arraySize = doc.size();
    for(uint8_t i = 0; i<arraySize; i++){
      String line = doc[i]["line"];
      String hour = doc[i]["hour"];
      bool realtime = doc[i]["realtime"]=="true"?1:0;
      table[i].line = line;
      table[i].hour = hour;
      table[i].realtime = realtime;
      /*Serial.print(line);
      Serial.print("\t|\t");
      Serial.print(hour);
      Serial.print("\t|\t");
      Serial.println(realtime);*/
    }
  }
  client.end();
}
