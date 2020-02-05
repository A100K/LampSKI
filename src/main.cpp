#include <M5StickC.h>
#include <FastLED.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <Arduino.h>
// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

// BLE Settings
//-------------------
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_MODE "1058010e-52bc-4554-9c86-88933fde581a"
#define CHARACTERISTIC_SPEED "2058010e-52bc-4554-9c86-88933fde581a"
#define CHARACTERISTIC_BRIGHTNESS "3a0af36a-469e-4ef0-836d-a0ae9bcfaf91"

FASTLED_USING_NAMESPACE
#define DATA_PIN 26
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS 110
CRGB leds[NUM_LEDS];

#define BRIGHTNESS 80
#define FRAMES_PER_SECOND 80
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0;                  // rotating "base color" used by many of the patterns
uint8_t gBrighness = 80;
uint8_t gSpeed = 80;
// function delcairations
//---------------------------------
void rainbow();
void addGlitter(fract8 chanceOfGlitter);
void rainbowWithGlitter();
void confetti();
void sinelon();
void bpm();
void juggle();
void snow();
void raindrops();

void readbutton();
void nextPattern();
void setBrightness(long value);
void setSpeed(long value);
void setMode(long value);
void snow();
void drops();

// drops
uint8_t drop_pos = 0;
uint8_t drop_state = 0;
uint8_t drop_hue = 0;

typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = {rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm, snow, raindrops};

//BLE Callbacks
//---------------------------------------
class LampSkiSetSpeed : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        std::string rxValue = pCharacteristic->getValue();
        //Serial.println(rxValue.c_str());
        if (rxValue.length() > 0){
            char *p;
            long value = strtol(rxValue.c_str(), &p, 10);
            if (*p) Serial.println("conversion failed because the input wasn't a number");
            else setSpeed(value);
        }
    }
};
class LampSkiSetBrightness : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        std::string rxValue = pCharacteristic->getValue();
        //Serial.println(rxValue.c_str());
        if (rxValue.length() > 0)
        {
            char *p;
            long value = strtol(rxValue.c_str(), &p, 10);
            if (*p)
                Serial.println("conversion failed because the input wasn't a number");
            else
                setBrightness(value);
        }
    }
};
class LampSkiSetMode: public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        std::string rxValue = pCharacteristic->getValue();
        //Serial.println(rxValue.c_str());
        if (rxValue.length() > 0)
        {
            char *p;
            long value = strtol(rxValue.c_str(), &p, 10);
            if (*p)
                Serial.println("conversion failed because the input wasn't a number");
            else
                setMode(value);
        }
    }
};

// SETUP
//------------------------------
void setup()
{
    delay(3000); // 3 second delay for recovery

    // tell FastLED about the LED strip configuration
    FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

    // set master brightness control
    FastLED.setBrightness(BRIGHTNESS);

    // BLE Config
    Serial.begin(9600);
    Serial.println("Starting BLE ...");

    BLEDevice::init("LampSKI");
    BLEServer *pServer = BLEDevice::createServer();

    BLEService *pService = pServer->createService(SERVICE_UUID);

    BLECharacteristic *pCharacteristicSpeed = pService->createCharacteristic(
        CHARACTERISTIC_SPEED,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE);
    pCharacteristicSpeed->setCallbacks(new LampSkiSetSpeed());

    BLECharacteristic *pCharacteristicBrightness = pService->createCharacteristic(
        CHARACTERISTIC_BRIGHTNESS,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE);
    pCharacteristicBrightness->setCallbacks(new LampSkiSetBrightness());

    BLECharacteristic *pCharacteristicMode = pService->createCharacteristic(
        CHARACTERISTIC_MODE,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE);
    pCharacteristicMode->setCallbacks(new LampSkiSetMode());

    pService->start();

    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->start();

    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
}

void loop()
{
    // Call the current pattern function once, updating the 'leds' array
    gPatterns[gCurrentPatternNumber]();
    FastLED.setBrightness(gBrighness);

    // send the 'leds' array out to the actual LED strip
    FastLED.show();
    // insert a delay to keep the framerate modest
    FastLED.delay(1000 /  gSpeed); //FRAMES_PER_SECOND);

    // do some periodic updates
    EVERY_N_MILLISECONDS(50) { gHue++; }   // slowly cycle the "base color" through the rainbow
    
    //EVERY_N_SECONDS(10) { nextPattern(); } // change patterns periodically
}




// LED MODES
//----------------------------------------
void rainbow()
{
    // FastLED's built-in rainbow generator
    fill_rainbow(leds, NUM_LEDS, gHue, 7);
}

void addGlitter(fract8 chanceOfGlitter)
{
    if (random8() < chanceOfGlitter)
    {
        leds[random16(NUM_LEDS)] += CRGB::White;
    }
}

void rainbowWithGlitter()
{
    // built-in FastLED rainbow, plus some random sparkly glitter
    rainbow();
    addGlitter(80);
}

void confetti()
{
    // random colored speckles that blink in and fade smoothly
    fadeToBlackBy(leds, NUM_LEDS, 10);
    int pos = random16(NUM_LEDS);
    leds[pos] += CHSV(gHue + random8(64), 200, 255);
}

void sinelon()
{
    // a colored dot sweeping back and forth, with fading trails
    fadeToBlackBy(leds, NUM_LEDS, 20);
    int pos = beatsin16(13, 0, NUM_LEDS - 1);
    leds[pos] += CHSV(gHue, 255, 192);
}

void bpm()
{
    // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
    uint8_t BeatsPerMinute = 62;
    CRGBPalette16 palette = PartyColors_p;
    uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
    for (int i = 0; i < NUM_LEDS; i++)
    { //9948
        leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
    }
}

void juggle()
{
    // eight colored dots, weaving in and out of sync with each other
    fadeToBlackBy(leds, NUM_LEDS, 20);
    byte dothue = 0;
    for (int i = 0; i < 8; i++)
    {
        leds[beatsin16(i + 7, 0, NUM_LEDS - 1)] |= CHSV(dothue, 200, 255);
        dothue += 32;
    }
}

void snow()
{
    addGlitter(100);
    fadeToBlackBy(leds, NUM_LEDS, 6);
}

void raindrops()
{ // Random colored speckles that blink in and fade smoothly.

    fadeToBlackBy(leds, NUM_LEDS, 40);

    int pos = random16(NUM_LEDS);
    if (drop_state > 4)
    {
        drop_state = 0;
        drop_pos = pos;
        drop_hue = gHue + random8(64);
    }

    switch (drop_state)
    {
    case 0:
        leds[drop_pos] += CHSV(drop_hue, 200, 150);
        drop_state++;
        return;
    case 1:
        leds[drop_pos] += CHSV(drop_hue, 200, 50);
        drop_state++;
        return;
    case 2:
        leds[drop_pos - 1] += CHSV(drop_hue, 200, 150);
        leds[drop_pos + 1] += CHSV(drop_hue, 200, 150);

        drop_state++;
        return;
    case 3:
        fadeToBlackBy(leds, NUM_LEDS, 40);
        leds[drop_pos - 2] += CHSV(drop_hue, 200, 150);
        leds[drop_pos + 2] += CHSV(drop_hue, 200, 150);
        drop_state++;
        return;
    case 4:
        fadeToBlackBy(leds, NUM_LEDS, 40);
        leds[drop_pos - 3] += CHSV(drop_hue, 200, 150);
        leds[drop_pos + 3] += CHSV(drop_hue, 200, 150);
        drop_state++;
        return;
    }
}


// ADJUSTMENTS
//--------------------------
void nextPattern()
{
    // add one to the current pattern number, and wrap around at the end
    gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);
}

void setBrightness(long value)
{
    // add one to the current pattern number, and wrap around at the end
    if (0 < value && value <= 100){
        gBrighness = value;
        Serial.println("brightness set to: " + (String)value);
    }
    else
        Serial.println("brightness value out of range: " + (String)value);
}

void setSpeed(long value)
{
    if (0 < value && value <= 100){
        gSpeed = value;
        Serial.println("speed set to: " + (String)value);
    }
    else
        Serial.println("speed value out of range: " + (String)value);
}

void setMode(long value)
{
    if (0 <= value && value <= ARRAY_SIZE(gPatterns)-1)
    {
        gCurrentPatternNumber = value;
        Serial.println("mode set to: " + (String)value);
    }
    else
        Serial.println("mode out of range: " + (String)value);
}