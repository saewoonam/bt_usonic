/*************************************************************************************************
 * \file   spp_client_main.c
 * \brief  SPP client example
 *
 *
 ***************************************************************************************************
 * <b> (C) Copyright 2016 Silicon Labs, http://www.silabs.com</b>
 ***************************************************************************************************
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 **************************************************************************************************/

/* Board headers */
#include <crypto_and_time/monocypher.h>
#include <crypto_and_time/x25519-cortex-m4.h>
#include "ble-configuration.h"
#include "board_features.h"

/* Bluetooth stack headers */
#include "bg_types.h"
#include "native_gecko.h"
#include "gatt_db.h"

#include <stdio.h>
#include "retargetserial.h"
#include "sleep.h"
#include "spp_utils.h"
#include "em_usart.h"
#include "em_rtcc.h"
#include "em_prs.h"

#include "spiflash/btl_storage.h"

#include "encounter/encounter.h"
#include "crypto_and_time/crypto.h"
#include "crypto_and_time/timing.h"
#include "ota.h"

/***************************************************************************************************
 Local Macros and Definitions
 **************************************************************************************************/

#define DISCONNECTED	0
#define SCAN_ADV		1
#define FIND_SERVICE	2
#define SEND_0			3
#define FIND_SPP		4
#define ENABLE_NOTIF 	5
#define DATA_MODE		6
#define DISCONNECTING 	7
#define STATE_CONNECTED 8
#define STATE_SPP_MODE 	9
#define ADV_ONLY		10
#define FIND_UPLOAD		11
#define UPLOAD_DATA 	12
#define FINISHED_UPLOAD 	13
#define TIME_SYNC	 	14
#define ROLE_UNKNOWN 		-1
#define ROLE_CLIENT_MASTER 	0
#define ROLE_SERVER_SLAVE 	1
#define ROLE_CLIENT_MASTER_UPLOAD 	2

#define CLIENT_IS_BTDEV		0
#define CLIENT_IS_COMPUTER	1

#define LOG_MARK (1<<2)
#define LOG_UNMARK (1<<3)
#define LOG_RESET (1<<4)

#define RSSI_LIST_SIZE (12)

#define K_OFFSET	276
#define K_OFFSET2	428

const char *version_str = "Version: " __DATE__ " " __TIME__;
const char *ota_version = "2.0";


// SPP service UUID: 4880c12c-fdcb-4077-8920-a450d7f9b907

//const uint8 serviceUUID[16] = { 0x07, 0xb9, 0xf9, 0xd7, 0x50, 0xa4, 0x20, 0x89,
//		0x77, 0x40, 0xcb, 0xfd, 0x2c, 0xc1, 0x80, 0x48 };

// encounter service UUID: 7b183224-9168-443e-a927-7aeea07e8105
const uint8 serviceUUID[16] = { 0x05, 0x81, 0x7E, 0xA0, 0xEE, 0x7A, 0x27, 0xA9,
		0x3E, 0x44, 0x68, 0x91, 0x24, 0x32, 0x18, 0x7B };

const uint8 service_upload_UUID[2] = { 0x19, 0xC0 };

// SPP data UUID: fec26ec4-6d71-4442-9f81-55bc21d658d6
const uint8 charUUID[16] = { 0xd6, 0x58, 0xd6, 0x21, 0xbc, 0x55, 0x81, 0x9f,
		0x42, 0x44, 0x71, 0x6d, 0xc4, 0x6e, 0xc2, 0xfe };

// rw data UUID: 56cd7757-5f47-4dcd-a787-07d648956068
const uint8 charUUID_rw[16] = { 0x68, 0x60, 0x95, 0x48, 0xd6, 0x07, 0x87, 0xa7,
		0xcd, 0x4d, 0x47, 0x5f, 0x57, 0x77, 0xcd, 0x56 };


const uint8 char_upload_UUID[2] = {0x0E, 0xEC};
const uint8 char_time_UUID[2] = {0x0F, 0xEC};

/* soft timer handles */
#define RESTART_TIMER    1
#define HEARTBEAT_TIMER    2
#define GPIO_POLL_TIMER  3

static bool reset_needed = false;

static const char master_string[] = "Master";
static const char slave_string[] = "Slave";

int16_t left_t[32];
int16_t right_t[32];
int _data_idx = 0;

/* maximum number of iterations when polling UART RX data before sending data over BLE connection
 * set value to 0 to disable optimization -> minimum latency but may decrease throughput */
#define UART_POLL_TIMEOUT  5000

// void initPDM(void);
void pdm_start(void);
void pdm_pause(void);

void populateBuffers(int k_value);
void startDMADRV_TMR(void);

void update_public_key(void);
void set_new_mac_address(void);
void print_mac(uint8_t *addr);

void setup_encounter_record(uint8_t* mac_addr);
void process_ext_signals(uint32_t signals);
void process_server_spp_from_computer(struct gecko_cmd_packet* evt, bool sending_ota, bool sending_turbo);

void start_writing_flash();
// void store_event(uint8_t *event);
void flash_erase();
void determine_counts(uint32_t flash_size);
void flash_erase();
void read_name_ps(void);
void set_name(uint8_t *name);
void read_encounters_tracking(void);

void readBatteryLevel(void);

void timer0_prescale(int prescale);
void play_speaker(void);

// int32_t calc_k_from_mac(uint8_t *mac);

bool pdm_on = false;

int32_t k_goertzel = 276;
int32_t k_speaker = 276;
uint8_t k_speaker_offsets[12];
uint8_t k_goertzel_offsets[12];

float pulse_width = 1.0e-3;
int out=0;
bool distant=false;

void setup_k() {
	bd_addr local_addr;
	local_addr = gecko_cmd_system_get_bt_address()->address;
	if (local_addr.addr[0]>0x80) {
		k_goertzel = 392;
		k_speaker = 392;
	} else {
		k_goertzel = 392;
		k_speaker = 392;
	}

}
/***************************************************************************************************
 Global Variables
 **************************************************************************************************/
struct my_encounter_index _encounters_tracker;
// encounter related
uint32_t encounter_count = 0;
Encounter_record_v2 encounters[IDX_MASK+1];
uint32_t c_fifo_last_idx=0;
uint32_t p_fifo_last_idx=0;

// storage related
bool write_flash = false;
bool mark_set = false;

// mic and spkr related
bool update_k_goertzel=true;
bool update_k_speaker=true;

// bt connection
uint8 _conn_handle = 0xFF;
uint8 _max_packet_size = 20;
uint8 _min_packet_size = 20;  // Target minimum bytes for one packet
uint8_t local_mac[6];

uint8_t battBatteryLevel = 0;

extern uint8_t public_key[32];

bool listen_usb = true;
/***************************************************************************************************
 Local Variables
 **************************************************************************************************/
//bt connection state
static int _main_state;
static uint32 _service_handle;
static uint16 _char_handle;
static uint16 _char_upload_handle;
static uint16 _char_time_handle;
static uint16 _char_rw_handle;

static int8 _role = ROLE_UNKNOWN;
uint8_t local_mac[6];
/* Default maximum packet size is 20 bytes. This is adjusted after connection is opened based
 * on the connection parameters */

volatile int conn_interval = 64; //64
Encounter_record_v2 *current_encounter = encounters;
static int8  _rssi_count = 0;

static int bad_heartbeats = 0;
static int8 _client_type = -1;
static bool sending_ota = false;
uint8 _status = 1<<2;  // start with clock not set bit
static bool _update_minute_after_upload = false;
static void reset_variables() {
	_conn_handle = 0xFF;
	_main_state = DISCONNECTED;
	_service_handle = 0;
	_char_handle = 0;
	_char_upload_handle = 0;
	_char_time_handle = 0;
	_char_rw_handle = 0;
	_role = ROLE_UNKNOWN;
	_client_type = -1;

	_max_packet_size = 20;
	_min_packet_size = 20;

	_data_idx = 0;
	memset(left_t, 0, 32*2);
	memset(right_t, 0, 32*2);
	_rssi_count = 0;
	sharedCount = 0;
	bad_heartbeats = 0;  // not sure this is the best thing,  doesn't handle 2 connections right
}

void get_local_mac(void);
int process_scan_response(
		struct gecko_msg_le_gap_scan_response_evt_t *pResp, uint8_t *status);
void start_bt(void);
void setup_adv(void);
void set_adv_params(uint16_t *params);

void record_tof(Encounter_record_v2 *current_encounter);


void update_counts_status(void) {
	uint8_t temp[4];
	memcpy(temp, (uint8_t *) &encounter_count, 4);
	temp[3] = _status;
	gecko_cmd_gatt_server_write_attribute_value(gattdb_count, 0, 4,
			temp);
}

static void send_rw_client(char cmd) {
	uint8 len = 1;
	uint16 result;

	// Stack may return "out-of-memory" error if the local buffer is full -> in that case, just keep trying until the command succeeds
	do {
		//result = gecko_cmd_gatt_write_characteristic_value_without_response(
		result = gecko_cmd_gatt_write_characteristic_value(
				_conn_handle, _char_rw_handle, len, (uint8_t *) &cmd)->result;
	} while (result == bg_err_out_of_memory);

	if (result != 0) {
		printLog("Unexpected error try to send char_rw:%c 0x%x\r\n", cmd, result);
	}
	// printLog("Sent rw command: %c\r\n", cmd);
}

void unlinkPRS();

static void send_spp_data_client() {
	uint8 len = 1;
	uint16 result;
	uint8_t temp[81];

	if (sharedCount==0) {
		// printLog("********send PUBLIC KEY from client\r\n");
		memcpy(temp+1, public_key, 32);
		len = 33;
	} else if (sharedCount==20) {
		unlinkPRS();
		// printLog("send spp data client, got %d\r\n", sharedCount);
		// printLog("*******send usound _data_idx %d\r\n", _data_idx);
		record_tof(current_encounter);
		temp[1] = current_encounter->usound.n;
		uint16_t *usound_data = (uint16_t *) (temp+2);
		usound_data[0] = current_encounter->usound.left;
		usound_data[1] = current_encounter->usound.left_irq;
		usound_data[2] = current_encounter->usound.right;
		usound_data[3] = current_encounter->usound.right_irq;
		len = 10;
	}

	sharedCount++;

	temp[0] = sharedCount;
	// Stack may return "out-of-memory" error if the local buffer is full -> in that case, just keep trying until the command succeeds
	do {
		result = gecko_cmd_gatt_write_characteristic_value_without_response(
				_conn_handle, _char_handle, len, temp)->result;
	} while (result == bg_err_out_of_memory);

	if (result != 0) {
		printLog("Unexpected error sending shared count: %x\r\n", result);
	}

}

static void send_upload_data() {
	uint16 result;
	// int len = 192;
	// static uint32_t index = 0;
	uint8_t temp[196];
	bool done = false;
//	for (int i=0; i<192; i++) {
//		temp[i] = i;
//	}
	uint8_t chunk_size = 192;
	if ((encounter_count - _encounters_tracker.start_upload) > 6) {
		chunk_size = 192;
	} else {
		chunk_size = (encounter_count - _encounters_tracker.start_upload) << 5;
		done = true;
	}
	memcpy(temp, &(_encounters_tracker.start_upload), 4);
	if (chunk_size>0) {
		// printLog("Send start: %lu\r\n", _encounters_tracker.start_upload);
		// figure out actual size of the data to send... usually 192 except at the end
		uint8 xfer_len = chunk_size;
		// Figure out starting point in memory
		uint32_t addr = (_encounters_tracker.start_upload) << 5;
		// get data from storage
		//int32_t retCode =
		storage_readRaw(addr, temp + 4, xfer_len);
		xfer_len += 4;  // Add length of packet number
		//send over bluetooth
		do {
			result = gecko_cmd_gatt_write_characteristic_value(_conn_handle,
					_char_upload_handle, xfer_len, temp)->result;
		} while (result == bg_err_out_of_memory);
		_encounters_tracker.start_upload += (chunk_size) >> 5;
	}
	if (done) {
		_main_state = FINISHED_UPLOAD;
		printLog("Upload finished start_upload: %lu\r\n", _encounters_tracker.start_upload);
	}

}

// scratch work
//
//    	printLog("Trying to upload counter: %lu\r\n", counter);
//    	// gecko_cmd_gatt_read_characteristic_value(_conn_handle, _char_upload_handle);
//		do {
//
//			// without response does nothing with computer...
//			result = gecko_cmd_gatt_write_characteristic_value(
//					_conn_handle, _char_upload_handle, len, temp)->result;
//		} while (result == bg_err_out_of_memory);
//		counter++;
//		if (result != 0) {
//			printLog("Unexpected error sending upload count: 0x%x\r\n", result);
//		}



static void send_spp_data() {
	uint8 len = 1;
	uint16 result;
	uint8_t temp[33];
	// printLog("send spp data, got %d\r\n", sharedCount);
	if (sharedCount==1) {
		// printLog("Got the first packet from the client, sending: %d\r\n", sharedCount);
		memcpy(temp+1, public_key, 32);
		len = 33;
	}
	sharedCount++;
	temp[0] = sharedCount;
		// Stack may return "out-of-memory" error if the local buffer is full -> in that case, just keep trying until the command succeeds
		do {
			result = gecko_cmd_gatt_server_send_characteristic_notification(
					_conn_handle,
					gattdb_gatt_spp_data, len, temp)->result;
		} while (result == bg_err_out_of_memory);

		if (result != 0) {
			printLog("Unexpected error trying to send shared count: %x\r\n", result);
		}
}

#define LINKPRS
void linkPRS() {
#ifdef LINKPRS
	if (_role == ROLE_SERVER_SLAVE) {
		// setup speaker
		PRS_SourceAsyncSignalSet(TX_OBS_PRS_CHANNEL,
								 PRS_ASYNC_CH_CTRL_SOURCESEL_MODEM,
								 _PRS_ASYNC_CH_CTRL_SIGSEL_MODEMFRAMESENT);
	} else {
		// setup microphone
		PRS_SourceAsyncSignalSet(RX_OBS_PRS_CHANNEL,
								 PRS_ASYNC_CH_CTRL_SOURCESEL_MODEML,
								 _PRS_ASYNC_CH_CTRL_SIGSEL_MODEMLFRAMEDET);
		pdm_start();
		pdm_on = true;
	}
#endif
}

void unlinkPRS() {
	PRS_SourceAsyncSignalSet(RX_OBS_PRS_CHANNEL,
			PRS_ASYNC_CH_CTRL_SOURCESEL_NONE,
			_PRS_ASYNC_CH_CTRL_SIGSEL_NONE);
	PRS_SourceAsyncSignalSet(TX_OBS_PRS_CHANNEL,
			PRS_ASYNC_CH_CTRL_SOURCESEL_NONE,
			_PRS_ASYNC_CH_CTRL_SIGSEL_NONE);
}

/**
 * @brief  SPP client mode main loop
 */
void read_serial(uint8_t *buf, int len) {
	int c;
	for(int i=0; i<len;) {
		c = RETARGET_ReadChar();
		if (c>=0) {
			buf[i++] = c;
		}
	}
}

void parse_bt_command(uint8_t c) {
	switch (c) {
	case 'f':{
		printLog("mode: send data ota\r\n");
        sending_ota = true;
        break;
	}
	case 'F':{
		printLog("mode: Set data ota false\r\n");
        sending_ota = false;
        break;
	}
	case 't':{
		printLog("mode: send data turbo\r\n");
		// SLEEP_SleepBlockBegin(sleepEM2); // Disable sleeping
		// sending_turbo = true;
		send_data();  // over bluetooth
		// SLEEP_SleepBlockEnd(sleepEM2); // Enable sleeping
        break;
	}
	case 'T':{
		printLog("mode: place holder for end send turbo\r\n");
		// sending_turbo = false;
        break;
	}
	case 'u':{
		printLog("send mtu\r\n");
		send_ota_uint8(_min_packet_size);
        break;
	}
 	case 'E':{
		// mode = MODE_ENCOUNTER;
		write_flash = false;
		_status &= ~(1 << 3); // clear data collection mode
		update_counts_status();
		printLog("Set to mode to ENCOUNTER\r\n");
		break;
	}
	case 'R': {
		// mode = MODE_RAW;
		write_flash = false;
		_status |= (1 << 3); // set data collection mode to raw
		update_counts_status();
		printLog("Set to mode to RAW\r\n");
		break;
	}
//	case 'm':{
//		uint8_t value=mode;
//		send_ota_uint8(value);
//		printLog("sent mode value: %d\r\n", value);
//		break;
//	}
	case 'I':{
		uint8_t value=0;

		if (write_flash) {
			value = 1;
			if (mark_set) {
				value = 3;
			}
		}
		send_ota_uint8(value);
		printLog("sent write_flash value: %d\r\n", value);
		break;
	}
	case 'w':{
		if (!write_flash) {
			_status |= 0x01;
			start_writing_flash();
		}
        break;
	}
	case 's':{
		write_flash = false;
		mark_set = false;
		_status &= ~(1); // clear flash writing bit
		update_counts_status();
		printLog("Stop writing to flash\r\n");
        break;
	}
	case 'A': {
		uint32_t ts = ts_ms();
		send_ota_uint32(ts);
		send_ota_uint32(_time_info.time_overflow);
		printLog("ts, overflow: %ld, %ld\r\n", ts, _time_info.time_overflow);
		break;
	}
	case 'a': {
		printLog("Send ota time info\r\n");
		send_ota_uint32(_time_info.epochtimesync);
		send_ota_uint32(_time_info.offset_time);
		send_ota_uint32(_time_info.offset_overflow);
		break;
	}
	case 'C': {
		flash_erase();
		// if (encounter_count==0) printLog("Flash is empty\r\n");
		break;
	}
	case 'S': {
		timer0_prescale(4);
		k_speaker = 440;
		pulse_width = 10e-3;
		populateBuffers(k_speaker);
		play_speaker();
		timer0_prescale(0);
		pulse_width = 1e-3;
		break;
	}
//	case 'o': {
//		uint8_t time_evt[32];
//        get_uint32(&(_time_info.epochtimesync));
//        get_uint32(&(_time_info.offset_time));
//        get_uint32(&(_time_info.offset_overflow));
//		printLog("epochtime offset_times: %ld %ld %ld\r\n",
//				_time_info.epochtimesync, _time_info.offset_time, _time_info.offset_overflow);
//		memset(time_evt, 0, 32);
//		memcpy(time_evt+4, &(_time_info.epochtimesync), 4);
//		memcpy(time_evt+8, &(_time_info.offset_time), 4);
//		memcpy(time_evt+12, &(_time_info.offset_overflow), 4);
//	    store_event(time_evt);
//	    setup_next_minute();
//		break;
//	}
	case 'B': { //bt scan parameters
		uint16_t *scan_params;

		scan_params = (uint16_t *) gecko_cmd_gatt_server_read_attribute_value(gattdb_gatt_spp_data,0 )->value.data;
		printLog("Change bluetooth scanning parameters interval, window: %u, %u\r\n", scan_params[0], scan_params[1]);
		// startObserving(scan_params[0], scan_params[1]);
		break;
	}
	case 'P': {  //connection parameters
		uint16_t *conn_params;

		conn_params = (uint16_t *) gecko_cmd_gatt_server_read_attribute_value(gattdb_gatt_spp_data,0 )->value.data;
		printLog("Change bluetooth connection parameters min %u, max %u, timeout min %u\r\n", conn_params[0], conn_params[1], conn_params[2]);
		// setConnectionTiming(conn_params);
		break;
	}
	case 'b': { //bt advertising parameters
		uint16_t *adv_params;

		adv_params = (uint16_t *) gecko_cmd_gatt_server_read_attribute_value(gattdb_gatt_spp_data,0 )->value.data;
		printLog("Change bluetooth adv parameters interval, window: %u, %u\r\n", adv_params[0], adv_params[1]);
		set_adv_params(adv_params);
		break;
	}

	case 'O': {
		// uint8_t time_evt[32];
		uint32_t *timedata;
		uint32_t ts = ts_ms();
		timedata = (uint32_t *) gecko_cmd_gatt_server_read_attribute_value(gattdb_gatt_spp_data,0 )->value.data;
		uint8_t len = gecko_cmd_gatt_server_read_attribute_value(gattdb_gatt_spp_data,0 )->value.len;
        printLog("In O, got %d bytes, ts: %ld\r\n", len, ts);
        sync_clock(ts, timedata);

		_status &= ~(1<<2); // clear clock not set bit
		update_counts_status();
		break;
	}

	case 'N': {
		uint8_t *new_name;
		new_name = (uint8_t *) gecko_cmd_gatt_server_read_attribute_value(gattdb_gatt_spp_data,0 )->value.data;
		set_name(new_name);
		printLog("set new name\r\n");
		break;
	}
	case 'M': {
        gecko_external_signal(LOG_MARK);
        mark_set = true;
        _status |= 0x2;
        update_counts_status();
        break;
	}
	case 'U': {
        gecko_external_signal(LOG_UNMARK);
        mark_set = false;
        _status &= 0xFD;
        update_counts_status();
        break;
	}
    case 'h':
        printLog("%s\r\n", version_str);
        break;
    case 'v': {
        send_ota_msg((char *)ota_version);
        break;
    }
	case '0':
		_client_type = CLIENT_IS_BTDEV;
		// printLog("set client type %d\r\n", _client_type);
		break;

	case '1':
		_client_type = CLIENT_IS_COMPUTER;
		// printLog("set client type %d\r\n", _client_type);
		break;

	default:
		printLog("OTA Got %c, %d\r\n", c, c);
		break;
	}
}

void parse_command(uint8_t c) {
	switch (c) {
	case 'k': {
		int16_t temp = 0;
		read_serial((uint8_t *)&temp, 2);
		printLog("Got k=%d\r\n", temp);
		k_goertzel = temp;
		k_speaker = temp;
		populateBuffers(k_speaker);

		break;
	}
	case 'p': {
		int16_t temp = 0;
		read_serial((uint8_t *)&temp, 2);
		printLog("Got p %d\r\n", temp);
		pulse_width = temp * 1e-4;
		populateBuffers(k_speaker);
		break;
	}
	case 'b': {
		int16_t temp = 0;
		read_serial((uint8_t *)&temp, 2);
		printLog("Got b %d\r\n", temp);
		conn_interval = temp;
		break;
	}
	case 'd':
		distant = true;
		printLog("distant %d\r\n", distant);
		break;

	case 'e':
		distant = false;
		printLog("distant %d\r\n", distant);
		break;

	case '0':
		out = 0;
		break;

	case '1':
		out = 1;
		break;

	case '2':
		out = 2;
		break;

	default:
		printLog("Got %c, %d\r\n", c, c);
		break;
	}
}

int IQR(int16_t* a, int n, int *mid_index);
// #define PRINT_ENCOUNTER_DETAIL
void print_encounter(int index) {
	Encounter_record_v2 *e;
	e = encounters+index;
	uint8_t *temp = (uint8_t *) e;
#ifdef PRINT_ENCOUNTER_DETAIL
	printLog("encounters[%d]=\r\n", index);
	for (int j = 0; j < 2; j++) {
		for (int i = 0; i < 32; i++) {
			printLog("%02X ", temp[i+32*j]);
		}
		printLog("\r\n");
	}
#endif
	uint16_t *sound = (uint16_t *) e;
	printLog("******* Sound: %5d", temp[11]);
	for (int i=6; i<10; i++) {
		printLog("%5d", sound[i]);
	}
	printLog("\r\n");
	flushLog();
}

// #define PRINT_TIMES
// #define PRINT_T_ARRAY
void record_tof(Encounter_record_v2 *current_encounter) {
#ifdef PRINT_TIMES
	printLog("--------n: %d, ", _data_idx);
#ifdef PRINT_T_ARRAY
	printLog("left_t: ");
	for (int i=0; i<_data_idx; i++) {
		printLog("%d, ", left_t[i]);
	}
	printLog("\r\n");
#endif
#endif
	int med, iqr;
	iqr = IQR(left_t, _data_idx, &med);
	current_encounter->usound.left= med;
	current_encounter->usound.left_irq = iqr;
#ifdef PRINT_TIMES
	printLog("l: %d, %d, ", med, iqr);
#ifdef PRINT_T_ARRAY
	printLog("right_t: ");
	for (int i=0; i<_data_idx; i++) {
		printLog("%d, ", right_t[i]);
	}
	printLog("\r\n");
#endif
#endif
	iqr = IQR(right_t, _data_idx, &med);
#ifdef PRINT_TIMES
	printLog("r: %d, %d\r\n", med, iqr);
#endif
	current_encounter->usound.n = _data_idx;
	current_encounter->usound.right = med;
	current_encounter->usound.right_irq = iqr;
}

#define PRIVATE

void check_time (char *msg) {
	uint32_t ts = ts_ms();
	if (ts > _time_info.next_minute) {
		if (_main_state == SCAN_ADV || _main_state == DISCONNECTED ) {
//		if (_role != ROLE_CLIENT_MASTER_UPLOAD ) {
			printLog("%lu: %s check_time update next minute, _main_state: %d ",
					ts, msg, _main_state);
			update_next_minute();
			printLog("%lu: next_minute %lu\r\n",ts_ms(), _time_info.next_minute);
		} else {
			_update_minute_after_upload = true;
		}
	}

}
//#define DEBUG_SHAREDCOUNT

void spp_client_main(void) {
#ifdef DEBUG_SHAREDCOUNT
	uint32_t prev_shared_count = 0;
#endif
	while (1) {
		/* Event pointer for handling events */
		struct gecko_cmd_packet* evt;

		check_time("while loop");
		// evt = gecko_wait_event();
	    if (listen_usb) {
	      /* If SPP data mode is active, use non-blocking gecko_peek_event() */
	      evt = gecko_peek_event();
	      int c = RETARGET_ReadChar();
	      if (c>=0) {
	    	  parse_command(c);
	    	  // printLog("Got %c, %d\r\n", c);
	      }
	      if (evt == NULL) {
	        continue; /* Jump directly to next iteration i.e. call gecko_peek_event() again */
	      }
	    } else {
	      /* SPP data mode not active -> check for stack events using the blocking API */
	      evt = gecko_wait_event();
	    }


		/* Handle events */
		switch (BGLIB_MSG_ID(evt->header)) {

		/* This boot event is generated when the system boots up after reset.
		 * Here the system is set to start advertising immediately after boot procedure. */
		case gecko_evt_system_boot_id:
			get_local_mac();
			printLog("Builtin mac: ");
			print_mac(local_mac);
			// Not sure this is the right place for speaker init stuff
			populateBuffers(k_speaker);
			startDMADRV_TMR();

			readBatteryLevel();
			printLog("BatteryLevel %d\r\n", battBatteryLevel);

			  int32_t flash_ret = storage_init();
			  uint32_t flash_size = storage_size();
			  printLog("storage_init: %ld %ld\r\n", flash_ret, flash_size);
			  determine_counts(flash_size);
			  read_encounters_tracking();
			  _encounters_tracker.start_upload = 0;
			  read_name_ps();

			// ***************
			reset_variables();
			gecko_cmd_gatt_set_max_mtu(247);
			// set initial public key
			update_public_key();
//			// Start discovery using the default 1M PHY
//			// Master_client mode
//			gecko_cmd_le_gap_start_discovery(1, le_gap_discover_generic);
			_main_state = SCAN_ADV;
			// Server_slave mode
			setup_adv();
			set_new_mac_address();

			gecko_cmd_hardware_set_soft_timer(32768<<2, HEARTBEAT_TIMER, false);

			break;

		case gecko_evt_le_gap_scan_response_id:
			if (_main_state == SCAN_ADV) {
				// create random offset to reduce possible bt collisions as master
				int offset = gecko_cmd_system_get_random_data(1)->data.data[0]%16;
				offset *= 250;
				// printLog("%lu, _main_state: %lu: wait before connecting\r\n", ts_ms(), _time_info.next_minute);

//				if (ts_ms() < (_time_info.next_minute - ENCOUNTER_PERIOD + 2000 +offset)) {
//					break;
//				}
				// printLog("Did we break?\r\n");
				int response = process_scan_response(&(evt->data.evt_le_gap_scan_response), &_status);

				if (response>0) {
					struct gecko_msg_le_gap_connect_rsp_t *pResp;
					if (response==1) {
						_role = ROLE_CLIENT_MASTER;
					} else {
						_role = ROLE_CLIENT_MASTER_UPLOAD;
					}

					if ((response==2) && (write_flash)) {
						write_flash = false;
						_status &= ~(1); // clear flash writing bit
						update_counts_status();
					}

					// Match found -> stop discovery and try to connect
					gecko_cmd_le_gap_end_procedure();
			        gecko_cmd_le_gap_stop_advertising(0); // Need HANDLE_ADV

					pResp = gecko_cmd_le_gap_connect(
							evt->data.evt_le_gap_scan_response.address,
							evt->data.evt_le_gap_scan_response.address_type, 1);

					if (pResp->result == bg_err_success) {
						// Make copy of connection handle for later use (for example, to cancel the connection attempt)
						_conn_handle = pResp->connection;
						if (_conn_handle == 0x02) {
							printLog("***********************************\r\n");
							gecko_cmd_le_connection_close(2);
						}
					} else {
						printLog(
								"gecko_cmd_le_gap_connect failed with code 0x%4.4x will try to close\r\n",
								pResp->result);
						printLog("mac address problem: ");
						print_mac(evt->data.evt_le_gap_scan_response.address.addr);
						if (pResp->result == 0x181) {
							gecko_cmd_le_connection_close(1);
							printLog("Tried to close extra connections\r\n");
						}
					}

				}
			}
			break;

			/* Connection opened event */
		case gecko_evt_le_connection_opened_id:
			// stop advertsing and scanning once we are connected
		    gecko_cmd_le_gap_end_procedure();
	        gecko_cmd_le_gap_stop_advertising(0);
			printLog("%lu: status: %d,  Role: %s ", ts_ms(), _status,
					(evt->data.evt_le_connection_opened.master == 0) ?
							slave_string : master_string);
			printLog("Handle: #%d ",
					evt->data.evt_le_connection_opened.connection);
			// printLog("state: %d ", _main_state);
			if (evt->data.evt_le_connection_opened.connection==2) {
				// gecko_cmd_le_connection_close(1);
				int16_t result = gecko_cmd_le_connection_close(2)->result;
				printLog("how did we get handle #2?, close result: %d  ", result);
				// printLog("tried to close 2\r\n");
				// TODO take care of bad encounter event
			}
			if (evt->data.evt_le_connection_opened.master == 1) {
				// Start service discovery (we are only interested in one UUID)
				if (_role == ROLE_CLIENT_MASTER) {
					gecko_cmd_le_connection_set_timing_parameters(_conn_handle, conn_interval,
							conn_interval, 0, 200, 0, 0xFFFF);
				} else { // set connection parameters so that it uploads faster
			    	gecko_cmd_le_connection_set_timing_parameters(_conn_handle, 6, 6, 0, 10, 0, 0xFFFF);
				}
				gecko_cmd_gatt_discover_primary_services(_conn_handle);
//				gecko_cmd_gatt_discover_primary_services_by_uuid(_conn_handle,
//						16, serviceUUID);
//				gecko_cmd_gatt_discover_primary_services_by_uuid(_conn_handle,
//						2, service_upload_UUID);
				_main_state = FIND_SERVICE;
				// _role = ROLE_CLIENT_MASTER;

//	    	    uint16_t result = gecko_cmd_le_gap_end_procedure()->result;
//	    	    printLog("try to end scan, result: %x\r\n", result);
			} else {
				_conn_handle = evt->data.evt_le_connection_opened.connection;
				_main_state = STATE_CONNECTED;
//				gecko_cmd_le_connection_set_timing_parameters(_conn_handle, conn_interval,
//						conn_interval, 0, 200, 0, 0xFFFF);
				_role = ROLE_SERVER_SLAVE;
			}
	        current_encounter = encounters + (c_fifo_last_idx & IDX_MASK);
	        // printLog("open connection type: %d mac: ", evt->data.evt_le_connection_opened.address_type);
	        // print_mac(evt->data.evt_le_connection_opened.address.addr);
	        setup_encounter_record(evt->data.evt_le_connection_opened.address.addr);
			break;

		case gecko_evt_le_connection_closed_id:
			// printLog("DISCONNECTED!\r\n");
			// printLog("_role, _client_type: %d, %d\r\n", _role, _client_type);
            if ( (_role==ROLE_CLIENT_MASTER) || ((_role==1) && (_client_type==0))) {
            	if (sharedCount>0) {
					// compute shared secret
					X25519_calc_shared_secret(shared_key, private_key,
							current_encounter->public_key);
					memcpy(current_encounter->public_key, shared_key, 32);
					// turnoff speaker or microphone

					// printLog("client_type: %d, role: %d\r\n", _client_type, _role);
					unlinkPRS();
					if (pdm_on) {
						pdm_pause();
						pdm_on = false;
					}
					// Get usound data and record encounter in fifo
					// record_tof(current_encounter);

					printLog("%lu: SC: %d, c_fifo: [%lu] ", ts_ms(), sharedCount, c_fifo_last_idx);
					print_encounter(c_fifo_last_idx & IDX_MASK);
					c_fifo_last_idx++; // this actually saves the data in the fifo
            	} else { printLog("__________sharedCount = 0\r\n"); }
			}
            // reset everything
			reset_variables();
			SLEEP_SleepBlockEnd(sleepEM2);  // Enable sleeping after disconnect
			gecko_cmd_hardware_set_soft_timer(32768>>2, RESTART_TIMER, true);
			break;

		case gecko_evt_le_connection_parameters_id:
			if (_role == ROLE_CLIENT_MASTER_UPLOAD) {
				printLog("Conn.parameters: interval %u units, txsize %u\r\n",
						evt->data.evt_le_connection_parameters.interval,
						evt->data.evt_le_connection_parameters.txsize);
			}
			break;

		case gecko_evt_gatt_mtu_exchanged_id:
			/* Calculate maximum data per one notification / write-without-response, this depends on the MTU.
			 * up to ATT_MTU-3 bytes can be sent at once  */
			_max_packet_size = evt->data.evt_gatt_mtu_exchanged.mtu - 3;
			_min_packet_size = _max_packet_size; // Try to send maximum length packets whenever possible
//			 printLog("MTU exchanged: %d\r\n",
//					evt->data.evt_gatt_mtu_exchanged.mtu);
			break;

		case gecko_evt_gatt_service_id:  // after connection handle looking for service
//			printLog("length of gatt_service: %d\r\n", evt->data.evt_gatt_service.uuid.len);
//			for(int i=0; i<evt->data.evt_gatt_service.uuid.len; i++) {
//				printLog("%02X ", evt->data.evt_gatt_service.uuid.data[i]);
//			}
//			printLog("\r\n");
			if (evt->data.evt_gatt_service.uuid.len == 16) {
				if (memcmp(serviceUUID, evt->data.evt_gatt_service.uuid.data,
						16) == 0) {
					// printLog("Service discovered\r\n");
					_service_handle = evt->data.evt_gatt_service.service;
				}
			}
			if (evt->data.evt_gatt_service.uuid.len == 2) {
				if (memcmp(service_upload_UUID, evt->data.evt_gatt_service.uuid.data,
						2) == 0) {
					printLog("upload service found, _main_state: %d\r\n", _main_state);
					_service_handle = evt->data.evt_gatt_service.service;
					_role = ROLE_CLIENT_MASTER_UPLOAD;
				}
			}
			break;

		case gecko_evt_gatt_procedure_completed_id:
			switch (_main_state) {
			case FIND_SERVICE:
				if (_service_handle > 0) {
					if (_role == ROLE_CLIENT_MASTER) {
					// Service found, next search for characteristics
					gecko_cmd_gatt_discover_characteristics(_conn_handle,
							_service_handle);
					_main_state = SEND_0;
					} else {
						gecko_cmd_gatt_discover_characteristics(_conn_handle,
								_service_handle);
						if (_status & (1<<2)) {
							_main_state = TIME_SYNC;
						} else {
							_main_state = FIND_UPLOAD;
						}
					}
				} else {
					// No service found -> disconnect
					printLog("SPP service not found?\r\n");
					gecko_cmd_le_connection_close(_conn_handle);
				}
				break;
			case TIME_SYNC:
		    	gecko_cmd_gatt_read_characteristic_value(_conn_handle, _char_time_handle);
				_main_state = FIND_UPLOAD;
				break;
			case FIND_UPLOAD:
				if (_char_upload_handle > 0) {
					// printLog("Try to send upload data\r\n");
					send_upload_data();
				} else {
					// No characteristic found? -> disconnect
					printLog("UPLOAD char not found?\r\n");
					gecko_cmd_le_connection_close(_conn_handle);
				}
				break;
			case FINISHED_UPLOAD:
				gecko_cmd_le_connection_close(_conn_handle);
				break;

			case FIND_SPP:
				if (_char_handle > 0) {
					// Char found, turn on indications
					gecko_cmd_gatt_set_characteristic_notification(_conn_handle,
							_char_handle, gatt_notification);
					_main_state = ENABLE_NOTIF;
				} else {
					// No characteristic found? -> disconnect
					printLog("SPP char not found?\r\n");
					gecko_cmd_le_connection_close(_conn_handle);
				}
				break;
			case SEND_0:
				if (_char_rw_handle > 0) {
					_main_state = FIND_SPP;
					send_rw_client('0');
				} else {
					// No characteristic found? -> disconnect
					printLog("RW char not found?\r\n");
					gecko_cmd_le_connection_close(_conn_handle);

				}
				break;
			case ENABLE_NOTIF:
				_main_state = DATA_MODE;
				// printLog("SPP Mode ON\r\n");
				SLEEP_SleepBlockBegin(sleepEM2); // Disable sleeping when SPP mode active
		        linkPRS();
				// Send rw command that a bt dev is trying to connect
				sharedCount = 0;
				// immediately try to update k_goertzel
				update_k_goertzel = true;
				send_spp_data_client();
				break;
			default:
				printLog("Unhandled gatt completed event: _main_state: %d\r\n", _main_state);
				break;
			}
			break;

		case gecko_evt_gatt_characteristic_id:  // after service found handle looking for characteristic
			if(false) {
				printLog("length of gatt_characteristic: %d\r\n",
						evt->data.evt_gatt_characteristic.uuid.len);
				for (int i = 0; i < evt->data.evt_gatt_characteristic.uuid.len;
						i++) {
					printLog("%02X ",
							evt->data.evt_gatt_characteristic.uuid.data[i]);
				}
				printLog("\r\n");
			}
			if (evt->data.evt_gatt_characteristic.uuid.len == 16) {
				if (memcmp(charUUID,
						evt->data.evt_gatt_characteristic.uuid.data, 16) == 0) {
					// printLog("Char SPP discovered\r\n");
					_char_handle =
							evt->data.evt_gatt_characteristic.characteristic;
				}
				if (memcmp(charUUID_rw,
						evt->data.evt_gatt_characteristic.uuid.data, 16) == 0) {
					// printLog("Char RW discovered\r\n");
					_char_rw_handle =
							evt->data.evt_gatt_characteristic.characteristic;
				}
			}
			if (evt->data.evt_gatt_characteristic.uuid.len == 2) {
				if (memcmp(char_upload_UUID,
						evt->data.evt_gatt_characteristic.uuid.data, 2) == 0) {
					printLog("upload char discovered\r\n");
					_char_upload_handle =
							evt->data.evt_gatt_characteristic.characteristic;
				}
				if (memcmp(char_time_UUID,
						evt->data.evt_gatt_characteristic.uuid.data, 2) == 0) {
					printLog("time char discovered\r\n");
					_char_time_handle =
							evt->data.evt_gatt_characteristic.characteristic;
				}
			}
			break;

		case gecko_evt_gatt_characteristic_value_id:
			if (evt->data.evt_gatt_characteristic_value.characteristic
					== _char_handle) {
				if (evt->data.evt_gatt_characteristic_value.value.len > 0) {
					// immediately try to update k_goertzel
					update_k_goertzel = true;
					uint32_t t = RTCC_CounterGet();
					sharedCount =
							evt->data.evt_gatt_characteristic_value.value.data[0];
#ifdef DEBUG_SHAREDCOUNT
					printLog("%lu, %lu, MC SC:%3d  len: %d\r\n", t,
							t - prev_rtcc, sharedCount,
							evt->data.evt_gatt_characteristic_value.value.len);
#endif
					if (sharedCount == 2) {
						memcpy(current_encounter->public_key, evt->data.evt_gatt_characteristic_value.value.data+1, 32);
						// printLog("got public key\r\n");
					}
					prev_rtcc = t;
					if (sharedCount>20) {
						gecko_cmd_le_connection_close(_conn_handle);
					} else {
						send_spp_data_client();
						gecko_cmd_le_connection_get_rssi(_conn_handle);
					}
					// send_spp_data_client();
				}
			}
			if (evt->data.evt_gatt_characteristic_value.characteristic
					== _char_time_handle) {
				printLog("length of gatt_characteristic_value: %d\r\n",
						evt->data.evt_gatt_characteristic_value.value.len);
				for (int i = 0; i < evt->data.evt_gatt_characteristic_value.value.len;
						i++) {
					printLog("%02X ",
							evt->data.evt_gatt_characteristic_value.value.data[i]);
				}
				printLog("\r\n");
				uint32_t ts = ts_ms();
				uint32_t timeinfo[3];
				memcpy(timeinfo, evt->data.evt_gatt_characteristic_value.value.data, 8);
				timeinfo[1] = ts - timeinfo[1];  // subtract offset
				timeinfo[2] = _time_info.time_overflow;

				sync_clock(ts, timeinfo);
				_status &= ~(1<<2); // clear clock not set bit
				update_counts_status();

				_main_state = FIND_UPLOAD;
			}
			break;

			/* Software Timer event */
		case gecko_evt_hardware_soft_timer_id: {
			uint32_t ts = ts_ms();
			if (ts > _time_info.next_minute) {
				if (_role != ROLE_CLIENT_MASTER_UPLOAD) {
					printLog("Soft timer update next minute, curr: %lu, next:%lu\r\n", ts, _time_info.next_minute);
					update_next_minute();
				} else {
					_update_minute_after_upload = true;
				}
			}
			readBatteryLevel();
			if (ts > (_time_info.near_hotspot_time + ENCOUNTER_PERIOD)) {
				if (!write_flash) {
					printLog("Not near hotspot, start writing\r\n");
					write_flash = true;
					_status |= 0x01;
					start_writing_flash();
					update_counts_status();
				}
				// Start writing
			}
			switch (evt->data.evt_hardware_soft_timer.handle) {
			case RESTART_TIMER:
				if (_update_minute_after_upload) {  // could be due to upload or taking data
					printLog("Handle missed update_minute\r\n");
					// update_next_minute();
					check_time("restart");
					printLog("next minute: %lu\r\n", _time_info.next_minute);
					_update_minute_after_upload = false;
				} else {
				// Restart discovery using the default 1M PHY
				start_bt();
				}
				_main_state = SCAN_ADV;
				break;
			case HEARTBEAT_TIMER:
				if ((_role == ROLE_SERVER_SLAVE)
						&& (_client_type != CLIENT_IS_BTDEV)) {
					if (_main_state != STATE_SPP_MODE) {

						printLog(
								"*********** check if reset needed conn_handle %d state: %d role: %d\r\n",
								_conn_handle, _main_state, _role);
						bad_heartbeats++;
					}
				}
				if (bad_heartbeats==3) {
					gecko_cmd_le_connection_close(_conn_handle);
					bad_heartbeats=0;
					reset_needed = false;
				}
				break;
			default:
				break;
			}
		}
			break;

		case gecko_evt_gatt_server_characteristic_status_id: {
			struct gecko_msg_gatt_server_characteristic_status_evt_t *pStatus;
			pStatus = &(evt->data.evt_gatt_server_characteristic_status);

			if (pStatus->characteristic == gattdb_gatt_spp_data) {
				if (pStatus->status_flags == gatt_server_client_config) {
					// Characteristic client configuration (CCC) for spp_data has been changed
					if (pStatus->client_config_flags == gatt_notification) {
						_main_state = STATE_SPP_MODE;
						SLEEP_SleepBlockBegin(sleepEM2); // Disable sleeping
						// printLog("SPP Mode ON\r\n");
						// printLog("Client Type: %d\r\n", _client_type);
						if (_client_type == CLIENT_IS_BTDEV) linkPRS();
						k_speaker = k_speaker_offsets[0] + K_OFFSET;
						// linkPRS();
						// pdm_start();
						// pdm_on = true;
					} else {
						// printLog("SPP Mode OFF\r\n");
						_main_state = STATE_CONNECTED;
						SLEEP_SleepBlockEnd(sleepEM2); // Enable sleeping
					}

				}
			}
		}
			break;

		case gecko_evt_gatt_server_attribute_value_id: {
			struct gecko_msg_gatt_server_attribute_value_evt_t *v;
			v = &(evt->data.evt_gatt_server_attribute_value);
			if (v->attribute == gattdb_Read_Write) {
				int c = v->value.data[0];
				// printLog("new rw value %c\r\n", c);
				parse_bt_command(c);
			} else if (v->attribute == gattdb_gatt_spp_data) {
				if (_client_type == CLIENT_IS_BTDEV) {
					if (v->value.len > 0) {
						uint32_t t = RTCC_CounterGet();
						sharedCount = v->value.data[0];
#ifdef DEBUG_SHAREDCOUNT
						printLog("%lu %lu %lu slave server SC: %d len: %d\r\n",
								t, t - prev_rtcc, t - prev_shared_count,
								v->value.data[0],
								v->value.len);
#endif
						if (sharedCount == 1) {
							memcpy(current_encounter->public_key,
									v->value.data + 1, 32);
							// printLog("got public key\r\n");
						} else {
							if (v->value.len>1) {
								// printLog("Got usound data, sharedCount: %d\r\n", sharedCount);
								int16_t *usound_data = (int16_t *) (v->value.data + 2);
								current_encounter->usound.n = v->value.data[1];
								current_encounter->usound.left = usound_data[0];
								current_encounter->usound.left_irq = usound_data[1];
								current_encounter->usound.right = usound_data[2];
								current_encounter->usound.right_irq = usound_data[3];
							}
						}
						prev_rtcc = t;
#ifdef DEBUG_SHAREDCOUNT
						prev_shared_count = prev_rtcc;
#endif
						send_spp_data();
						gecko_cmd_le_connection_get_rssi(_conn_handle);
						k_speaker = k_speaker_offsets[sharedCount>>1] + K_OFFSET;
						populateBuffers(k_speaker);
						// printLog("Update index: %d k_speaker: %ld\r\n", sharedCount>>1, k_speaker);
					}
				// } else if (_client_type == CLIENT_IS_COMPUTER) {
				} else  {
					process_server_spp_from_computer(evt, sending_ota, false);

				}
			}
		}
			break;
		case gecko_evt_le_connection_rssi_id: {
//			uint32_t t = RTCC_CounterGet();
//			printLog("%lu %lu %lu rssi: %d\r\n", t,
//					t - prev_rtcc, t-prev_shared_count, evt->data.evt_le_connection_rssi.rssi);
//			prev_rtcc = t;
			// printLog("%lu: rssi: %d\r\n", ts_ms(), evt->data.evt_le_connection_rssi.rssi);
			current_encounter->rssi_values[(_rssi_count++)%RSSI_LIST_SIZE] = evt->data.evt_le_connection_rssi.rssi;
			break;
		}

		case gecko_evt_gatt_server_user_read_request_id:
			printLog("user read request characteristic: %d\r\n",
					(evt->data.evt_gatt_server_user_read_request.characteristic));
			if (evt->data.evt_gatt_server_user_read_request.characteristic
					== gattdb_battery_level) {
				gecko_cmd_gatt_server_send_user_read_response(
						evt->data.evt_gatt_server_user_read_request.connection,
						evt->data.evt_gatt_server_user_read_request.characteristic,
						0, 1, &battBatteryLevel);
			}
			break;
		case gecko_evt_system_external_signal_id:
			process_ext_signals(evt->data.evt_system_external_signal.extsignals);
			break;

		case gecko_evt_le_connection_phy_status_id:
			break;

		default:
			printLog("Unhandled event  %lX\r\n", BGLIB_MSG_ID(evt->header));
			break;
		}
	}
}

