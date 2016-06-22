#include <TheAirBoard.h>
;

#define MOISTURE_PIN A4
#define MOISTURE_VCC 10
//#define MOISTURE_GND 11

#define LM35_PIN A5
//#define LM35_VCC 12
//#define LM35_GND 13

#define ENABLE_RF_CONTROL true
#define DELAY_FOR_SETUP 5000

#define DELAY_AFTER_WAKEUP 10000
#define DELAY_BEFORE_SLEEP 500
#define DELAY_FOR_BLINK 5

#define INTERVAL_MILLIS 8000 // set watchdog timeout in milliseconds (max 8000)
#define TIMEOUT_INTERVAL (((10 * 60 * 1000L) - \
                           (DELAY_AFTER_WAKEUP + DELAY_BEFORE_SLEEP)) / \
                          (INTERVAL_MILLIS - DELAY_FOR_BLINK))

//#define ENABLE_RF_CONTROL false
//#define INTERVAL_MILLIS 1000
//#define TIMEOUT_INTERVAL 0


TheAirBoard board;
volatile boolean f_wdt = true;
int timeout = 0;

long blinkColor = 0;
#define RGB(r,g,b) (((long)(r) << 16) + ((g) << 8) + (b))

void setup() {
  Serial.begin(57600);

#ifdef MOISTURE_GND
  pinMode(MOISTURE_GND, OUTPUT);
  digitalWrite(MOISTURE_GND, LOW);
#endif
#ifdef MOISTURE_VCC
  pinMode(MOISTURE_VCC, OUTPUT);
  digitalWrite(MOISTURE_VCC, LOW);
#endif

#ifdef LM35_GND
  pinMode(LM35_GND, OUTPUT);
  digitalWrite(LM35_GND, LOW);
#endif
#ifdef LM35_VCC
  pinMode(LM35_VCC, OUTPUT);
  digitalWrite(LM35_VCC, LOW);
#endif

#if ENABLE_RF_CONTROL
  pinMode(RF, OUTPUT);
  digitalWrite(RF, HIGH);
  analogWrite(BLUE, 1);
  delay(DELAY_FOR_SETUP); // allow time to launch programming, before a possible wireless module power down
  analogWrite(BLUE, 0);
#endif

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  board.setWatchdog(INTERVAL_MILLIS);
}


ISR(WDT_vect) {
  f_wdt = true;
}

void loop() {
  if (f_wdt) {
    if (--timeout <= 0) {
      const char *status;

#if ENABLE_RF_CONTROL
      digitalWrite(RF, HIGH);
#endif
#ifdef MOISTURE_VCC
      digitalWrite(MOISTURE_VCC, HIGH);
#endif
#ifdef LM35_VCC
      digitalWrite(LM35_VCC, HIGH);
#endif
      delay(DELAY_AFTER_WAKEUP);

      //
      // DFRobot SEN0014: Moisture Sensor
      // The sensor value description
      // 0  ~300   dry soil
      // 300~700   humid soil
      // 700~950   in water
      int moisture = analogRead(MOISTURE_PIN);
      if (moisture < 300) { // is dry, blink red
        status = "dry";
        blinkColor = RGB(1, 0, 0);
      } else if (moisture < 400) { // is little dry, blink yellow
      status = "little dry";
        blinkColor = RGB(1, 1, 0);
      } else if (moisture < 700) { // is humid, blink green
        status = "humid";
        blinkColor = RGB(0, 1, 0);
      } else { // in water, blink cyan
        status = "in water";
        blinkColor = RGB(0, 1, 1);
      }
      Serial.print("{\"moisture\":{\"value\":");
      Serial.print(moisture);
      Serial.print(",\"status\":\"");
      Serial.print(status);
      Serial.print("\"}");

      //
      // TI LM35
      float temperature = 3.3 * analogRead(LM35_PIN) / 1024 * 100;
      Serial.print(",\"temperature\":{\"value\":");
      Serial.print(temperature);
      Serial.print("}");

      //
      // Battery
      float voltage = 3.3 * analogRead(VBAT) / 1024 + 1.2;
      status = (analogRead(VUSB) // USB plugged in
                ? (digitalRead(NCHG) ? "charged" : "charging")
                : (voltage < 3.1 ? "low" : "good"));

      Serial.print(",\"battery\":{\"value\":");
      Serial.print(voltage);
      Serial.print(",\"status\":\"");
      Serial.print(status);
      Serial.println("\"}}");

#ifdef MOISTURE_VCC
      digitalWrite(MOISTURE_VCC, LOW);
#endif
#ifdef LM35_VCC
      digitalWrite(LM35_VCC, LOW);
#endif

      delay(DELAY_BEFORE_SLEEP);
#if ENABLE_RF_CONTROL
      digitalWrite(RF, LOW);
#endif

      timeout = TIMEOUT_INTERVAL;
    }

    // blink
    analogWrite(RED, (blinkColor >> 16) & 0xff);
    analogWrite(GREEN, (blinkColor >> 8) & 0xff);
    analogWrite(BLUE, blinkColor & 0xff);
    delay(DELAY_FOR_BLINK);
    analogWrite(RED, 0);
    analogWrite(GREEN, 0);
    analogWrite(BLUE, 0);

    f_wdt = false;
  }

  sleep_mode();
}
