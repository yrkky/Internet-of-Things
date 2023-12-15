#include <ArduinoBLE.h>
#include <FastLED.h>

#define LEDSTRIP_PIN 4
#define LEDSTRIP_LEDS 50
#define LED_PIN_BUILTIN 2
#define BRIGHTNESS 64

CRGB leds[LEDSTRIP_LEDS];
int trackedLedIndex = 0;
bool isStripOn = true;
bool isConnected = false;
BLEService ledService("19B10000-E8F2-537E-4F6C-D104768A1214");                                       // create service
BLEByteCharacteristic ledCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite); // create led characteristic and allow remote device to read and write

void setup()
{
    delay(3000);
    Serial.begin(9600);
    while (!Serial)
        ;

    pinMode(LED_PIN_BUILTIN, OUTPUT);
    pinMode(LEDSTRIP_PIN, OUTPUT);
    FastLED.addLeds<WS2812B, LEDSTRIP_PIN, GRB>(leds, LEDSTRIP_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.show(); // Initialize all LEDs to 'off'
    FastLED.setBrightness(BRIGHTNESS);

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

void toggleBuiltInLed()
{
    if (isStripOn)
    {
        digitalWrite(LED_PIN_BUILTIN, LOW);
    }
    else
    {
        digitalWrite(LED_PIN_BUILTIN, HIGH);
    }
}

void turnOnNext5()
{
    FastLED.clear();
    for (int i = 0; i < LEDSTRIP_LEDS; ++i)
    {
        if (i <= trackedLedIndex + 5)
        {
            leds[i] = 0xFF9900;
        }
    }
    FastLED.show();
    trackedLedIndex += 5;
    if (trackedLedIndex > LEDSTRIP_LEDS)
    {
        trackedLedIndex = 0;
    }
    Serial.println("Current Led Index: " + String(trackedLedIndex));
}

void turnOffNext5()
{
    FastLED.clear();
    for (int i = 0; i < LEDSTRIP_LEDS; ++i)
    {
        if (i <= trackedLedIndex - 5)
        {
            leds[i] = 0xFF9900;
        }
    }
    FastLED.show();
    trackedLedIndex -= 5;
    // if the trackedLedIndex is less than the 0, reset it to NUM_LEDS
    if (trackedLedIndex < 0)
    {
        trackedLedIndex = LEDSTRIP_LEDS;
    }
    Serial.println("Current Led Index: " + String(trackedLedIndex));
}

void toggleStrip()
{
    FastLED.clear();
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
    toggleBuiltInLed();
    isStripOn = !isStripOn;
    Serial.println("Current Led Index: " + String(trackedLedIndex));
}

void executeCommand(uint8_t command)
{
    switch (command)
    {
    case 0x00E9: // Turn on the next 5 LEDs
        turnOnNext5();
        Serial.println("Execute Increase");
        break;
    case 0x00EA: // Turn off the next 5 LEDs
        turnOffNext5();
        Serial.println("Execute Decrease");
        break;
    case 0x00CD: // Toggle the whole strip off and on
        toggleStrip();
        Serial.println("Execute Toggle");
        break;
    default:
        Serial.println("Not Known Command");
        break;
    }
}

void loop()
{
    // poll for Bluetooth® Low Energy events
    BLE.poll(50);


    if (!BLE.connected())
    {
        digitalWrite(LED_PIN_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_PIN_BUILTIN, LOW);
        delay(500);
    }
    
}

void blePeripheralConnectHandler(BLEDevice central)
{
    // central connected event handler
    isConnected = true;
    Serial.print("Connected event, central: ");
    Serial.println(central.address());
}

void blePeripheralDisconnectHandler(BLEDevice central)
{
    // central disconnected event handler
    isConnected = false;
    Serial.print("Disconnected event, central: ");
    Serial.println(central.address());
}

void ledCharacteristicWritten(BLEDevice central, BLECharacteristic characteristic)
{
    // central wrote new value to characteristic, update LED
    Serial.print("Characteristic event, written: ");

    const uint8_t *valuedata = characteristic.value();
    Serial.print("    Value: ");
    uint8_t command = 0x0000;
    for (int k = 0; k < characteristic.valueLength(); k++)
    {
        Serial.println(valuedata[k]);
        command = command << 1;
        command = command | valuedata[k];
        Serial.println(command);
    }
    Serial.println(command);
    executeCommand(command);

    ledCharacteristic.setValue(0);
}