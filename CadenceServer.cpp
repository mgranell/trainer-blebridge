#include "config.h"
#include "CadenceServer.h"

// Define Speed and Cadence Properties
#define speedService BLEUUID((uint16_t)0x1816)

CadenceServer::CadenceServer() :
    cscMeasurementCharacteristics(BLEUUID((uint16_t)0x2A5B), BLECharacteristic::PROPERTY_NOTIFY),
    cscFeatureCharacteristics(BLEUUID((uint16_t)0x2A5C), BLECharacteristic::PROPERTY_READ),
    sensorLocationCharacteristics(BLEUUID((uint16_t)0x2A5D), BLECharacteristic::PROPERTY_READ),
    scControlPointCharacteristics(BLEUUID((uint16_t)0x2A55), BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_INDICATE)
{
        // https://www.bluetooth.com/xml-viewer/?src=https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Characteristics/org.bluetooth.characteristic.csc_measurement.xml
        cscMeasurementDescriptor.setValue("Exercise Bike CSC Measurement");
        cscMeasurementCharacteristics.addDescriptor(&cscMeasurementDescriptor);

        // https://www.bluetooth.com/xml-viewer/?src=https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Characteristics/org.bluetooth.characteristic.csc_feature.xml
        cscFeatureDescriptor.setValue("Exercise Bike CSC Feature");
        cscFeatureCharacteristics.addDescriptor(&cscFeatureDescriptor);

        // https://www.bluetooth.com/xml-viewer/?src=https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Characteristics/org.bluetooth.characteristic.sensor_location.xml
        sensorLocationDescriptor.setValue("Exercise Bike CSC Sensor Location");
        sensorLocationCharacteristics.addDescriptor(&sensorLocationDescriptor);

        // https://www.bluetooth.com/xml-viewer/?src=https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Characteristics/org.bluetooth.characteristic.sc_control_point.xml
        scControlPointDescriptor.setValue("Exercise Bike CSC SC Control Point");
        scControlPointCharacteristics.addDescriptor(&scControlPointDescriptor);
}

void CadenceServer:: init(BLEServer *pServer)
    {
        // Create Speed and Cadence Configuration
        BLEService *pSpeed = pServer->createService(speedService); // Create Speed and Cadence Service
        pSpeed->addCharacteristic(&cscMeasurementCharacteristics);
        pSpeed->addCharacteristic(&cscFeatureCharacteristics);
        pSpeed->addCharacteristic(&sensorLocationCharacteristics);
        pSpeed->addCharacteristic(&scControlPointCharacteristics);

        byte cscfeature[1] = {0b010}; // Bit 1 - Crank rev present
        cscFeatureCharacteristics.setValue(cscfeature, 1);

        byte sensorlocation[1] = {6}; // Right crank -
        sensorLocationCharacteristics.setValue(sensorlocation, 1);

        scControlPointCharacteristics.setCallbacks(this);

        // Add UUIDs for Services to BLE Service Advertising
        pServer->getAdvertising()->addServiceUUID(speedService);

        // Start p Instances
        pSpeed->start();
    }

    void CadenceServer::update(ulong crankCount, ulong msCrankEventTime)
    {
        ushort lastCrankTimeInTicks = (((ulong)(msCrankEventTime * (1024.0 / 1000))) & 0xFFFF);

        // Least significant byte first
        byte cscmeasurement[5];         // flags (byte), revCount (uint32), eventTime (uint16)
        cscmeasurement[0] = 0b00000010; // Crank Measurement present
        cscmeasurement[1] = crankCount & 0xFF;
        cscmeasurement[2] = (crankCount >> 8) & 0xFF;

        cscmeasurement[3] = lastCrankTimeInTicks & 0xFF;
        cscmeasurement[4] = (lastCrankTimeInTicks >> 8) & 0xFF;

        cscMeasurementCharacteristics.setValue(cscmeasurement, 5);
        cscMeasurementCharacteristics.notify();
    }

#define RESPONSE_CODE_SUCCESS 1
#define RESPONSE_CODE_OPCODE_NOT_SUPPORTED 2
#define RESPONSE_CODE_INVALID_PARAMETER 3
#define RESPONSE_CODE_FAILED 4

    //https://www.bluetooth.com/xml-viewer/?src=https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Characteristics/org.bluetooth.characteristic.sc_control_point.xml
    void CadenceServer::onWrite(BLECharacteristic *pSCCharacteristic)
    {
        std::string data = pSCCharacteristic->getValue();
        int dataLen = data.length();

        byte opCode = data[0];
        byte opCodeResponse = RESPONSE_CODE_FAILED;
        switch (opCode) // Op Code
        {
            /*
        case 1: // Set Cumulative Value
            if (dataLen < 5)
            {
                opCodeResponse = RESPONSE_CODE_INVALID_PARAMETER;
                Serial.println("Invalid wheel count set operations");
            }
            else
            {
                ulong wheelRevCount = data[1] + (data[2] << 8) + (data[3] << 16) + (data[4] << 24);
                // TODO: Send updated wheel count back

                opCodeResponse = RESPONSE_CODE_SUCCESS;
                Serial.print("WheelRevCount set to ");
                Serial.println(wheelRevCount);
            }
            break;*/

        default:
            Serial.print("Received unsupported op code: ");
            Serial.println(opCode);
            opCodeResponse = RESPONSE_CODE_OPCODE_NOT_SUPPORTED;
        }

        byte SCControlPointResponse[3];
        SCControlPointResponse[0] = 0x10;
        SCControlPointResponse[1] = opCode; // Copy request op code into response
        SCControlPointResponse[2] = opCodeResponse;
        pSCCharacteristic->setValue(SCControlPointResponse, 3);
        pSCCharacteristic->indicate();
    }

