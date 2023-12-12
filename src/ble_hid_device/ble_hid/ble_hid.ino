#include <bluefruit.h>

BLEDis bledis;
BLEHidAdafruit blehid;

bool hasKeyPressed = false;

#define ON LOW
#define OFF HIGH

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);

  // Setup BLE
  Bluefruit.begin();
  Bluefruit.setTxPower(4);

  // Configure and Start Device Information Service
  bledis.setManufacturer("Adafruit Industries");
  bledis.setModel("Bluefruit Feather 52");
  bledis.begin();

  // Start BLE HID Service
  blehid.begin();

  Bluefruit.Periph.setConnInterval(9, 12);

  startAdv();
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

void loop()
{
  if (hasKeyPressed)
  {
    hasKeyPressed = false;
    blehid.keyRelease();
    delay(5);
  }

  if (Serial.available())
  {
    char ch = (char) Serial.read();
    Serial.write(ch);
    blehid.keyPress(ch);
    hasKeyPressed = true;
    delay(5);
  }
}
