#include <Adafruit_NeoPixel.h>
#include <ArduinoBLE.h>

#define PIN_WS2812B 4
#define NUM_PIXELS 60
#define LED_BUILTIN 2

#define UP 0x00E9
#define DOWN 0x00EA
#define PP 0x00CD

Adafruit_NeoPixel ws2812b(NUM_PIXELS, PIN_WS2812B, NEO_GRB + NEO_KHZ800);
BLECharacteristic hidReportCharacteristic;

void setup()
{
    Serial.begin(9600);
    while (!Serial)
        sleep(10);

    // initialize built-in LED pin
    pinMode(LED_BUILTIN, OUTPUT);

    // initialize WS2812B
    ws2812b.begin();

    // initialize BLE hardware
    Serial.println("Starting BLE work! \n");
    if (!BLE.begin())
    {
        Serial.println("Starting Bluetooth® Low Energy module failed!");
        sleep(1000);
    }

    BLE.setEventHandler(BLEDisconnected, bleDisconnectHandler);
    BLE.setEventHandler(BLEConnected, bleConnectHandler);

    Serial.println("Bluetooth® Low Energy Central - LED control");
}

void loop()
{
    if (!BLE.connected())
    {
        BLE.scanForName("XIAO nRF52840 S");
        BLEDevice peripheral = BLE.available();
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
                checkCharacteristics(peripheral);
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
    else
    {
        Serial.println("Connected to XIAO nRF52840 Sense \n");
        Serial.println("Waiting for key press \n");
        delay(1000);
    }
}

void checkCharacteristics(BLEDevice peripheral)
{
    while (peripheral.connected())
    {
        if (peripheral.discoverAttributes())
        {
            if (peripheral.discoverService("1812"))
            {
                hidReportCharacteristic = peripheral.characteristic("2a4d"); // UUID for HID report characteristic

                if (hidReportCharacteristic)
                {
                    Serial.println("Found HID report characteristic");

                    if (hidReportCharacteristic.subscribe())
                    {
                        Serial.println("Subscribed to HID report characteristic");
                    }
                }
            }
            while (peripheral.connected())
            {
                if (hidReportCharacteristic.valueUpdated())
                {
                    byte report[8];
                    hidReportCharacteristic.readValue(report, sizeof(report));
                    Serial.println(report);
                    controlLeds(report[2]);
                }
            }
            Serial.println(hidReportCharacteristic.read());
        }
        else
        {
            printf("Attribute discovery failed!");
            peripheral.disconnect();
        }

        BLEService HIDservice = peripheral.service("1812");
        Serial.println("Service: ");
        Serial.println(HIDservice.uuid());                    // 1812
        Serial.println(HIDservice.hasCharacteristic("1812")); // 0
        BLECharacteristic HIDcharacteristic = HIDservice.characteristic("1812");

        // bleReadCharacteristics(HIDcharacteristic);
        // controlLeds();
    }
}

/*
const uint8_t *bleReadCharacteristics(BLECharacteristic characteristic)
{
    Serial.print("Characteristic value: ");
    characteristic.read();
    const uint8_t *key = characteristic.value();
    printf("Key: %s \n", key);
    return key;
}
*/

void controlLeds(byte key)
{
    Serial.println(key);
    switch (key)
    {
    case UP:
        printf("UP \n");
        break;
    case DOWN:
        printf("DOWN \n");
        break;
    case PP:
        printf("PP \n");
        break;
    default:
        printf("Unknown key \n");
        break;
    }
}

void bleDisconnectHandler(BLEDevice device)
{
    // central disconnected event handler
    Serial.print("Disconnected event, central: ");
    Serial.println(device.address());
    printf("\n Disconnected from XIAO nRF52840 Sense \n");
    digitalWrite(LED_BUILTIN, LOW);
}

void bleConnectHandler(BLEDevice device)
{
    // central connected event handler
    Serial.print("Connected event, central: ");
    Serial.println(device.address());
    Serial.println("Connected to XIAO nRF52840 Sense \n")
    digitalWrite(LED_BUILTIN, HIGH);
}