#define DEBUG 1  // Set to 0 to disable debug output


#if DEBUG
  #define DEBUG_PRINT(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)
#endif


const int red = 6; // LEDs are assigned to pins 6, 9 and 12
const int yellow = 9;
const int green = 12;
const int switchPin = 5;


void setup() {

  Serial.begin(115200);           // initialise the serial line
  pinMode(LED_BUILTIN, OUTPUT);   // set up GPIO pin for built-in LED

  pinMode(red, OUTPUT);
  pinMode(yellow, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(switchPin, INPUT_PULLUP);
  
  digitalWrite(green, HIGH);

}

void loop() {
  if (digitalRead(switchPin) == LOW) {
    DEBUG_PRINT("Button pressed: Starting sequence");

    delay(2000);
    digitalWrite(green, LOW);
    digitalWrite(yellow, HIGH);
    DEBUG_PRINT("Green LED OFF, Yellow LED ON");

    delay(1000);
    digitalWrite(yellow, LOW);
    digitalWrite(red, HIGH);
    DEBUG_PRINT("Yellow LED OFF, Red LED ON");

    delay(5000);
    digitalWrite(yellow, HIGH);
    delay(1000);

    digitalWrite(red, LOW);
    digitalWrite(yellow, LOW);
    digitalWrite(green, HIGH);
    DEBUG_PRINT("Cycle complete: Green LED ON");
  }
}
