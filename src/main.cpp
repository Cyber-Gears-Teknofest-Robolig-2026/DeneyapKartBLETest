#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define DEVICE_NAME "CyberGears_BLE"

#define SERVICE_UUID "8C17A100-2B31-4F52-9A68-7B126A090001"
#define RX_UUID      "8C17A100-2B31-4F52-9A68-7B126A090002"
#define TX_UUID      "8C17A100-2B31-4F52-9A68-7B126A090003"

BLECharacteristic* txCharacteristic;
bool connected = false;

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* server) override {
    connected = true;
    Serial.println("BLE baglandi");
  }

  void onDisconnect(BLEServer* server) override {
    connected = false;
    Serial.println("BLE baglantisi kesildi");

    BLEDevice::startAdvertising();
  }
};

class RxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* characteristic) override {
    auto data = characteristic->getValue();

    if (data.length() > 0) {
      Serial.write(
        reinterpret_cast<const uint8_t*>(data.data()),
        data.length()
      );
    }
  }
};

void setup() {
  Serial.begin(115200);

  BLEDevice::init(DEVICE_NAME);

  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  BLEService* service = server->createService(SERVICE_UUID);

  txCharacteristic = service->createCharacteristic(
    TX_UUID,
    BLECharacteristic::PROPERTY_NOTIFY
  );

  txCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic* rxCharacteristic =
    service->createCharacteristic(
      RX_UUID,
      BLECharacteristic::PROPERTY_WRITE |
      BLECharacteristic::PROPERTY_WRITE_NR
    );

  rxCharacteristic->setCallbacks(new RxCallbacks());

  service->start();

  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->setScanResponse(true);
  advertising->start();

  Serial.println("BLE hazir");
}

void loop() {
  if (!connected || Serial.available() == 0) {
    delay(1);
    return;
  }

  uint8_t buffer[20];
  size_t length = 0;

  while (Serial.available() > 0 && length < sizeof(buffer)) {
    buffer[length++] = Serial.read();
  }

  txCharacteristic->setValue(buffer, length);
  txCharacteristic->notify();

  delay(5);
}