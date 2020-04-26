#include <Arduino.h>
#include "config.h"
#include "PowerServer.h"
#include <string.h>

// Define Speed and Power Properties
// https://www.bluetooth.com/xml-viewer/?src=https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Services/org.bluetooth.service.cycling_power.xml
#define powerService BLEUUID((uint16_t)0x1818)

PowerServer::PowerServer() : cpMeasurementCharacteristics(BLEUUID((uint16_t)0x2A63), BLECharacteristic::PROPERTY_NOTIFY),
                             cpFeatureCharacteristics(BLEUUID((uint16_t)0x2A65), BLECharacteristic::PROPERTY_READ),
                             sensorLocationCharacteristics(BLEUUID((uint16_t)0x2A5D), BLECharacteristic::PROPERTY_READ)
{
    // https://www.bluetooth.com/xml-viewer/?src=https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Characteristics/org.bluetooth.characteristic.cp_measurement.xml
    cpMeasurementDescriptor.setValue("Exercise Bike Cycling Power Measurement");
    cpMeasurementCharacteristics.addDescriptor(&cpMeasurementDescriptor);

    // https://www.bluetooth.com/xml-viewer/?src=https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Characteristics/org.bluetooth.characteristic.cycling_power_feature.xml
    cpFeatureDescriptor.setValue("Exercise Bike Cycling Power Feature");
    cpFeatureCharacteristics.addDescriptor(&cpFeatureDescriptor);

    // https://www.bluetooth.com/xml-viewer/?src=https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Characteristics/org.bluetooth.characteristic.sensor_location.xml
    sensorLocationDescriptor.setValue("Exercise Bike Sensor Location");
    sensorLocationCharacteristics.addDescriptor(&sensorLocationDescriptor);
}

void PowerServer::init(BLEServer *pServer)
{
    // Create Speed and Power Configuration
    BLEService *pPower = pServer->createService(powerService); // Create Speed and Power Service
    pPower->addCharacteristic(&cpMeasurementCharacteristics);
    pPower->addCharacteristic(&cpFeatureCharacteristics);
    pPower->addCharacteristic(&sensorLocationCharacteristics);

    uint32_t cpfeature = (1 << 3); // Bit index 3 - Crank rev present
    cpFeatureCharacteristics.setValue(cpfeature);

    uint8_t sensorlocation = 6; // Right crank -
    sensorLocationCharacteristics.setValue(&sensorlocation, 1);

    // Add UUIDs for Services to BLE Service Advertising
    pServer->getAdvertising()->addServiceUUID(powerService);

    // Start service
    pPower->start();
}

void PowerServer::update(short watts, ulong crankCount, ulong msCrankEventTime)
{
// Least significant byte first
    std::string measure; // flags (byte), revCount (uint32), eventTime (uint16)
    measure.reserve(8);

    // 16bit - Flags
    uint16_t flags = (1 << 5); // Crank Measurement present
    measure += (byte)flags;
    measure += (byte)(flags >> 8) & 0xFF;

    // sint16 - Instantaneous Power
    measure += (byte)watts & 0xFF;
    measure += (byte)(watts >> 8) & 0xFF;

    // uint16 - Crank Revolution Data- Cumulative Crank Revolutions
    measure += (byte)crankCount & 0xFF;
    measure += (byte)(crankCount >> 8) & 0xFF;

    // uint16 - Crank Revolution Data- Last Crank Event Time
    uint16_t lastCrankTimeInTicks = (((ulong)(msCrankEventTime * (1024.0 / 1000))) & 0xFFFF);
    measure += (byte)lastCrankTimeInTicks & 0xFF;
    measure += (byte)(lastCrankTimeInTicks >> 8) & 0xFF;

    cpMeasurementCharacteristics.setValue(measure);
    cpMeasurementCharacteristics.notify();
}
