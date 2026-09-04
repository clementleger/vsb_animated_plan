#include <Adafruit_NeoPixel.h>

// Hardware defaults. Change these to match the installation.
constexpr uint16_t LED_COUNT = 120;
constexpr uint8_t LED_DATA_PIN = 6;
constexpr uint8_t NEXT_BUTTON_PIN = 2;

constexpr uint8_t LED_BRIGHTNESS = 80;
constexpr uint16_t FRAME_DELAY_MS = 100;
constexpr uint8_t STATION_RED = 255;
constexpr uint8_t IDLE_LINE_LEVEL = 15;
constexpr uint8_t MOVING_LIGHT_LEVEL = 255;
constexpr uint8_t MOVING_LIGHT_TRAIL = 90;

Adafruit_NeoPixel strip(LED_COUNT, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);

/*
 * A station is represented by the LED mounted at its location.
 * Every station LED is kept red, including when it is an endpoint of
 * the currently animated line.
 */
struct Station {
  uint16_t led;
};

/*
 * firstLed and lastLed are inclusive and must describe one contiguous
 * strip segment. Set reverse to true when the physical strip runs from
 * the end station back toward the start station.
 */
struct Line {
  uint16_t firstLed;
  uint16_t lastLed;
  bool reverse;
};

// Replace these example values with the LED coordinates from the map.
const Station stations[] = {
  {5},   // Station A
  {28},  // Station B
  {54},  // Station C
  {83},  // Station D
  {112}  // Station E
};

// Routes are shown in this order when the button is pressed.
const Line lines[] = {
  {5, 28, false},   // A -> B
  {28, 54, false},  // B -> C
  {54, 83, false},  // C -> D
  {83, 112, false}  // D -> E
};

constexpr size_t STATION_COUNT = sizeof(stations) / sizeof(stations[0]);
constexpr size_t LINE_COUNT = sizeof(lines) / sizeof(lines[0]);

size_t currentLine = 0;
uint16_t movingPosition = 0;
uint32_t lastFrameAt = 0;

bool lastButtonReading = HIGH;
bool stableButtonState = HIGH;
uint32_t buttonChangedAt = 0;
constexpr uint16_t BUTTON_DEBOUNCE_MS = 35;

uint32_t scaledWhite(uint8_t level) {
  return strip.Color(level, level, level);
}
uint32_t trainColor() {
  return strip.Color(0, 0, 255);
}

bool isStation(uint16_t led) {
  for (size_t i = 0; i < STATION_COUNT; ++i) {
    if (stations[i].led == led) {
      return true;
    }
  }
  return false;
}

void drawMap() {
  strip.clear();

  // Draw every route as a dim white idle line.
  for (size_t lineIndex = 0; lineIndex < LINE_COUNT; ++lineIndex) {
    if (lineIndex == currentLine) {
      continue;
    }
    const Line &line = lines[lineIndex];
    for (uint16_t led = line.firstLed; led <= line.lastLed; ++led) {
      if (!isStation(led)) {
        strip.setPixelColor(led, scaledWhite(IDLE_LINE_LEVEL));
      }
    }
  }

  // Add a bright-to-dim moving highlight to the selected route.
  const Line &line = lines[currentLine];
  const uint16_t length = line.lastLed - line.firstLed + 1;
  const uint16_t head = line.reverse
      ? line.lastLed - movingPosition
      : line.firstLed + movingPosition;

  if (!isStation(head)) {
    strip.setPixelColor(head, trainColor());
  }

  if (movingPosition > 0) {
    const uint16_t previous = line.reverse ? head + 1 : head - 1;
    if (previous >= line.firstLed && previous <= line.lastLed &&
        !isStation(previous)) {
      strip.setPixelColor(previous, trainColor());
    }
  }

  // Stations are always red and take priority over route pixels.
  for (size_t i = 0; i < STATION_COUNT; ++i) {
    strip.setPixelColor(stations[i].led, strip.Color(STATION_RED, 0, 0));
  }

  strip.show();

  ++movingPosition;
  if (movingPosition >= length) {
    movingPosition = 0;
  }
}

void advanceLine() {
  currentLine = (currentLine + 1) % LINE_COUNT;
  movingPosition = 0;
}

void readButton() {
  const bool reading = digitalRead(NEXT_BUTTON_PIN);

  if (reading != lastButtonReading) {
    buttonChangedAt = millis();
    lastButtonReading = reading;
  }

  if (millis() - buttonChangedAt >= BUTTON_DEBOUNCE_MS &&
      reading != stableButtonState) {
    stableButtonState = reading;
    // The button is wired between the pin and ground.
    if (stableButtonState == LOW) {
      advanceLine();
    }
  }
}

void setup() {
  pinMode(NEXT_BUTTON_PIN, INPUT_PULLUP);
  strip.begin();
  strip.setBrightness(LED_BRIGHTNESS);
  strip.clear();
  strip.show();
}

void loop() {
  readButton();

  if (millis() - lastFrameAt >= FRAME_DELAY_MS) {
    lastFrameAt = millis();
    drawMap();
  }
}
