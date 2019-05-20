
#include "BLEDevice.h"
#include <WiFi.h>
#include <PubSubClient.h>
#define wifi_ssid "rehanpebe"
#define wifi_password "mautauaja"
#define token "2i0ptg9HX2c7Wfa52hxa"


int status = WL_IDLE_STATUS;
char thingsboardServer[] = "demo.thingsboard.io";
WiFiClient espClient;
PubSubClient client(espClient);
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914a");
static BLEUUID charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a7");


static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;
String pData;


static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pDataT,
  size_t length,
  bool isNotify) {
  Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.println(length);
  Serial.print("data: ");
  pData = (char*)pDataT;
  Serial.println(pData);
}


class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
      Serial.print ("lagi konek");
    }


    void onDisconnect(BLEClient* pclient) {
      connected = false;
      Serial.println("onDisconnect");
    }
};


bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());


  BLEClient* pClient = BLEDevice::createClient();
  Serial.println(" - Created client");


  pClient->setClientCallbacks(new MyClientCallback());
  pClient->connect(myDevice);
  Serial.println(" - Connected to server");
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    pClient->disconnect();
    return false;
  }
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);


  if (pRemoteCharacteristic == nullptr) {
    pClient->disconnect();
    return false;
  }


  if (pRemoteCharacteristic->canRead()) {
    std::string value = pRemoteCharacteristic->readValue();
    Serial.println(value.c_str());
  }


  if (pRemoteCharacteristic->canNotify())
    pRemoteCharacteristic->registerForNotify(notifyCallback);


  connected = true;
}


class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      Serial.print("BLE Advertised Device found: ");
      Serial.println(advertisedDevice.toString().c_str());
      if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
        BLEDevice::getScan()->stop();
        myDevice = new BLEAdvertisedDevice(advertisedDevice);
        doConnect = true;
        doScan = true;
      }
    }
};




void setup() {
  Serial.begin(115200);
  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
  client.setServer(thingsboardServer, 1883);
}


void reconnect() {
  while (!client.connected()) {
    status = WiFi.status();
    if ( status != WL_CONNECTED) {
      WiFi.begin(wifi_ssid, wifi_password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("Connected to AP");
    }
    Serial.print("Connecting to ThingsBoard node ...");
    if ( client.connect("ESPThingsboard", token, NULL) ) {
      Serial.println( "[DONE]" );
    } else {
      Serial.print( "[FAILED]" );
      Serial.println( client.state() );
      delay(1000);
    }
  }
}


void loop() {
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("YES");
    } else {
      Serial.println("FAIL");
    }
    doConnect = false;
  }
  if (connected) {
    String newValue = "Time since boot: " + String(millis() / 1000);
    Serial.println("Setting new characteristic value to \"" + newValue + "\"");
    pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
  } else if (doScan) {
    BLEDevice::getScan()->start(0);
  }
  delay(5000);
  if ( !client.connected() ) {
    reconnect();
  }
  int datasize = sizeof(pData);
  char attribut[datasize+1];
  pData.toCharArray(attribut, datasize);
  client.publish( "v1/devices/me/telemetry", attribut );
  delay(2000);


  client.loop();
}
