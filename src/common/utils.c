#include "utils.h"
#include "avdc.h"

// timer

int16_t timer_start;
int16_t timer_offset;

uint16_t timer_ms() {
	uint8_t s = CTC_SECONDS;
	uint8_t s100 = CTC_HUNDREDS;
	uint8_t cs10 = CTC_TENTHS_CS;
	uint8_t s100_check = CTC_HUNDREDS;
	uint8_t s_check = CTC_SECONDS;
	uint16_t ms;
	if (s100 == s100_check) {
		ms = bcd2bin(s100) * 10 + bcd2bin(cs10 >> 4);
	} else {
		ms = bcd2bin(s100_check) * 10;
	}
	if (s == s_check) {
		return bcd2bin(s) * 1000 + ms;
	} else {
		return bcd2bin(s_check) * 1000;
	}
}

int16_t timer() {
	uint8_t seconds = CTC_SECONDS;
	uint8_t hundreds = CTC_HUNDREDS;
	uint8_t seconds_check = CTC_SECONDS;
	if (seconds == seconds_check) {
		return bcd2bin(seconds) * 100 + bcd2bin(hundreds);
	} else {
		return bcd2bin(seconds_check) * 100;
	}
}

void timer_reset(int16_t offset) {
	timer_start = timer();
	timer_offset = offset;
}

int16_t timer_diff() {
	int16_t now = timer();
	if (now >= timer_start) { return now - (timer_start + timer_offset); }
	return (now + 6000) - (timer_start + timer_offset); 
}

int16_t timer_diff_ex(uint16_t timer_start, uint16_t timer_offset) {
	int16_t now = timer();
	if (now >= timer_start) { return now - (timer_start + timer_offset); }
	return (now + 6000) - (timer_start + timer_offset); 
}

// keyboard

void kbd_wait_ready() {
	uint8_t status = 0;
	while ((status & KBD_STATUS_READY) == 0) {
		status = KBD_STATUS;
	}
}

void kbd_beep(bool long_beep) {
	kbd_wait_ready();
	KBD_CMD = long_beep ? KBD_CMD_BEEP_LONG : KBD_CMD_BEEP;
}

// other

bool sys_is_emu() {
	static int _idp_is_emu = -1;
	if (_idp_is_emu == -1) {
		_idp_is_emu = avdc_get_pointer(0, 0) == 0xFFFF;
	}
	return _idp_is_emu;
}

int sys_rand() {
	return rand() / 10; // WARNME: this should be removed when rand is fixed
}