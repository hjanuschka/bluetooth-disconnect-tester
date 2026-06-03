#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

namespace {

constexpr char kDeviceName[] = "dino c(h)ancler";
constexpr char kServiceUuid[] = "9c9f0b6a-8f7d-4d0d-9d30-405029430001";
constexpr char kNotifyCharacteristicUuid[] =
    "9c9f0b6a-8f7d-4d0d-9d30-405029430002";

BLEServer* server = nullptr;
BLEAdvertising* advertising = nullptr;
BLECharacteristic* notify_characteristic = nullptr;
uint16_t connection_id = 0;
bool connected = false;
uint32_t last_notify_ms = 0;

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* server, esp_ble_gatts_cb_param_t* param) override {
    connected = true;
    connection_id = param->connect.conn_id;
    Serial.printf("connected, conn_id=%u\n", connection_id);
  }

  void onDisconnect(BLEServer* server) override {
    connected = false;
    connection_id = 0;
    Serial.println("disconnected");
    advertising->start();
    Serial.println("advertising restarted");
  }
};

void startAdvertising() {
  advertising->start();
  Serial.println("advertising started");
}

void stopAdvertising() {
  advertising->stop();
  Serial.println("advertising stopped");
}

void disconnectCentral() {
  if (!connected) {
    Serial.println("no central connected");
    return;
  }
  server->disconnect(connection_id);
  Serial.println("disconnect requested");
}

void enterDeepSleep() {
  stopAdvertising();
  if (connected) {
    disconnectCentral();
    delay(250);
  }
  Serial.println("entering deep sleep; press reset to wake");
  Serial.flush();
  esp_deep_sleep_start();
}

void printHelp() {
  Serial.println();
  Serial.println("commands:");
  Serial.println("  a - start advertising");
  Serial.println("  s - stop advertising");
  Serial.println("  d - disconnect central");
  Serial.println("  z - stop advertising, disconnect, deep sleep");
  Serial.println("  r - restart");
  Serial.println();
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(1000);
  printHelp();

  BLEDevice::init(kDeviceName);
  server = BLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  BLEService* service = server->createService(kServiceUuid);
  notify_characteristic = service->createCharacteristic(
      kNotifyCharacteristicUuid,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  notify_characteristic->addDescriptor(new BLE2902());
  notify_characteristic->setValue("ready");
  service->start();

  advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(kServiceUuid);
  advertising->setScanResponse(true);
  advertising->setMinPreferred(0x06);
  advertising->setMaxPreferred(0x12);
  startAdvertising();
}

void loop() {
  while (Serial.available()) {
    switch (Serial.read()) {
      case 'a':
        startAdvertising();
        break;
      case 's':
        stopAdvertising();
        break;
      case 'd':
        disconnectCentral();
        break;
      case 'z':
        enterDeepSleep();
        break;
      case 'r':
        ESP.restart();
        break;
      case '\r':
      case '\n':
        break;
      default:
        printHelp();
        break;
    }
  }

  if (connected && millis() - last_notify_ms > 1000) {
    last_notify_ms = millis();
    String value = String("tick ") + String(last_notify_ms);
    notify_characteristic->setValue(value.c_str());
    notify_characteristic->notify();
  }
}
