/*
 * Detects a heart rate and activates the pulse motor and leds accordingly.
 * Arad Eizen 26/04/18.
 */
#include <Wire.h>
#include <FastLED.h>
#include <MAX30100_PulseOximeter.h>

/* get array items count */
#define ARRAY_SIZE(x)				(sizeof(x) / sizeof(*x))
/* uart baud rate in bit per seconds */
#define SERIAL_BAUD_RATE			(115200)
/* set uart read timeout in milliseconds */
#define SERIAL_TIMEOUT				(1)
/* not used but stil connected */
#define MAX30100_INT_PIN			(2)
/* smart leds din pin */
#define LED_DATA_PIN				(3)
/* goes to FET that drive the motor when high */
#define PULSE_MOTOR_PIN				(12)
/* total amount of rgb channels */
#define RGB_COUNT					(3)
/* led strip type */
#define LED_CHIPSET					WS2812B
/* led strip color rgb channels order */
#define LED_COLOR_ORDER				(GRB)
/* max brightness for rgb channel */
#define LED_BRIGHTNESS				(64)
/* total amount of rgb leds */
#define LED_COUNT					(8)
/* periodic debug print time */
#define DEBUG_UPDATE_MS				(4000)

PulseOximeter pox;
CRGBArray<LED_COUNT> leds;

uint32_t debug_last_update;

struct {
	uint8_t motor_state = 0;
	uint8_t motor_pulses[4] = {150, 300, 150, 0};
	uint32_t last_motor_update;
	uint8_t led_state = 0;
	uint8_t led_pulses[6] = {80, 150, 70, 0};
	uint32_t last_led_update;
} beat_effect;


void set_pulse_motor(bool is_on)
{
	digitalWrite(PULSE_MOTOR_PIN, is_on);
}

void set_leds(CRGB c)
{
	leds.fill_solid(c);
	FastLED.show();
}

void beat_effect_start()
{
	uint32_t cur_ms = millis();

	set_pulse_motor(true);
	beat_effect.motor_state = 1;
	beat_effect.last_motor_update = cur_ms + beat_effect.motor_pulses[0];

	set_leds(CRGB::Red);
	beat_effect.led_state = 1;
	beat_effect.last_led_update = cur_ms + beat_effect.led_pulses[0];
}

void handle_beat()
{
	Serial.println("beat!");
	beat_effect_start();
}

void setup()
{
	/* initialize uart for serial communication */
	Serial.begin(SERIAL_BAUD_RATE);
	Serial.setTimeout(SERIAL_TIMEOUT);
	delay(100);

	/* initialize pulse motor */
	pinMode(PULSE_MOTOR_PIN, OUTPUT);
	set_pulse_motor(false);

	/* initialize smart leds */
	FastLED.addLeds<LED_CHIPSET, LED_DATA_PIN, LED_COLOR_ORDER>(leds, LED_COUNT).setCorrection(TypicalLEDStrip);
	FastLED.setBrightness(LED_BRIGHTNESS);
	FastLED.show();

	/* initialize heartbeat sensor and register a beat-detected callback */
	pox.begin();
	pox.setOnBeatDetectedCallback(handle_beat);

	Serial.println("heart v1!");
}

void loop()
{
	uint32_t cur_ms = millis();
	pox.update();

	if (debug_last_update < cur_ms) {
		debug_last_update = cur_ms + DEBUG_UPDATE_MS;
		Serial.print(F("heart rate: ")); Serial.print(pox.getHeartRate()); Serial.println(" bpm");
		Serial.print(F("oxidation (spO2): ")); Serial.print(pox.getSpO2()); Serial.println('%');
		// Serial.print(F("temp: ")); Serial.print(pox.getTemperature()); Serial.println('C');
	}

	if (beat_effect.motor_state) {
		if (beat_effect.last_motor_update < cur_ms) {
			if (beat_effect.motor_state++ >= ARRAY_SIZE(beat_effect.motor_pulses)) {
				beat_effect.motor_state = 0;
				set_pulse_motor(0);
			}
			else {
				beat_effect.last_motor_update = cur_ms + beat_effect.motor_pulses[beat_effect.motor_state - 2];
				set_pulse_motor(beat_effect.motor_state & 1);
			}
		}
	}

	if (beat_effect.led_state) {
		if (beat_effect.last_led_update < cur_ms) {
			if (beat_effect.led_state++ >= ARRAY_SIZE(beat_effect.led_pulses)) {
				beat_effect.led_state = 0;
				set_leds(CRGB::Black);
			}
			else {
				beat_effect.last_led_update = cur_ms + beat_effect.led_pulses[beat_effect.led_state - 2];
				set_leds(beat_effect.led_state & 1 ? CRGB::Black : CRGB::Red);
			}
		}
	}
}
