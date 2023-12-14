#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#define PIN_WS2812B 4
#define NUM_PIXELS 60
#define LED_BUILTIN 2

#define UP 0x00E9
#define DOWN 0x00EA
#define PP 0x00CD

Adafruit_NeoPixel ws2812b(NUM_PIXELS, PIN_WS2812B, NEO_GRB + NEO_KHZ800);
static BLERemoteCharacteristic *hidReportCharacteristic;

static BLEUUID serviceUUID((uint16_t)0x1812); // The remote service we wish to connect to.
static BLEUUID charUUID((uint16_t)0x2a4d);    // The characteristic of the remote service we are interested in.

static bool doConnect = false;
static bool connected = false;
static bool doScan = false;
static BLEAdvertisedDevice *bleDevice;

static void notifyCallback(
    BLERemoteCharacteristic *pBLERemoteCharacteristic,
    uint8_t *pData,
    size_t length,
    bool isNotify)
{
#ifdef DEBUG
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    for (size_t i = 0; i < length; i++)
    {
        Serial.printf("%2x", pData[i]);
    }
    Serial.println("");
#endif
}

class BLECallbacks : public BLEClientCallbacks
{
    void onConnect(BLEClient *pclient)
    {
        printf("Connected to XIAO nRF52840 Sense \n");
        digitalWrite(LED_BUILTIN, HIGH);
        connected = true;
    }

    void onDisconnect(BLEClient *pclient)
    {
        printf("Disconnected from XIAO nRF52840 Sense \n");
        digitalWrite(LED_BUILTIN, LOW);
        connected = false;
    }
};

class ClientAdvertisedCallbacks : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        Serial.print("BLE Advertised Device found: ");
        Serial.println(advertisedDevice.toString().c_str());
        if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID))
        {

            BLEDevice::getScan()->stop();
            bleDevice = new BLEAdvertisedDevice(advertisedDevice);
            doConnect = true;
            doScan = true;
        }
    }
};

bool connectToServer()
{
    Serial.print("Forming a connection to ");
    Serial.println(bleDevice->getAddress().toString().c_str());

    BLEClient *pClient = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new BLECallbacks());

    // Connect to the remove BLE Server.
    if (!pClient->connect(bleDevice))
    { // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
        return false;
    }
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr)
    {
        Serial.print("Failed to find our service UUID: ");
        Serial.println(serviceUUID.toString().c_str());
        pClient->disconnect();
        return false;
    }
    Serial.println(" - Found our service");

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    do
    {
        hidReportCharacteristic = pRemoteService->getCharacteristic(charUUID);
        if (hidReportCharacteristic == nullptr)
        {
            Serial.print("Failed to find our characteristic UUID: ");
            Serial.println(charUUID.toString().c_str());
            pClient->disconnect();
            return false;
        }

        if (hidReportCharacteristic->canNotify())
            break;
    } while (1);
    Serial.println(" - Found our characteristic");

    hidReportCharacteristic->registerForNotify(notifyCallback);

    return true;
}

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

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        sleep(10);

    // initialize built-in LED pin
    pinMode(LED_BUILTIN, OUTPUT);

    // initialize WS2812B
    ws2812b.begin();

    // initialize BLE hardware
    Serial.println("Starting BLE Client application! \n");

    BLEDevice::init("");
    BLEScan *pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new ClientAdvertisedCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(5, false);

    // if (!BLE.begin())
    // {
    //     Serial.println("Starting Bluetooth® Low Energy module failed!");
    //     sleep(1000);
    // }

    // BLE.setEventHandler(BLEDisconnected, bleDisconnectHandler);
    // BLE.setEventHandler(BLEConnected, bleConnectHandler);

    Serial.println("Bluetooth® Low Energy Central - LED control");
}

void loop()
{

    // If the flag "doConnect" is true then we have scanned for and found the desired
    // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
    // connected we set the connected flag to be true.
    if (doConnect == true)
    {
        if (connectToServer())
        {
            Serial.println("We are now connected to the BLE Server.");
        }
        else
        {
            Serial.println("We have failed to connect to the server; there is nothin more we will do.");
        }
        doConnect = false;
    }

    // If we are connected to a peer BLE Server, update the characteristic each time we are reached
    // with the current time since boot.
    if (connected)
    {
        // do nothing, all data is handled in notifications callback
    }
    else if (doScan)
    {
        BLEDevice::getScan()->start(0); // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
    }

    delay(1000);
}

// Delay a second between loops.

// if (!BLE.connected())
// {
//     BLE.scanForName("XIAO nRF52840 S");
//     BLEDevice peripheral = BLE.available();
//     if (peripheral)
//     {
//         Serial.print("Found ");
//         Serial.print(peripheral.address());
//         Serial.print(" '");
//         Serial.print(peripheral.localName());
//         Serial.print("' ");
//         Serial.print(peripheral.advertisedServiceUuid());

//         Serial.println();

//         BLE.stopScan();
//         if (peripheral.connect())
//         {
//             Serial.println("Connected");
//             digitalWrite(LED_BUILTIN, HIGH);
//             checkCharacteristics(peripheral);
//         }
//         else
//         {
//             Serial.println("Failed to connect!");
//         }
//     }
//     else
//     {
//         Serial.println("No peripheral found!");
//         digitalWrite(LED_BUILTIN, LOW);
//         delay(1000);
//     }
// }
// else
// {
//     Serial.println("Connected to XIAO nRF52840 Sense \n");
//     Serial.println("Waiting for key press \n");
//     delay(1000);
// }
// }

// void checkCharacteristics(BLEDevice peripheral)
// {
//     while (peripheral.connected())
//     {
//         if (peripheral.discoverAttributes())
//         {
//             if (peripheral.discoverService("1812"))
//             {
//                 Serial.println("Service discovered");
//                 hidReportCharacteristic = peripheral.characteristic("1812"); // UUID for HID report characteristic

//                 if (hidReportCharacteristic.canSubscribe())
//                 {
//                     Serial.println("Characteristic can subscribe");
//                 }
//                 else
//                 {
//                     Serial.println("Characteristic can not subscribe");
//                 }

//                 if (hidReportCharacteristic)
//                 {
//                     Serial.println("Found HID report characteristic");

//                     if (hidReportCharacteristic.subscribe())
//                     {
//                         Serial.println("Subscribed to HID report characteristic");
//                     }
//                 }
//             }
//             while (peripheral.connected())
//             {
//                 if (hidReportCharacteristic.valueUpdated())
//                 {
//                     byte report[8];
//                     hidReportCharacteristic.readValue(report, sizeof(report));
//                     Serial.print("Report: ");
//                     for (int i = 0; i < sizeof(report); i++)
//                     {
//                         Serial.print(report[i]);
//                         Serial.print(" ");
//                     }
//                     Serial.println();
//                     controlLeds(report[2]);
//                 }
//                 else
//                 {
//                     Serial.println("No report");
//                 }
//             }
//         }
//         else
//         {
//             printf("Attribute discovery failed!");
//             peripheral.disconnect();
//         }

//         BLEService HIDservice = peripheral.service("1812");
//         Serial.println("Service: ");
//         Serial.println(HIDservice.uuid());                    // 1812
//         Serial.println(HIDservice.hasCharacteristic("1812")); // 0
//         BLECharacteristic HIDcharacteristic = HIDservice.characteristic("1812");

//         // bleReadCharacteristics(HIDcharacteristic);
//         // controlLeds();
//     }
// }

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