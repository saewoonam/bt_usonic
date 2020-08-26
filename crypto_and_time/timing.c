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

extern bool write_flash;
void flash_store(void);


#define TICKS_PER_SECOND 32768
uint32_t time_overflow=0;   // current time overflow
uint32_t offset_time = 0;
uint32_t offset_overflow = 0;  // set of offset has overflowed
uint32_t next_minute = 60000;
uint32_t epochtimesync = 0;
struct my_time_struct _time_info = {
		.time_overflow=0,
		.offset_time = 0,
		.offset_overflow = 0,
		.next_minute = 60000,
		.epochtimesync=0,
		.gottime=false};

uint32_t ts_ms() {
	/*  Need to handle overflow... of t_ms... 36hrs*32... perhaps every battery change
	 *
	 */
	static uint32_t old_ts = 0;
	uint32_t ts = RTCC_CounterGet();
	uint32_t t = ts/TICKS_PER_SECOND;
	uint32_t fraction = ts % TICKS_PER_SECOND;
	uint32_t t_ms = t*1000 + (1000*fraction)/TICKS_PER_SECOND;
    if (old_ts > ts) {
    	time_overflow++;
    }
    t_ms += (1000*time_overflow) << 17; // RTCC overflows after 1<<17 seconds
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
	return ((timestamp-offset_time) / 1000 + epochtimesync)/(60*60*24);
}

void update_next_minute(void) {
	static uint32_t day = 0;
	next_minute +=60000;
	//  Change key daily... for now.
	if (day==0) {
		day = epoch_day();
	}
	uint32_t current_day = epoch_day();
	if (current_day>day) {
		update_public_key();
		newRandAddress();
		day = current_day;
	}
	if (write_flash) flash_store();
}

void setup_next_minute(void) {
	printLog("setup next_minute\r\n");
    uint32_t timestamp = ts_ms();
    uint32_t epoch_minute_origin = (epochtimesync)/60;
    uint32_t extra_seconds = epochtimesync - 60*epoch_minute_origin;
    next_minute = offset_time - extra_seconds * TICKS_PER_SECOND + 60000;
    printLog("offset_time, timestamp, next minute: %ld, %ld, %ld\r\n", offset_time, timestamp, next_minute);
    /*
    while (next_minute < timestamp) {
    	next_minute += 60000;
    }
    */
}
