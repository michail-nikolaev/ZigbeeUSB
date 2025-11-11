#include <Arduino.h>
#include <Zigbee.h>

// Define the GPIO pin connected to the MOSFET Gate.
const int MOSFET_GATE_PIN = D10;

// The built-in LED is usually defined as LED_BUILTIN.
// On the ESP32-C6, this typically corresponds to GPIO8.
const int ONBOARD_LED_PIN = LED_BUILTIN;

// The interval for each state (on/off) in milliseconds.
const int BLINK_INTERVAL = 10000; // 3 seconds

ZigbeeHueLight *powerOutlet;

void setup()
{
  Serial.begin(115200);
  Serial.println("USB & LED Power Controller Initialized");

  // Set the MOSFET gate pin as an output.
  pinMode(MOSFET_GATE_PIN, OUTPUT);
  // Set the built-in LED pin as an output.
  pinMode(ONBOARD_LED_PIN, OUTPUT);
  pinMode(BOOT_PIN, INPUT_PULLUP);

  // Ensure both the USB port and LED are off at the start.
  digitalWrite(MOSFET_GATE_PIN, LOW);
  digitalWrite(ONBOARD_LED_PIN, LOW);
  Serial.println("Initial state: USB Port & LED OFF");

  uint8_t phillips_hue_key[] = {0x81, 0x45, 0x86, 0x86, 0x5D, 0xC6, 0xC8, 0xB1, 0xC8, 0xCB, 0xC4, 0x2E, 0x5D, 0x65, 0xD3, 0xB9};
  Zigbee.setEnableJoiningToDistributed(true);
  Zigbee.setStandardDistributedKey(phillips_hue_key);

  powerOutlet = new ZigbeeHueLight(10, ESP_ZB_HUE_LIGHT_TYPE_DIMMABLE);

  powerOutlet->setManufacturerAndModel("nkey", "powerOutlet");
  powerOutlet->setSwBuild("0.0.1");
  powerOutlet->setOnOffOnTime(0);
  powerOutlet->setOnOffGlobalSceneControl(false);

  Zigbee.addEndpoint(powerOutlet);

  if (!Zigbee.begin(ZIGBEE_ROUTER, false))
  {
    Serial.println("Zigbee failed to start!");
    Serial.println("Rebooting...");
    ESP.restart();
  }

  Serial.println("Connecting Zigbee to network");

  while (!Zigbee.connected())
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
  }
}

void resetSystem()
{
  Serial.println("=== System Reset ===");

  Serial.println("Resetting Zigbee network...");
  Zigbee.factoryReset();

  Serial.println("System reset complete - device will restart");
  delay(500); // Additional delay to ensure all operations complete
  ESP.restart();
}

void checkForReset()
{
  // Checking button for factory reset
  if (digitalRead(BOOT_PIN) == LOW)
  { // Push button pressed
    // Key debounce handling
    vTaskDelay(pdMS_TO_TICKS(100));
    int startTime = millis();
    bool ledState = false;
    unsigned long lastBlink = millis();
    const int BLINK_INTERVAL = 100; // Fast blink every 100ms

    while (digitalRead(BOOT_PIN) == LOW)
    {
      // Fast blink built-in LED while button is held
      if (millis() - lastBlink >= BLINK_INTERVAL)
      {
        ledState = !ledState;
        digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
        lastBlink = millis();
      }

      vTaskDelay(pdMS_TO_TICKS(10));
      ; // Short delay to prevent excessive CPU usage

      if ((millis() - startTime) > 3000)
      {
        // If key pressed for more than 3secs, perform unified system reset
        Serial.println("Button held for 3+ seconds - performing full system reset");
        digitalWrite(LED_BUILTIN, LOW); // Turn off LED before reset
        resetSystem();
      }
    }

    // Button released - turn off LED
    digitalWrite(LED_BUILTIN, LOW);
  }
}

// void loop runs over and over again
void loop()
{
  // Turn the USB port and the LED ON.
  digitalWrite(MOSFET_GATE_PIN, LOW);
  digitalWrite(ONBOARD_LED_PIN, HIGH);
  Serial.println("USB Port & LED OFF");

  // Wait for the specified interval.
  delay(BLINK_INTERVAL);

  // Turn the USB port and the LED OFF.
  digitalWrite(MOSFET_GATE_PIN, HIGH);
  digitalWrite(ONBOARD_LED_PIN, LOW);
  Serial.println("USB Port & LED ON");

  // Wait before repeating the loop.
  delay(BLINK_INTERVAL);

  checkForReset();
}