#include "config.h"
#include "CadenceServer.h"
#include "PowerServer.h"
#include "NetworkService.h"
#include "BikeBridge.h"

bool _BLEClientConnected = false;

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    _BLEClientConnected = true;
  };

  void onDisconnect(BLEServer *pServer)
  {
    _BLEClientConnected = false;
  }
};

//CadenceServer cadenceServer;
PowerServer powerServer;
NetworkService networkService;
BikeBridge bike;

void initBLE()
{
  // Create BLE Device
  BLEDevice::init("BodyWorx ABX450AT");

  // Create BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  //cadenceServer.init(pServer);
  powerServer.init(pServer);

  // Start Advertising
  pServer->getAdvertising()->start();
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Start");

  networkService.setup();
  bike.setup(&networkService);

  initBLE();
}

ulong lastCrankEventTime = 0;
short lastPower = 0;

void loop()
{
  networkService.loop();
  bike.loop();

  if ( lastCrankEventTime != bike.getLastCrankEventTime()
    || lastPower != bike.getPower() ) 
  {
      lastCrankEventTime = bike.getLastCrankEventTime();
      lastPower = bike.getPower();
      int crankCount = bike.getCrankCount();

      //cadenceServer.Update(wheelRevCount, lastRevTime);
      powerServer.update(lastPower, crankCount, lastCrankEventTime);

      networkService.publish("cycle/bleupdate", "%d,%d,%d", lastPower, crankCount, lastCrankEventTime);
  }
}
