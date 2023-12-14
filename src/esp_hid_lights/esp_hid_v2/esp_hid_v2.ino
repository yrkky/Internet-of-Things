#include <ArduinoBLE.h>

#define DOUBLE_CLICK_KEY 0x00CD
#define INCREASE_KEY 0x00E9
#define DECREASE_KEY 0x00EA

BLEService commandService("00001812-0000-1000-8000-00805f9b34fb");
BLEByteCharacteristic commandCharacteristic("1812", BLERead | BLENotify);
bool connected = false;

void setup()
{
  Serial.begin(9600);
  while (!Serial)
    ;

  pinMode(LED_BUILTIN, OUTPUT); // Set LED pin as output

  if (!BLE.begin())
  {
    Serial.println("Starting BLE failed!");
    while (1)
      ;
  }

  BLE.setLocalName("CommandReceiver");
  BLE.setAdvertisedService(commandService);

  commandService.addCharacteristic(commandCharacteristic);
  BLE.addService(commandService);

  commandCharacteristic.setValue(0); // Initialize characteristic value

  BLE.advertise();

  Serial.println("BLE Command Receiver ready");
}

void loop()
{
  if (!connected)
  {
    Serial.println("Scanning...");
    BLE.scanForName("FingerTracker");
    BLEDevice peripheral = BLE.available();

    if (peripheral)
    {
      Serial.print("Connecting to device: ");
      Serial.println(peripheral.address());

      BLE.stopScan(); // Stop scanning before attempting to connect

      if (peripheral.connect())
      {
        Serial.println("Connected");
        connected = true;
      }
      else
      {
        Serial.println("Failed to connect");
      }
    }
    else
    {
      // Blink LED while not connected to Bluetooth
      digitalWrite(LED_BUILTIN, HIGH);
      delay(500);
      digitalWrite(LED_BUILTIN, LOW);
      delay(500);
    }
  }
  else
  {
    // Check for incoming commands from "FingerTracker"
    if (BLE.connected())
    {
      BLEDevice peripheral = BLE.central();
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

      // BLEDevice peripheral = BLE.central();
      BLECharacteristic commandCharacteristic = peripheral.characteristic("00001812-0000-1000-8000-00805f9b34fb", 0x2a4d);

      if (commandCharacteristic.canRead())
      {
        const uint16_t *value;
        commandCharacteristic.read();
        value = commandCharacteristic.value();
        printf("Received: %d\n", value);
      }
      // Check received command
      // switch (value)
      // {
      // case DOUBLE_CLICK_KEY:
      //   Serial.println("Received Double Click Key");
      //   // Do something for double click key command
      //   // e.g., trigger an action
      //   break;
      // case INCREASE_KEY:
      //   Serial.println("Received Increase Key");
      //   // Do something for increase key command
      //   // e.g., trigger another action
      //   break;
      // case DECREASE_KEY:
      //   Serial.println("Received Decrease Key");
      //   // Do something for decrease key command
      //   // e.g., trigger a different action
      //   break;
      // default:
      //   Serial.println("Unknown Command Received");
      //   break;
      // }
      //}
      // }
      // ... (rest of your connected BLE handling code)
    }
  }
}