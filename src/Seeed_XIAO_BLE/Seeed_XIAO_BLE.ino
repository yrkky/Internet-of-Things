#include <ble-iot_inferencing.h>
#include <ArduinoBLE.h>
#include <LSM6DS3.h>

#define ON LOW
#define OFF HIGH
#define CONVERT_G_TO_MS2 9.80665f
#define MAX_ACCEPTED_RANGE 2.0f

// More info on these constants can be found here:
// https://github.com/hathach/tinyusb/blob/master/src/class/hid/hid.h
#define DOUBLE_CLICK_KEY 0x00CD // Keyboard Play/Pause
#define INCREASE_KEY 0x00E9     // Keyboard Volume Up
#define DECREASE_KEY 0x00EA     // Keyboard Volume Down

static bool debug_nn = false;
LSM6DS3 myIMU(I2C_MODE, 0x6A);

bool hasKeyPressed = false;

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        delay(10);

    pinMode(LED_RED, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);

    if (!myIMU.begin())
    {
        printf("Failed to initialize IMU!\r\n");
    }
    else
    {
        printf("IMU initialized\r\n");
    }

    if (EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME != 3)
    {
        printf("ERR: EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME should be equal to 3 (the 3 sensor axes)\n");
        return;
    }

    // initialize the Bluetooth® Low Energy hardware
    BLE.begin();

    Serial.println("Bluetooth® Low Energy Central - LED control");

    digitalWrite(LED_GREEN, ON);
    digitalWrite(LED_BLUE, ON);
    digitalWrite(LED_RED, OFF);

    // start scanning for peripherals
    BLE.scanForUuid("19B10000-E8F2-537E-4F6C-D104768A1214");
}

void loop()
{
    if (!BLE.connected())
    {
        blinkLed(LED_BLUE, 500);
        BLE.scanForUuid("19B10000-E8F2-537E-4F6C-D104768A1214"); // start scanning
    }
    // check if a peripheral has been discovered
    BLEDevice peripheral = BLE.available();

    if (peripheral)
    {
        // discovered a peripheral, print out address, local name, and advertised service
        Serial.print("Found ");
        Serial.print(peripheral.address());
        Serial.print(" '");
        Serial.print(peripheral.localName());
        Serial.print("' ");
        Serial.print(peripheral.advertisedServiceUuid());
        Serial.println();

        if (peripheral.localName() != "LEDStrip")
        {
            Serial.println("Peripheral has no LED service");
            return;
        }

        BLE.stopScan();         // stop scanning when the correct peripheral is discovered
        controlLed(peripheral); // control the peripheral LED
    }
    else
    {
        // peripheral disconnected, start scanning again
        BLE.disconnect();
        BLE.scanForUuid("19B10000-E8F2-537E-4F6C-D104768A1214");
        Serial.println("Peripheral disconnected");
        
        delay(100);
    }
}

void controlLed(BLEDevice peripheral)
{
    // connect to the peripheral
    Serial.println("Connecting ...");

    if (peripheral.connect())
    {
        Serial.println("Connected");
    }
    else
    {
        Serial.println("Failed to connect!");
        return;
    }

    // discover peripheral attributes
    Serial.println("Discovering attributes ...");
    if (peripheral.discoverAttributes())
    {
        Serial.println("Attributes discovered");
    }
    else
    {
        Serial.println("Attribute discovery failed!");
        peripheral.disconnect();
        return;
    }

    // retrieve the LED characteristic
    BLECharacteristic ledCharacteristic = peripheral.characteristic("19B10001-E8F2-537E-4F6C-D104768A1214");

    if (!ledCharacteristic)
    {
        Serial.println("Peripheral does not have LED characteristic!");
        peripheral.disconnect();
        return;
    }
    else if (!ledCharacteristic.canWrite())
    {
        Serial.println("Peripheral does not have a writable LED characteristic!");
        peripheral.disconnect();
        return;
    }

    while (peripheral.connected())
    {

        // while the peripheral is connected
        uint8_t buf1[64] = "Idle";
        uint8_t buf2[64] = "Increase";
        uint8_t buf3[64] = "Decrease";

        printf("Sampling...\n");

        // Allocate a buffer here for the values we'll read from the IMU
        float buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE] = {0};

        for (size_t ix = 0; ix < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; ix += 3)
        {
            uint64_t next_tick = micros() + (EI_CLASSIFIER_INTERVAL_MS * 1000);

            buffer[ix] = myIMU.readFloatAccelX();
            buffer[ix + 1] = myIMU.readFloatAccelY();
            buffer[ix + 2] = myIMU.readFloatAccelZ();

            for (int i = 0; i < 3; i++)
            {
                if (fabs(buffer[ix + i]) > MAX_ACCEPTED_RANGE)
                {
                    buffer[ix + i] = get_sign(buffer[ix + i]) * MAX_ACCEPTED_RANGE;
                }
            }

            buffer[ix + 0] *= CONVERT_G_TO_MS2;
            buffer[ix + 1] *= CONVERT_G_TO_MS2;
            buffer[ix + 2] *= CONVERT_G_TO_MS2;

            delayMicroseconds(next_tick - micros());
        }

        // Raw buffer into a classifiable signal
        signal_t signal;
        int err = numpy::signal_from_buffer(buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
        if (err != 0)
        {
            printf("Failed to create signal from buffer (%d)\n", err);
            return;
        }

        // Run the classifier
        ei_impulse_result_t result = {0};
        err = run_classifier(&signal, &result, debug_nn);
        if (err != EI_IMPULSE_OK)
        {
            printf("ERR: Failed to run classifier (%d)\n", err);
            return;
        }

        // print the predictions
        printf("Predictions ");
        printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)",
               result.timing.dsp, result.timing.classification, result.timing.anomaly);
        printf(": \n");
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++)
        {
            printf("    %s: %.5f\n", result.classification[ix].label, result.classification[ix].value);
        }

#if EI_CLASSIFIER_HAS_ANOMALY == 1
        printf("    anomaly score: %.3f\n", result.anomaly);
#endif

        if (result.classification[0].value > 0.8)
        {
            printf("Decrease\n");
            ledCharacteristic.writeValue((byte)DECREASE_KEY);
            blinkLed(LED_RED, 100);
        }

        if (result.classification[1].value > 0.8)
        {
            printf("Double click\n");
            ledCharacteristic.writeValue((byte)DOUBLE_CLICK_KEY);
            blinkLed(LED_BLUE, 100);
        }

        if (result.classification[2].value > 0.8)
        {
            printf("Idle\n");
            delay(10);
        }

        if (result.classification[3].value > 0.8)
        {
            printf("Increase\n");
            ledCharacteristic.writeValue((byte)INCREASE_KEY);
            blinkLed(LED_GREEN, 100);
        }
    }

    Serial.println("Peripheral disconnected");
    peripheral.disconnect();

    if (peripheral.connected())
    {
        Serial.println("Peripheral still connected.");
    }
    else
    {
        Serial.println("Peripheral disconnected: YES");
    }
    return;
}

float get_sign(float number)
{
    return (number >= 0.0) ? 1.0 : -1.0;
}

void blinkLed(int ledPin, uint32_t delay_ms)
{
    digitalWrite(ledPin, ON);
    delay(delay_ms);
    digitalWrite(ledPin, OFF);
    delay(delay_ms);
}