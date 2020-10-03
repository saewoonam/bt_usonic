/*
 * timing.c
 *
 *  Created on: Aug 26, 2020
 *      Author: nams
 */

#include "bg_types.h"
#include "timing.h"

#include "em_rtcc.h"
#include "app.h"

#include "crypto_and_time/crypto.h"

extern bool write_flash;
void flash_store(void);
void set_new_mac_address(void);


#define TICKS_PER_SECOND 32768

struct my_time_struct _time_info = {
		.time_overflow=0,
		.offset_time = 0,
		.offset_overflow = 0,
		.next_minute = ENCOUNTER_PERIOD,
		.epochtimesync=0,
		.gottime=false};

uint32_t ts_ms() {
	/*  Need to handle overflow... of ts_ms... 36hrs*32... perhaps every battery change
	 *
	 */
	static uint32_t old_ts = 0;
	uint32_t ts = RTCC_CounterGet();
	uint32_t t = ts/TICKS_PER_SECOND;
	uint32_t fraction = ts % TICKS_PER_SECOND;
	uint32_t t_ms = t*1000 + (1000*fraction)/TICKS_PER_SECOND;
    if (old_ts > ts) {
    	_time_info.time_overflow++;
    }
    t_ms += (1000*_time_info.time_overflow) << 17; // RTCC overflows after 1<<17 seconds
    old_ts = ts;

	return t_ms;
}

void wait(int wait) {
	printLog("Start wait %d\r\n", wait);
	uint32_t start = ts_ms();
	while((ts_ms() - start)<wait) {
		;
	}
	printLog("Done wait %d\r\n", wait);

}

uint32_t k_uptime_get() {
	return ts_ms();
}

uint32_t epoch_day(void) {
	uint32_t timestamp = ts_ms();
	return ((timestamp-_time_info.offset_time) / 1000 + _time_info.epochtimesync)/(60*60*24);
}

void update_next_minute(void) {
	static uint32_t day = 0;
	while (ts_ms() > _time_info.next_minute) {
		printLog("update_next_minute: ");
		_time_info.next_minute +=ENCOUNTER_PERIOD;
	}
	set_new_mac_address();

	//  Change stuff daily... if desired.
	if (day==0) {
		day = epoch_day();
	}
	uint32_t current_day = epoch_day();
	if (current_day>day) {
		// update_public_key();
		day = current_day;
	}
	if (write_flash) flash_store();
}

void setup_next_minute(void) {
	printLog("setup next_minute\r\n");
    uint32_t timestamp = ts_ms();
    uint32_t epoch_minute_origin = (_time_info.epochtimesync)/60;
    uint32_t extra_seconds =_time_info.epochtimesync - 60*epoch_minute_origin;
    _time_info.next_minute = _time_info.offset_time - extra_seconds * TICKS_PER_SECOND + ENCOUNTER_PERIOD;
    printLog("offset_time, timestamp, next minute: %ld, %ld, %ld\r\n",
    		_time_info.offset_time, timestamp, _time_info.next_minute);
    /*
    while (next_minute < timestamp) {
    	next_minute += ENCOUNTER_PERIOD;
    }
    */
}

uint32_t em(uint32_t t) {
	return ((t-_time_info.offset_time) / 1000 + _time_info.epochtimesync)/60;
}

void sync_clock(uint32_t ts, uint32_t *timedata) {
	_time_info.epochtimesync = timedata[0]; // units are sec
	_time_info.offset_time = timedata[1];  // units are ms
	_time_info.offset_overflow = timedata[2];  // units are integers
	//printLog("timedata: %ld, %ld, %ld\r\n", timedata[0], timedata[1], timedata[2]);
	uint32_t epoch_minute_origin = (_time_info.epochtimesync) / 60;
	uint32_t extra_seconds = _time_info.epochtimesync % 60;
	_time_info.next_minute = _time_info.offset_time - extra_seconds * 1000
			+ ENCOUNTER_PERIOD;
	uint32_t dt = (ts - _time_info.offset_time) / 1000;
	uint32_t epoch_minute = (dt + _time_info.epochtimesync) / 60;
	printLog("em(ts), em(next_minute) %ld, %ld\r\n", em(ts),
			em(_time_info.next_minute + 1000));

	printLog("offset_time, timestamp, next minute: %ld, %ld, %ld %ld\r\n",
			_time_info.offset_time, ts, _time_info.next_minute, extra_seconds);

	printLog("epochtimes, e_min, e_min_origin: %ld %ld %ld\r\n",
			_time_info.epochtimesync, epoch_minute, epoch_minute_origin);
}
