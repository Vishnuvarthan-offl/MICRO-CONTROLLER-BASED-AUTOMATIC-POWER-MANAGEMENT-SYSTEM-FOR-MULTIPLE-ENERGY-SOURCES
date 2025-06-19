// Define input pins for power source detection
#define SOURCE1_PIN 2  // Digital pin for Solar
#define SOURCE2_PIN 3  // Digital pin for Grid Mains
#define SOURCE3_PIN 4  // Digital pin for Battery

// Define output pins for relays
#define RELAY1_PIN 5   // Relay for Solar
#define RELAY2_PIN 6   // Relay for Grid Mains
#define RELAY3_PIN 7   // Relay for Battery
#define RELAY4_PIN 8   // Relay controlled by ACS712 current sensor
#define RELAY5_PIN 9   // Relay 5 (Opposite of Relay 4)

// Define ACS712 current sensor pin
#define ACS712_PIN A0  // Analog pin for current sensor

// Calibration factor (change based on ACS712 model)
#define SENSITIVITY 0.100  // For ACS712-20A (Adjust if using 5A or 30A)

// Offset voltage (typically 2.5V for ACS712)
#define OFFSET_VOLTAGE 2.5

// Current threshold (450 mA = 0.45 A)
#define CURRENT_THRESHOLD 0.14  

bool previousSourceWasBattery = false; // Track if the last active source was battery

void setup() {
    Serial.begin(9600); // Start serial communication

    pinMode(SOURCE1_PIN, INPUT_PULLUP);
    pinMode(SOURCE2_PIN, INPUT_PULLUP);
    pinMode(SOURCE3_PIN, INPUT_PULLUP);

    pinMode(RELAY1_PIN, OUTPUT);
    pinMode(RELAY2_PIN, OUTPUT);
    pinMode(RELAY3_PIN, OUTPUT);
    pinMode(RELAY4_PIN, OUTPUT);
    pinMode(RELAY5_PIN, OUTPUT);

    // Start with Solar ON (LOW = ON for active LOW relay), others OFF
    digitalWrite(RELAY1_PIN, LOW);
    digitalWrite(RELAY2_PIN, HIGH);
    digitalWrite(RELAY3_PIN, HIGH);
    digitalWrite(RELAY4_PIN, LOW);  // Relay 4 initially ON
    digitalWrite(RELAY5_PIN, HIGH); // Relay 5 initially OFF

    Serial.println("System Initialized. Waiting for source detection...\n");
}

void loop() {
    bool solar = digitalRead(SOURCE1_PIN);  // LOW = Power Available
    bool grid = digitalRead(SOURCE2_PIN);
    bool battery = digitalRead(SOURCE3_PIN);

    String sourceName = "Unknown";
    bool isBatterySource = false;

    if (solar) {
        digitalWrite(RELAY1_PIN, LOW);
        digitalWrite(RELAY2_PIN, HIGH);
        digitalWrite(RELAY3_PIN, HIGH);
        sourceName = "Solar";
        isBatterySource = false;
    } 
    else if (grid) {
        digitalWrite(RELAY1_PIN, HIGH);
        digitalWrite(RELAY2_PIN, LOW);
        digitalWrite(RELAY3_PIN, HIGH);
        sourceName = "Grid Mains";
        isBatterySource = false;
    } 
    else if (battery) {
        digitalWrite(RELAY1_PIN, HIGH);
        digitalWrite(RELAY2_PIN, HIGH);
        digitalWrite(RELAY3_PIN, LOW);
        sourceName = "Battery";
        isBatterySource = true;
    }

    // Read ACS712 sensor value
    int rawValue = analogRead(ACS712_PIN);
    float voltage = (rawValue * 5.0) / 1023.0; // Convert ADC to voltage
    float current = abs((voltage - OFFSET_VOLTAGE) / SENSITIVITY); // Convert to Amps & ensure positive

    // Print structured and formatted output
    Serial.println("\n----------------------------------");
    Serial.print("🔋 Active Power Source: ");
    Serial.println(sourceName);
    Serial.print("⚡ Current: ");
    Serial.print(current, 2); // Display current with 2 decimal places
    Serial.println(" A");

    // Overcurrent protection *only applies if Battery is the source*
    if (isBatterySource) {
        if (current > CURRENT_THRESHOLD) {
            digitalWrite(RELAY4_PIN, HIGH);  // Turn OFF Load (Relay 4)
            digitalWrite(RELAY5_PIN, LOW);   // Relay 5 turns ON
            Serial.println("❌ Overcurrent on Battery! Load is OFF (Relay 4: OFF, Relay 5: ON)");
        } else {
            digitalWrite(RELAY4_PIN, LOW);   // Turn ON Load (Relay 4)
            digitalWrite(RELAY5_PIN, HIGH);  // Relay 5 turns OFF
            Serial.println("✅ Normal Current. Load is ON (Relay 4: ON, Relay 5: OFF)");
        }
        previousSourceWasBattery = true; // Mark battery as the last source used
    } 
    else {
        // If switching from Battery to Solar/Grid, reset Relay 4 to ON
        if (previousSourceWasBattery) {
            digitalWrite(RELAY4_PIN, LOW);  // Ensure Relay 4 is ON
            digitalWrite(RELAY5_PIN, HIGH); // Ensure Relay 5 is OFF
            Serial.println("🔄 Switching from Battery. Resetting Load (Relay 4: ON, Relay 5: OFF)");
            previousSourceWasBattery = false; // Reset the flag
        } else {
            Serial.println("⚠ Overcurrent protection is inactive (Not on Battery Mode)");
        }
    }

    Serial.println("----------------------------------");

    delay(2500); // Wait before next reading
}