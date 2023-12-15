#include <Arduino.h>
#include <ArduinoBLE.h>

BLEDevice peripheral;

void bleConnectHandler(BLEDevice central)
{
    Serial.print("Connected event, central: ");
    Serial.println(central.address());
}

void bleDisconnectHandler(BLEDevice central)
{
    Serial.print("Disconnected event, central: ");
    Serial.println(central.address());
}

void setup()
{
    Serial.begin(9600);
    while (!Serial)
        ;

    Serial.println("Starting BLE Client application! \n");
    if (!BLE.begin())
    {
        Serial.println("Failed to initialize BLE");
        while (1)
            ;
    }

    BLE.setEventHandler(BLEDisconnected, bleDisconnectHandler);
    BLE.setEventHandler(BLEConnected, bleConnectHandler);
}

void loop()
{
    if (!BLE.connected())
    {
        BLE.scanForName("FingerTracker");
        peripheral = BLE.available();
        if (peripheral)
        {
            Serial.print("Found ");
            Serial.print(peripheral.address());
            Serial.print(" '");
            Serial.print(peripheral.localName());
            Serial.print("' ");
            Serial.print(peripheral.advertisedServiceUuid());

            Serial.println();

            BLE.stopScan();
            if (peripheral.connect())
            {
                Serial.println("Connected");
                digitalWrite(LED_BUILTIN, HIGH);
            }
            else
            {
                Serial.println("Failed to connect!");
            }
        }
        else
        {
            Serial.println("No peripheral found!");
            digitalWrite(LED_BUILTIN, LOW);
            delay(1000);
        }
    }
    else if (BLE.connected())
    {
        // Serial.println("Connected to XIAO nRF52840 Sense \n");
        Serial.println("Waiting for key press \n");
        delay(10);

        if (peripheral.localName() == "FingerTracker")
        {
            // Discover the peripheral's services and characteristics
            if (peripheral.discoverAttributes())
            {
                // Serial.println("Attributes discovered");
            }
            else
            {
                Serial.println("Attribute discovery failed!");
                return;
            }

            for (int i = 0; i < peripheral.serviceCount(); i++)
            {
                BLEService service = peripheral.service(i);
                Serial.print("Service ");
                Serial.print(i);
                Serial.print(": UUID = ");
                Serial.println(service.uuid());
            }

            // // print out the UUIDs and properties of each discovered characteristic:
            // Serial.println("Peripheral characteristics:");
            // BLECharacteristic foundCharacteristic;
            // const char *foundCharacteristicUUID;
            // BLEDescriptor foundDescriptor;
            // const char *foundDescriptorUUID;

            // for (int i = 0; i < peripheral.characteristicCount(); i++)
            // {
            //     foundCharacteristic = peripheral.characteristic(i);
            //     foundCharacteristicUUID = foundCharacteristic.uuid();
            //     Serial.print("  Characteristic ");
            //     Serial.print(i);
            //     Serial.print(": UUID = ");
            //     Serial.print(foundCharacteristicUUID);
            //     Serial.print(", properties = 0x");
            //     Serial.println(foundCharacteristic.properties(), HEX);

            //     if (foundCharacteristic.canRead())
            //     {
            //         foundCharacteristic.read();
            //         const uint8_t *valuedata = foundCharacteristic.value();
            //         Serial.print("    Value: ");
            //         for (int k = 0; k < foundCharacteristic.valueLength(); k++)
            //         {
            //             Serial.print(valuedata[k]);
            //         }
            //         Serial.println();
            //     }

            //     for (int j = 0; j < foundCharacteristic.descriptorCount(); j++)
            //     {
            //         foundDescriptor = foundCharacteristic.descriptor(j);
            //         foundDescriptorUUID = foundDescriptor.uuid();
            //         Serial.print("    Descriptor ");
            //         Serial.print(j);
            //         Serial.print(": UUID = ");
            //         Serial.println(foundDescriptorUUID);
            //     }
            // }

            // Find the characteristic you want to read
            // Replace "serviceUuid" and "charUuid" with the UUIDs of the service and characteristic you want to read
            // BLECharacteristic keyCharacteristic = peripheral.characteristic(12);

            // if (keyCharacteristic)
            // {
            //     Serial.print("Found key characteristic: ");
            //     Serial.println(keyCharacteristic.uuid());
            // }
            // else
            // {
            //     Serial.println("No key characteristic found!");
            // }

            // if (keyCharacteristic.canSubscribe())
            // {
            //     Serial.println("Can Subscribe");
            //     keyCharacteristic.subscribe();
            // }
            // if (keyCharacteristic.canRead())
            // {
            //     Serial.println("Can Read");
            //     keyCharacteristic.read();
            //     Serial.println("Read");
            // }
            // uint16_t value16 = 0x2A05;
            // int read_value = keyCharacteristic.readValue(value16);
            // Serial.println(read_value);

            // if (keyCharacteristic)
            // {
            //     const uint8_t *data = keyCharacteristic.value();
            //     String key(reinterpret_cast<const char *>(data));
            //     Serial.print("KeyCharacteristic value: ");
            //     Serial.println(key);

            //     // Read the value of the characteristic
            //     if (keyCharacteristic.valueUpdated())
            //     {
            //         const uint8_t *data = keyCharacteristic.value();
            //         String key(reinterpret_cast<const char *>(data));
            //         Serial.print("Received key press from FingerTracker: ");
            //         Serial.println(key);
            //     }
            //     else
            //     {
            //         Serial.println("Key press not received!");
            //     }
            // }
            // else
            // {
            //     Serial.print("Peripheral does not have key characteristic! :");
            //     Serial.print(keyCharacteristic);
            //     Serial.println();
            // }
        }
        else
        {
            Serial.println("Peripheral is not FingerTracker!");
            Serial.println("It is: ");
            Serial.println(peripheral.localName());
        }
    }
    else
    {
        Serial.println("Peripheral disconnected!");
        digitalWrite(LED_BUILTIN, LOW);
        delay(1000);
    }
}

void readKeyPresses(BLECharacteristic keyCharacteristic)
{
    while (keyCharacteristic.canRead())
    {
    }
}