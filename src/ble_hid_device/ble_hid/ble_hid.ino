#include <ble-iot_inferencing.h>
#include <bluefruit.h>
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

const int RED_PIN = 11;
const int BLUE_PIN = 12;
const int GREEN_PIN = 13;

static bool debug_nn = false;
LSM6DS3 myIMU(I2C_MODE, 0x6A);
BLEDis bledis;
BLEHidAdafruit blehid;

bool hasKeyPressed = false;

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        delay(10);
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_BLUE, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);

    // if (!IMU.begin()) {
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

    // Setup BLE
    Bluefruit.begin();
    Bluefruit.setTxPower(4);
    Bluefruit.setName("FingerTracker");

    // Configure and Start Device Information Service
    bledis.setManufacturer("Adafruit Industries");
    bledis.setModel("Bluefruit Feather 52");
    bledis.begin();

    // Start BLE HID Service
    blehid.begin();

    Bluefruit.Periph.setConnInterval(9, 12);

    startAdv();
}

void loop()
{

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
        sendKey(DECREASE_KEY);
        blinkLed(RED_PIN, 100);
    }

    if (result.classification[1].value > 0.8)
    {
        printf("Double click\n");
        sendKey(DOUBLE_CLICK_KEY);
        blinkLed(BLUE_PIN, 100);
    }

    if (result.classification[2].value > 0.8)
    {
        printf("Idle\n");
        delay(10);
    }

    if (result.classification[3].value > 0.8)
    {
        printf("Increase\n");
        sendKey(INCREASE_KEY);
        blinkLed(GREEN_PIN, 100);
    }

    if (hasKeyPressed)
    {
        hasKeyPressed = false;
        blehid.keyRelease();
        delay(5);
    }

    if (Serial.available())
    {
        char ch = (char)Serial.read();
        Serial.write(ch);
        uint16_t key = ch;
        sendKey(key);
        blehid.keyPress(ch);
        hasKeyPressed = true;
        delay(5);
    }
}

void sendKey(uint16_t key)
{
    blehid.consumerKeyPress(key);
    delay(5);
    blehid.consumerKeyRelease();
    delay(5);
}

/**
 * @brief Return the sign of the number
 *
 * @param number
 * @return int 1 if positive (or 0) -1 if negative
 */
float get_sign(float number)
{
    return (number >= 0.0) ? 1.0 : -1.0;
}

void startAdv(void)
{
    // Advertising packet
    Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
    Bluefruit.Advertising.addTxPower();
    Bluefruit.Advertising.addAppearance(BLE_APPEARANCE_HID_KEYBOARD);

    // Include BLE HID service
    Bluefruit.Advertising.addService(blehid);
    Bluefruit.Advertising.addName();

    // Set advertising interval
    Bluefruit.Advertising.restartOnDisconnect(true);
    Bluefruit.Advertising.setInterval(32, 244);
    Bluefruit.Advertising.setFastTimeout(30);
    Bluefruit.Advertising.start(0);
}

void blinkLed(int ledPin, uint32_t delay_ms)
{
    digitalWrite(ledPin, ON);
    delay(delay_ms);
    digitalWrite(ledPin, OFF);
    delay(delay_ms);
}