#include <Adafruit_NeoPixel.h>

// Hardware defaults. Change these to match the installation.
constexpr uint16_t LED_COUNT = 120;
constexpr uint8_t LED_DATA_PIN = 6;
constexpr uint8_t NEXT_BUTTON_PIN = 2;

constexpr uint8_t LED_BRIGHTNESS = 80;
constexpr uint16_t FRAME_DELAY_MS = 150;
constexpr uint32_t SEQUENCE_START_DELAY_S = 92;
constexpr uint8_t STATION_RED = 255;
constexpr uint8_t IDLE_LINE_LEVEL = 50;
constexpr uint8_t MOVING_LIGHT_LEVEL = 255;
constexpr uint8_t TRAIN_LENGTH = 4; // Number of LEDs in the moving train.

Adafruit_NeoPixel strip(LED_COUNT, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);

/*
 * A station is represented by the LED mounted at its location.
 * Every station LED is kept red, including when it is an endpoint of
 * the currently animated line.
 */
struct Station {
  uint16_t led;
  const char *name;
};

/*
 * firstLed and lastLed are inclusive and must describe one contiguous
 * strip segment. Set reverse to true when the physical strip runs from
 * the end station back toward the start station.
 */
struct Line {
  uint16_t firstLed;
  uint16_t lastLed;
  uint32_t seconds;
};

// Routes are shown in this order during the timed sequence.
const Line lines[] = {
    {0, 30, 66},  // A -> B, seconds
    {30, 51, 32}, // B -> C, seconds
    {51, 76, 67}, // C -> D, seconds
    {76, 83, 0}  // D -> E, seconds
};

constexpr size_t LINE_COUNT = sizeof(lines) / sizeof(lines[0]);

size_t currentLine = 0;
uint16_t movingPosition = 0;
uint32_t lastFrameAt = 0;
uint32_t currentLineStartedAt = 0;
uint32_t sequenceStartRequestedAt = 0;
bool sequenceMode = false;
bool sequenceStartPending = false;

bool lastButtonReading = HIGH;
bool stableButtonState = HIGH;
uint32_t buttonChangedAt = 0;
constexpr uint16_t BUTTON_DEBOUNCE_MS = 35;

uint32_t scaledWhite(uint8_t level) { return strip.Color(level, level, level); }
uint32_t trainColor() { return strip.Color(255, 128, 0); }

void drawStation(uint16_t led) {
  if (led != lines[0].firstLed && led != lines[LINE_COUNT - 1].lastLed) {
    strip.setPixelColor(led, strip.Color(STATION_RED, 0, 0));
  }
}

void drawLine(Line *line, bool active) {
  for (uint16_t led = line->firstLed; led <= line->lastLed; ++led) {
    if (active) {
      strip.setPixelColor(led, scaledWhite(MOVING_LIGHT_LEVEL));
    } else {
      strip.setPixelColor(led, scaledWhite(IDLE_LINE_LEVEL));
    }
  }

  drawStation(line->firstLed);
  drawStation(line->lastLed);
}

void drawTrain(uint16_t startLed, uint16_t lastLed) {
  for (uint8_t offset = 0; offset < TRAIN_LENGTH && offset <= movingPosition;
       ++offset) {
    const uint16_t trainLed = movingPosition - offset;
    if (trainLed >= startLed && trainLed <= lastLed) {
      strip.setPixelColor(trainLed, trainColor());
    }
  }

  movingPosition++;
  if (movingPosition > lastLed + TRAIN_LENGTH - 1) {
    movingPosition = startLed;
  }
}

void drawFullMap() {
  strip.clear();

  // Draw every route as a dim white idle line.
  for (size_t lineIndex = 0; lineIndex < LINE_COUNT; ++lineIndex) {
    drawLine(&lines[lineIndex], false);
  }

  drawTrain(lines[0].firstLed, lines[LINE_COUNT - 1].lastLed);
  strip.show();
}

void drawSequenceLine() {
  strip.clear();
  const Line &line = lines[currentLine];
  drawLine(&lines[currentLine], true);
  drawTrain(line.firstLed, line.lastLed - 1);
  for (size_t lineIndex = 0; lineIndex < LINE_COUNT; ++lineIndex) {
    if (lineIndex != currentLine) {
      drawLine(&lines[lineIndex], false);
    }
  }
  strip.show();
}

void startSequence() {
  sequenceMode = false;
  sequenceStartPending = true;
  sequenceStartRequestedAt = millis();
}

void updateSequence() {
  if (sequenceStartPending) {
    if (millis() - sequenceStartRequestedAt < SEQUENCE_START_DELAY_S * 1000UL) {
      return;
    }

    currentLine = 0;
    movingPosition = lines[currentLine].firstLed;
    currentLineStartedAt = millis();
    sequenceStartPending = false;
    sequenceMode = true;
    return;
  }

  if (!sequenceMode) {
    return;
  }

  if (millis() - currentLineStartedAt < lines[currentLine].seconds * 1000UL) {
    return;
  }

  if (currentLine + 1 >= LINE_COUNT) {
    sequenceMode = false;
    movingPosition = lines[0].firstLed;
    return;
  }

  ++currentLine;
  movingPosition = lines[currentLine].firstLed;
  currentLineStartedAt = millis();
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
      startSequence();
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
  updateSequence();

  if (millis() - lastFrameAt >= FRAME_DELAY_MS) {
    lastFrameAt = millis();
    if (sequenceMode) {
      drawSequenceLine();
    } else {
      drawFullMap();
    }
  }
}
