#ifndef PowerServer_H_
#define PowerServer_H_

#include <BLEServer.h>
#include <BLE2902.h>


class PowerServer 
{
public:
    PowerServer();

    void init(BLEServer * pServer);
    void update(short watts, ulong crankCount, ulong msCrankEventTime);

private:
    BLE2902 cpMeasurementDescriptor;
    BLE2902 cpFeatureDescriptor;
    BLE2902 sensorLocationDescriptor;

    BLECharacteristic cpMeasurementCharacteristics;
    BLECharacteristic cpFeatureCharacteristics;
    BLECharacteristic sensorLocationCharacteristics;
};

#endif