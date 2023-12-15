#include <ArduinoBLE.h>

bool connected = false;
BLECentral central;

void setup()
{
  Serial.begin(9600);
  while (!Serial)
    ;

  if (!BLE.begin())
  {
    Serial.println("Starting BLE failed!");
    while (1)
      ;
  }

  BLE.setLocalName("BLE Client");

  Serial.println("Scanning for BLE peripherals...");
  BLE.scanForUuid("1812"); // Scan for the service UUID
}

void blePeripheralConnectHandler(BLEDevice central)
{
  connected = true;
  Serial.println("Connected");
}

void blePeripheralDisconnectHandler(BLEDevice central)
{
  connected = false;
  Serial.println("Disconnected");
}

void bleCharacteristicReceived(BLEDevice peripheral, BLECharacteristic characteristic)
{
  Serial.print("Received data: ");
  Serial.print("Characteristic event, written: ");
  characteristic.read();
  const uint8_t *valuedata = characteristic.value();
  Serial.print("    Value: ");
  for (int k = 0; k < characteristic.valueLength(); k++)
  {
    Serial.print(valuedata[k]);
  }
  Serial.println();
}

void loop()
{
  if (!connected)
  {
    BLEDevice peripheral = BLE.available();
    if (peripheral)
    {
      if (peripheral.localName() == "FingerTracker")
      {                                 // Replace with your peripheral's name
        central = peripheral.connect(); // Connect to the peripheral
        if (central.connected())
        {
          connected = true;
          central.setEventHandler(BLEConnected, blePeripheralConnectHandler);
          central.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);
          central.setEventHandler(BLECharacteristicValue, bleCharacteristicReceived);
          Serial.println("Connected to peripheral!");
        }
      }
    }
  }
}
