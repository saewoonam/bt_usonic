/*
 * timing.h
 *
 *  Created on: Aug 26, 2020
 *      Author: nams
 */

#ifndef CRYPTO_AND_TIME_TIMING_H_
#define CRYPTO_AND_TIME_TIMING_H_

#include "bg_types.h"
#include "stdbool.h"


#define TICKS_PER_SECOND 32768

#define ENCOUNTER_PERIOD	60000
struct my_time_struct
{
	uint32_t time_overflow;
	uint32_t offset_time;
	uint32_t offset_overflow;
	uint32_t next_minute;
	uint32_t epochtimesync;
	uint32_t near_hotspot_time;
	bool gottime;
};

extern struct my_time_struct _time_info;

uint32_t ts_ms();
void wait(int wait);
uint32_t k_uptime_get();
void update_next_minute(void);
void setup_next_minute(void);
uint32_t em(uint32_t t);
void sync_clock(uint32_t ts, uint32_t *timedata);

#endif /* CRYPTO_AND_TIME_TIMING_H_ */
