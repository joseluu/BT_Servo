#include "SSD1306Wire.h"
#include "EEPROM.h"
/*
* Vous allez devoir en équipe répondre à une charade a trouver dans le code
* cherchez les commentaires (tels que celui-ci)
*/
#include <Servo.h>
#include <BluetoothSerial.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

/*
* Placez ci-dessous votre nom d'equipe
* Puis la reponse a la charade lorsque vous l'aurez trouvée
*/
#define BT_NAME "Servo tester"
#define REPONSE "Mystery"


#define USE_SERIAL Serial
static const int servoPin = 13;
Servo servo1;
SSD1306Wire display(0x3c, 4, 15);
char message[100];
BluetoothSerial serialBT;

EEPROMClass  accountStorage("eeprom", 0x500);

class MemoryState {
  public: 
  bool one; 
  char btName[80];
  int lastAngle;
  void initialize()
  { 
     one=true;
      strcpy(btName,BT_NAME);
      lastAngle=90;
  }
};
MemoryState memoryState;

void serialBT_SPP_callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    Serial.println("Client Connected has address:");
    for (int i = 0; i < 6; i++) {
      Serial.printf("%02X", param->srv_open.rem_bda[i]);
      if (i < 5) {
        Serial.print(":");
      }
    }
    Serial.println("");
  } else if (event == ESP_SPP_CLOSE_EVT) {
    Serial.println("Client disconnected");
  }
}

void setupBT(char * name) {
  bool bStatus = serialBT.begin(name); //Bluetooth device name
  if (! bStatus ) {
    Serial.println("Serial BT initialization failure");
  }
  serialBT.register_callback(serialBT_SPP_callback);
  serialBT.onData(serialBT_receive_callback);
}

void setup() {
  Serial.println("reading accountStorage");
if (!accountStorage.begin(accountStorage.length())) {
    Serial.println("Failed to initialise accountStorage");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }
#define FIRST_TIME_MEMORY_INITIALIZATION 1
#if FIRST_TIME_MEMORY_INITIALIZATION
  memoryState.initialize();
  accountStorage.put(0,memoryState);
  accountStorage.commit();
#endif
  accountStorage.get(0,memoryState);
  servo1.attach(servoPin);
  servo1.write(memoryState.lastAngle);
  /*
  * LCD
  */
  pinMode(16,OUTPUT); 
  digitalWrite(16,LOW); 
  delay(50); 
  digitalWrite(16,HIGH); 
  // Initialising the UI will init the display too.
/*
* Mon troisième est au bout des crayons 
*/
  display.init();
  display.clear();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
    if (memoryState.one) {
     display.drawString(0, 0, "Session Digital IOT  G0");
     memoryState.one=false;
  } else {
     display.drawString(0, 0, "Session Digital IOT _G0");
     memoryState.one=true;
  }
  accountStorage.put(0,memoryState);
  accountStorage.commit();
  display.drawString(0, 20, BT_NAME);
  display.drawString(0, 40, "Response: ");
  display.drawString(0, 50, REPONSE);
  display.display();
  
  Serial.begin(115200);
  setupBT(memoryState.btName);
  Serial.println("The device started, now you can pair it with bluetooth!");
}

#define BT_BUFFER_SIZE 200U
static char bufferBT[BT_BUFFER_SIZE];
static size_t lenBT = 0;

void serialBT_receive_callback(const uint8_t *data , size_t len) {
  // doc here: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/bluetooth/esp_spp.html#_CPPv4N18esp_spp_cb_param_t8data_indE
  Serial.print("in Receive callback: ");
  Serial.print(len);
  Serial.println(" bytes available");
  lenBT = len;
  strncpy(bufferBT, (const char *)data, min(len, BT_BUFFER_SIZE - 1));
  bufferBT[len] = 0;
}

char * read_from_BT() {
  if (lenBT == 0) {
    return NULL;
  }
  lenBT = 0;
  return bufferBT;
}

void loop() {
  if (Serial.available()) {
    serialBT.write(Serial.read());
  }
  char * command;
  if ((command = read_from_BT()) != NULL) {
    display.clear();
    display.drawStringMaxWidth(0, 0, 128,command);
    display.display();
    if (strstr(command,"angle:")){
      int angle;
/*
* Que suis je ?
*/
      angle=atoi(&command[6]);
      memoryState.lastAngle=angle;
      accountStorage.put(0,memoryState);
      accountStorage.commit();
      servo1.write(angle);
    }
    delay(1000);
  }
  delay(20);
}
