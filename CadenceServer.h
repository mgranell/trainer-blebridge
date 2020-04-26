#ifndef CadenceServer_H_
#define CadenceServer_H_

#include <BLEServer.h>
#include <BLE2902.h>


class CadenceServer  : BLECharacteristicCallbacks
{
public:
    CadenceServer();

    void init(BLEServer * pServer);
    void update(ulong crankCount, ulong msCrankEventTime);

protected:
    virtual void onWrite(BLECharacteristic* pSCCharacteristic);

private:
    BLE2902 cscMeasurementDescriptor;
    BLE2902 cscFeatureDescriptor;
    BLE2902 sensorLocationDescriptor;
    BLE2902 scControlPointDescriptor;

    BLECharacteristic cscMeasurementCharacteristics;
    BLECharacteristic cscFeatureCharacteristics;
    BLECharacteristic sensorLocationCharacteristics;
    BLECharacteristic scControlPointCharacteristics;
};

#endif