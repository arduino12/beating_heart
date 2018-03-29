/*
 * Connect hartbeat analog sensor to arduino pin A7.
 * Tools -> Serial Plotter (better than Serial Monitor for this demo).
 * Put your finger and after ~5 seconds you should see your hartbeat!
 * Arad Eizen 25/03/18.
 */

#define HARTBEAT_PIN				(7)
#define SERIAL_BAUDRATE				(115200)

#define HARTBEAT_MIN_RAW_READ		(400)
#define HARTBEAT_MAX_RAW_READ		(600)
#define HARTBEAT_MIN_BPM			(40)
#define HARTBEAT_MAX_BPM			(250)
#define HARTBEAT_SAMPLE_MS			(5)
#define HARTBEAT_CALIBRATION_MS		(5000)

#define HARTBEAT_NONE				(0)
#define HARTBEAT_IDLE				(1)
#define HARTBEAT_BEAT				(2)


uint8_t raw_sampels[60000 / HARTBEAT_SAMPLE_MS / HARTBEAT_MIN_BPM];
int8_t last_raw = 0;

uint32_t min_next_hartbeat_ms;
uint32_t hartbeat_calibration_ms;

uint8_t get_hartbeat()
{
	uint16_t raw_read = analogRead(HARTBEAT_PIN);

	if (raw_read > HARTBEAT_MAX_RAW_READ || raw_read < HARTBEAT_MIN_RAW_READ)
		return HARTBEAT_NONE;

	int8_t raw = (raw_read - HARTBEAT_MIN_RAW_READ) / 2;
	int8_t raw_diff = last_raw - raw;

	if (raw_diff < 0)
		raw_diff = -raw_diff;

	last_raw = raw;

	if (raw_diff > 2)
		return HARTBEAT_NONE;

	int8_t max_raw = raw;
	int8_t min_raw = raw;
	for (uint16_t i = sizeof(raw_sampels) - 1; i > 0; i--) {
		raw_sampels[i] = raw_sampels[i - 1];
		if (max_raw < raw_sampels[i])
			max_raw = raw_sampels[i];
		if (min_raw > raw_sampels[i])
			min_raw = raw_sampels[i];
		
	}
	raw_sampels[0] = raw;

	int8_t half_raw = (3 * max_raw + min_raw) / 4;
	// int8_t half_raw = (max_raw + min_raw) / 2;

	return raw > half_raw ? HARTBEAT_BEAT : HARTBEAT_IDLE;
}

uint8_t update_hartbeat()
{
	uint8_t hartbeat = get_hartbeat();
	uint32_t cur_ms = millis();

	if (hartbeat == HARTBEAT_NONE) {
		hartbeat_calibration_ms = cur_ms + HARTBEAT_CALIBRATION_MS;
		digitalWrite(LED_BUILTIN, false);
		return HARTBEAT_NONE;
	}

	if (cur_ms < hartbeat_calibration_ms) {
		return HARTBEAT_NONE;
	}

	digitalWrite(LED_BUILTIN, hartbeat == HARTBEAT_BEAT);
	
	if (hartbeat == HARTBEAT_BEAT) {
		if (cur_ms > min_next_hartbeat_ms) {
			min_next_hartbeat_ms = cur_ms + 60000 / HARTBEAT_MAX_BPM;
			return HARTBEAT_BEAT;
		}
	}
	return HARTBEAT_IDLE;
}

void setup()
{
	Serial.begin(SERIAL_BAUDRATE);
	pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
	uint8_t cur_hartbeat = update_hartbeat();

	Serial.println(cur_hartbeat);

	delay(HARTBEAT_SAMPLE_MS);
}
