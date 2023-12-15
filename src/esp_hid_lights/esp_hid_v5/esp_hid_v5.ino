#include <ArduinoBLE.h>
#include <FastLED.h>

#define LEDSTRIP_PIN 4
#define LEDSTRIP_LEDS 60
#define LED_PIN_BUILTIN 2

CRGB leds[LEDSTRIP_LEDS];
int trackedLedIndex = 0;
bool isStripOn = true;
BLEService ledService("19B10000-E8F2-537E-4F6C-D104768A1214");                                       // create service
BLEByteCharacteristic ledCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite); // create led characteristic and allow remote device to read and write

void setup()
{
    Serial.begin(9600);
    while (!Serial)
        ;

    pinMode(LED_PIN_BUILTIN, OUTPUT);
    FastLED.addLeds<WS2812B, LEDSTRIP_PIN, GRB>(leds, LEDSTRIP_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.show(); // Initialize all LEDs to 'off'

    // begin initialization
    if (!BLE.begin())
    {
        Serial.println("starting Bluetooth® Low Energy module failed!");

        while (1)
            ;
    }

    // set the local name peripheral advertises
    BLE.setLocalName("LEDStrip");
    // set the UUID for the service this peripheral advertises:
    BLE.setAdvertisedService(ledService);

    // add the characteristics to the service
    ledService.addCharacteristic(ledCharacteristic);

    // add the service
    BLE.addService(ledService);

    BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
    BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

    ledCharacteristic.setEventHandler(BLEWritten, ledCharacteristicWritten);

    ledCharacteristic.setValue(0); // initial value for the characteristic

    // start advertising
    BLE.advertise();

    Serial.println("Bluetooth® device active, waiting for connections...");
}

void turnOnNext5()
{
    for (int i = trackedLedIndex; i < trackedLedIndex + 5; ++i)
    {
        if (i < LEDSTRIP_LEDS)
        {
            leds[i] = 0xFF9900;
        }
    }
    FastLED.show();
    trackedLedIndex += 5;
}

void turnOffNext5()
{
    for (int i = trackedLedIndex; i < trackedLedIndex + 5; ++i)
    {
        if (i < LEDSTRIP_LEDS)
        {
            leds[i] = 0x000000;
        }
    }
    FastLED.show();
    trackedLedIndex -= 5;
}

void toggleStrip()
{
    if (isStripOn)
    {
        for (int i = 0; i < trackedLedIndex; ++i)
        {
            if (i < LEDSTRIP_LEDS)
            {
                leds[i] = 0xFF9900;
            }
        }
    }
    else
    {
        for (int i = 0; i < trackedLedIndex; ++i)
        {
            if (i < LEDSTRIP_LEDS)
            {
                leds[i] = 0x000000;
            }
        }
    }
    FastLED.show();
    isStripOn = !isStripOn;
}

void executeCommand(uint16_t command)
{
    switch (command)
    {
    case 0x00E9: // Turn on the next 5 LEDs
        turnOnNext5();
        break;
    case 0x00EA: // Turn off the next 5 LEDs
        turnOffNext5();
        break;
    case 0x00CD: // Toggle the whole strip off and on
        toggleStrip();
        break;
    default:
        break;
    }
}

void loop()
{
    // poll for Bluetooth® Low Energy events
    BLE.poll();
}

void blePeripheralConnectHandler(BLEDevice central)
{
    // central connected event handler
    Serial.print("Connected event, central: ");
    Serial.println(central.address());
}

void blePeripheralDisconnectHandler(BLEDevice central)
{
    // central disconnected event handler
    Serial.print("Disconnected event, central: ");
    Serial.println(central.address());
}

void ledCharacteristicWritten(BLEDevice central, BLECharacteristic characteristic)
{
    // central wrote new value to characteristic, update LED
    Serial.print("Characteristic event, written: ");

    const uint8_t *valuedata = characteristic.value();
    Serial.print("    Value: ");
    for (int k = 0; k < characteristic.valueLength(); k++)
    {
        Serial.print(valuedata[k]);
    }

    if (ledCharacteristic.value())
    {
        Serial.println("LED on");
        digitalWrite(LED_PIN_BUILTIN, HIGH);
    }
    else
    {
        Serial.println("LED off");
        digitalWrite(LED_PIN_BUILTIN, LOW);
    }
}