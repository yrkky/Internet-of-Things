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
        Serial.println("Connected to XIAO nRF52840 Sense \n");
        Serial.println("Waiting for key press \n");
        delay(1000);

        if (peripheral.localName() == "FingerTracker")
        {
            // Discover the peripheral's services and characteristics
            if (peripheral.discoverAttributes())
            {
                Serial.println("Attributes discovered");
            }
            else
            {
                Serial.println("Attribute discovery failed!");
                return;
            }

            // Find the characteristic you want to read
            // Replace "serviceUuid" and "charUuid" with the UUIDs of the service and characteristic you want to read
            BLECharacteristic keyCharacteristic = peripheral.characteristic("1812", 0x2092);

            if (keyCharacteristic)
            {
                // Read the value of the characteristic
                if (keyCharacteristic.valueUpdated())
                {
                    const uint8_t *data = keyCharacteristic.value();
                    String key(reinterpret_cast<const char *>(data));
                    Serial.print("Received key press from FingerTracker: ");
                    Serial.println(key);
                }
            }
            else
            {
                Serial.print("Peripheral does not have key characteristic! :");
                Serial.print(keyCharacteristic);
                Serial.println();
            }
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
