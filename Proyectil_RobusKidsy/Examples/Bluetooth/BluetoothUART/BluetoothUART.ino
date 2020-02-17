/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
   Has a characteristic of: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E - used for receiving data with "WRITE" 
   Has a characteristic of: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E - used to send data with  "NOTIFY"

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   In this example rxValue is the data received (only accessible inside that function).
   And txValue is the data to be sent, in this example just a byte incremented every second. 
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <RobusKidsy.h>

Robus Kidsy;

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
int txValue;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Kidsy.Neopixel.heartBeat(RED);
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Kidsy.Neopixel.heartBeat(BLUE);
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();

    if (rxValue.length() > 0) {
      Serial.println("*********");
      Serial.print("Received Value: ");
      for (int i = 0; i < rxValue.length(); i++)
        Serial.print(rxValue[i]);

      Serial.println();
      Serial.println("*********");

      if (rxValue == "!B516") Kidsy.Move.frontStraight(150);
      else if (rxValue == "!B507") Kidsy.Move.frontStraight(0);
      if (rxValue == "!B615") Kidsy.Move.backStraight(150);
      else if (rxValue == "!B606") Kidsy.Move.backStraight(0);
      if (rxValue == "!B714") Kidsy.Move.pivotLeft(150);
      else if (rxValue == "!B705") Kidsy.Move.pivotLeft(0);
      if (rxValue == "!B813") Kidsy.Move.pivotRight(150);
      else if (rxValue == "!B804") Kidsy.Move.pivotRight(0);

      if (rxValue == "!B11:") {
        Kidsy.Buzzer.playTone(2000, 50);
        Kidsy.Led1.on();
      }
      else if (rxValue == "!B219") {
        Kidsy.Buzzer.playTone(2500, 50);
        Kidsy.Led1.off();
      }
      else if (rxValue == "!B318") {
        Kidsy.Buzzer.playTone(3000, 50);
        Kidsy.Led2.on();
      }
      else if (rxValue == "!B417") {
        Kidsy.Buzzer.playTone(3500, 50);
        Kidsy.Led2.off();
      }
    }
  }
};


void setup() {
  Serial.begin(115200);
  Kidsy.begin();
  Kidsy.LedW.on();

  // Create the BLE Device
  BLEDevice::init("Robus Kidsy");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
                    CHARACTERISTIC_UUID_TX,
                    BLECharacteristic::PROPERTY_NOTIFY
                  );
                      
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                       CHARACTERISTIC_UUID_RX,
                      BLECharacteristic::PROPERTY_WRITE
                    );

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
    Kidsy.ColorSensor.read();
    Kidsy.Neopixel.color(Kidsy.ColorSensor.color_value);
    Serial.println(Kidsy.ColorSensor.color_string);
    
    if (deviceConnected) {
        pTxCharacteristic->setValue(&Kidsy.ColorSensor.color_value, 1);
        pTxCharacteristic->notify();
    delay(10); // bluetooth stack will go into congestion, if too many packets are sent
  }

    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}
