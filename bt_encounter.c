/*
 * bt_enconter.c
 *
 *  Created on: Aug 26, 2020
 *      Author: nams
 */

#include "bg_types.h"
#include "native_gecko.h"
#include "gatt_db.h"
#include "app.h"

#include "encounter/encounter.h"
#include "crypto_and_time/timing.h"
#include "ota.h"


//void update_counts_status(void);

/****************** globals */
//extern uint8 _max_packet_size;
//extern uint8 _min_packet_size;
extern uint8 _conn_handle;
//extern uint32_t encounter_count;
extern uint8_t local_mac[6];
extern uint8 serviceUUID[16];
extern uint8_t k_speaker_offsets[12];
// extern uint8_t k_goertzel_offsets[12];

extern bool enable_slave;
extern bool enable_master;

extern uint32_t chunk_offset;

// void update_public_key(void);
int in_encounters_fifo(const uint8_t * mac, uint32_t epoch_minute);
// extern int32_t k_goertzel;
// extern int32_t k_speaker;

// extern bool write_flash;

/* end globals **************/

#define K_OFFSET	268
#define K_OFFSET2	428
/* Definitions of external signals */
#define BUTTON_PRESSED (1 << 0)
#define BUTTON_RELEASED (1 << 1)
#define LOG_MARK (1<<2)
#define LOG_UNMARK (1<<3)
#define LOG_RESET (1<<4)

#define _KEY_WITH_ADV
// #define RANDOM_MAC
#define CHECK_PAST

uint8_t offset_list[12] = {0, 8, 20, 32, 40, 48, 56, 64, 88, 96, 104, 112};

struct {
	uint8_t flagsLen; /* Length of the Flags field. */
	uint8_t flagsType; /* Type of the Flags field. */
	uint8_t flags; /* Flags field. */
	uint8_t uuid16Len; /* Length of the Manufacturer Data field. */
	uint8_t uuid16Type;
	uint8_t uuid16Data[2]; /* Type of the Manufacturer Data field. */
	uint8_t svcLen; /* Company ID field. */
	uint8_t svcType; /* Company ID field. */
	uint8_t svcID[2]; /* Beacon Type field. */
	uint8_t key[16]; /* 128-bit Universally Unique Identifier (UUID). The UUID is an identifier for the company using the beacon*/
	uint8_t meta[4]; /* Beacon Type field. */
} etAdvData = {
/* Flag bits - See Bluetooth 4.0 Core Specification , Volume 3, Appendix C, 18.1 for more details on flags. */
2, /* length  */
0x01, /* type */
0x04 | 0x02, /* Flags: LE General Discoverable Mode, BR/EDR is disabled. */

/* Manufacturer specific data */
3, /* length of field*/
0x03, /* type */
{ 0x6F, 0xFD },

0x17, /* Length of service data */
0x16, /* Type */
{ 0x6F, 0xFD }, /* EN service */

/* 128 bit / 16 byte UUID */
{ 0xC0, 0x19, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x02, 0x19, 0xC0 },

{ 0x40, 0, 0, 0 }

};

struct {
	uint8_t flagsLen; /* Length of the Flags field. */
	uint8_t flagsType; /* Type of the Flags field. */
	uint8_t flags; /* Flags field. */
	uint8_t uuid16Len; /* Length of the Manufacturer Data field. */
	uint8_t uuid16Type;
	uint8_t uuid16Data[2]; /* Type of the Manufacturer Data field. */
	uint8_t uuid128Len; /* Length of the Manufacturer Data field. */
	uint8_t uuid128Type;
	uint8_t uuid128Data[16]; /* Type of the Manufacturer Data field. */
	uint8_t nameLen;
	uint8_t nameType;
	uint8_t name[4];
} etAdvData2 = {
/* Flag bits - See Bluetooth 4.0 Core Specification , Volume 3, Appendix C, 18.1 for more details on flags. */
2, /* length  */
0x01, /* type */
0x04 | 0x02, /* Flags: LE General Discoverable Mode, BR/EDR is disabled. */

/* Manufacturer specific data */
3, /* length of field*/
0x03, /* type */
//{ 0x6F, 0xFD },
//{ 0x19, 0xC0 },
{ 0x6F, 0xFD },

0x11, /* Length of service data */
0x06, /* Type */
/* 128 bit / 16 byte UUID */
{ 0x05, 0x81, 0x7E, 0xA0, 0xEE, 0x7A, 0x27, 0xA9,
		0x3E, 0x44, 0x68, 0x91, 0x24, 0x32, 0x18, 0x7B },

0x05,
0x09,
{'t', 'e', 's', 't'}

};


#define HANDLE_ADV	0

void get_local_mac(void) {
	bd_addr local_addr;
	local_addr = gecko_cmd_system_get_bt_address()->address;
	memcpy(local_mac, local_addr.addr, 6);
}

int32_t calc_k_from_mac(uint8_t *mac) {
	// int index = (mac[1] & 0xF) << 3;
	int index = mac[1] % 24;
	if (index < 18) {
		return K_OFFSET + (index << 3);
	} else {
		return K_OFFSET2 + ((index - 18) << 3);
	}
}

int32_t calc_k_offset(uint8_t index) {
	// printLog("offset[%d]: %d\r\n", index%12, offset_list[index%12]);
	return offset_list[index%8];
//		index %= 8;
//	// index %= 14;
//	if (index < 18) {
//		return (index << 3);
//	} else {
//		return 152 + ((index - 18) << 3);
//	}
}

void calc_k_offsets(uint8_t *mac, uint8_t *offsets) {
	for (int i = 0; i < 6; i++) {
		offsets[i << 1] = calc_k_offset(mac[i] & 0xF);
		offsets[(i << 1) + 1] = calc_k_offset((mac[i] >> 4) & 0xF);
//		printLog("mac[%d] %x: %d %d\r\n", i, mac[i], offsets[i << 1],
//				offsets[(i << 1) + 1]);
	}
}

void print_mac(uint8_t *addr) {
	for (int i = 0; i < 5; i++) {
		printLog("%2.2x:", addr[5 - i]);
	}
	printLog("%2.2x\r\n", addr[0]);
}


void setup_adv(void) {
	uint8_t len = sizeof(etAdvData2);
	uint8_t *pData = (uint8_t*) (&etAdvData2);
//	/* Set custom advertising data */
	uint16_t res = gecko_cmd_le_gap_bt5_set_adv_data(HANDLE_ADV, 0, len, pData)->result;
	printLog("Set adv data %u\r\n", res);

	/* Set 0 dBm Transmit Power */
	gecko_cmd_le_gap_set_advertise_tx_power(HANDLE_ADV, 0);
	// gecko_cmd_le_gap_set_advertise_configuration(HANDLE_ADV, 0x04);

	/* Set advertising parameters. 200ms (320/1.6) advertisement interval. All channels used.
	 * The first two parameters are minimum and maximum advertising interval, both in
	 * units of (milliseconds * 1.6).  */
	gecko_cmd_le_gap_set_advertise_timing(HANDLE_ADV, 320, 320, 0, 0);
	gecko_cmd_le_gap_set_discovery_timing(le_gap_phy_1m, 3200, 330);
	gecko_cmd_le_gap_set_discovery_type(le_gap_phy_1m, 0);

}

void print_offsets(uint8_t *offsets) {
	for (int i=0; i<12; i++) {
		printLog("%d ", offsets[i]);
	}
	printLog("\r\n");
}

#define USE_RAND_MAC
// extern uint8_t _status;
void start_bt(void) {
	// printLog("%lu: Start BT, _status:%d\r\n", ts_ms(), _status);
	calc_k_offsets(local_mac, k_speaker_offsets);
	if (enable_slave) {
		// Slave_server mode
		/* Start advertising in user mode */
		// advertise with name and custom service
#ifdef ADV_NAME
		uint16_t res = gecko_cmd_le_gap_start_advertising(HANDLE_ADV,
				le_gap_general_discoverable,
				le_gap_undirected_connectable)->result;
#else
		//uint16_t res =
		gecko_cmd_le_gap_start_advertising(HANDLE_ADV, le_gap_user_data,
		// le_gap_undirected_connectable)
		// le_gap_connectable_non_scannable)  // this does not work
				le_gap_connectable_scannable); //->result;
#endif
		// printLog("Start adv result: %x\r\n", res);
	}
	// Start discovery using the default 1M PHY
	// Master_client mode
	// uint16_t res_discovery =
	gecko_cmd_le_gap_start_discovery(1, le_gap_discover_generic); //->result;
	// printLog("%lu: Start discovery result: %d\r\n", ts_ms(), res_discovery);
}

void set_new_mac_address(void) {
	gecko_cmd_le_gap_stop_advertising(HANDLE_ADV);
	gecko_cmd_le_gap_end_procedure();

#ifdef USE_RAND_MAC
	bd_addr rnd_addr;
	struct gecko_msg_le_gap_set_advertise_random_address_rsp_t *response;
	// struct gecko_msg_le_gap_stop_advertising_rsp_t *stop_response;
	//struct gecko_msg_le_gap_set_advertise_configuration_rsp_t *config_response;
	struct gecko_msg_system_get_random_data_rsp_t *rnd_response;
	do {
		rnd_response = gecko_cmd_system_get_random_data(6);
		//  for valid 0x03 type addresses
		// rnd_response->data.data[5] &= 0x3F; // Last two bits must 0 for valid random mac address
		// what about 0x01?
		rnd_response->data.data[5] |= 0xC0; // Last two bits must be 1 for valid random mac address
		memcpy(rnd_addr.addr, rnd_response->data.data, 6);
		response = gecko_cmd_le_gap_set_advertise_random_address(HANDLE_ADV,
				0x01, rnd_addr);
		// printLog("create random address result: %x, ", response->result);
		printLog("new random mac: ");
		print_mac(response->address_out.addr);
	} while (response->result > 0);
	// if type is 0x02
	memcpy(local_mac, response->address_out.addr, 6);
	// if type is 0x03
	// memcpy(local_mac, rnd_addr.addr, 6);
#else
	get_local_mac();
#endif
	start_bt();


}

void  set_adv_params(uint16_t *params) {
    gecko_cmd_le_gap_stop_advertising(HANDLE_ADV);
	gecko_cmd_le_gap_set_advertise_timing(HANDLE_ADV, params[0], params[1], 0, 0);
	start_bt();
}

void startObserving(uint16_t interval, uint16_t window) {
	gecko_cmd_le_gap_end_procedure();
	/*  Start observing
	 * Use  extended to get channel info as well
	 */
	gecko_cmd_le_gap_set_discovery_extended_scan_response(1);
	/* scan on 1M PHY at 200 ms scan window/interval*/
	gecko_cmd_le_gap_set_discovery_timing(le_gap_phy_1m, interval, window);
	/* passive scanning */
	gecko_cmd_le_gap_set_discovery_type(le_gap_phy_1m, 0);
	/* discover all devices on 1M PHY*/
	gecko_cmd_le_gap_start_discovery(le_gap_phy_1m,
			le_gap_discover_observation);
}

void setConnectionTiming(uint16_t *params) {
	// Set connection parameters
	if (_conn_handle < 0xFF) {
		gecko_cmd_le_connection_set_timing_parameters(_conn_handle, params[0],
				params[1], 0, params[2], 0, 0xFFFF);
	}
}

uint8_t findServiceInAdvertisement(uint8_t *data, uint8_t len) {
	uint8_t adFieldLength;
	uint8_t adFieldType;
	uint8_t i = 0;
	// Parse advertisement packet
	while ((i + 3) < len) {
		adFieldLength = data[i];
		adFieldType = data[i + 1];
		if (adFieldType == 0x03) {
			// Look for exposure notification
			if (data[i + 2] == 0x6F && data[i + 3] == 0xFD) {
				return 1;
			}
		}
		// advance to the next AD struct
		i += adFieldLength + 1; // Need +1 because FieldLength byte is not included in length
	}
	return 0;
}

static bool compare_mac(uint8_t* addr) {
	uint32_t* l = (uint32_t *) local_mac;
	uint32_t* r = (uint32_t *) addr;

//	if (*l > *r) {
	if (*l < *r) {
		return 1;
	}
	return 0;
}

//extern uint32_t encounter_count;
//extern uint8_t _status;


int process_scan_response(struct gecko_msg_le_gap_scan_response_evt_t *pResp,
		uint8_t *status) {

	int i = 0;
	int ad_match_found = 0;
	int ad_len;
	int ad_type;

	// printLog("Process scan ");
//	printLog("%lu: process_scan rssi: %d ", ts_ms(),  pResp->rssi);
//	print_mac(pResp->address.addr);
	if (pResp->rssi <-85) {
		return 0;
	}
	while (i < (pResp->data.len - 1)) {

		ad_len = pResp->data.data[i];
		ad_type = pResp->data.data[i + 1];
		// printLog("ad_type: %4d, ad_len: %4d\r\n", ad_type, ad_len);
#ifdef NAME_MATCH
		char name[32];
		char dev_name[]="Empty Ex";
		if (ad_type == 0x08 || ad_type == 0x09) {
			// Type 0x08 = Shortened Local Name
			// Type 0x09 = Complete Local Name
			memcpy(name, &(pResp->data.data[i + 2]), ad_len - 1);
			name[ad_len - 1] = 0;
			printLog("found: %s\r\n", name);
			if (memcmp(dev_name, name, 8)==0) {
				printLog("Name match, addr compare: %d\r\n", compare_mac(pResp->address.addr));
				flushLog();
			}
		}
#endif
//		// 4880c12c-fdcb-4077-8920-a450d7f9b907
//		if (ad_type == 0x06 || ad_type == 0x07) {
//			// Type 0x06 = Incomplete List of 128-bit Service Class UUIDs
//			// Type 0x07 = Complete List of 128-bit Service Class UUIDs
//			if (memcmp(serviceUUID, &(pResp->data.data[i + 2]), 16) == 0) {
//				// printLog("Found SPP device\r\n");
//				// ad_match_found = 1;
//				ad_match_found = compare_mac(pResp->address.addr);
//			}
//		}
		if (ad_type == 0x03) {
			// Look for exposure notification
			if ((pResp->data.data[i + 2] == 0x6F)
					&& (pResp->data.data[i + 3] == 0xFD)) {

				if(enable_master && enable_slave) { // normal mode
					ad_match_found = compare_mac(pResp->address.addr);
				} else {
					printLog("cal mode\r\n");
					ad_match_found = 1; // in calibration mode
				}
				// ad_match_found = compare_mac(pResp->address.addr);
				// printLog("found exposure uuid\r\n");
			}
			if ((pResp->data.data[i + 2] == 0x19)
					&& (pResp->data.data[i + 3] == 0xC0)) {
//				printLog("found C019 uuid, count: %d, start: %d\r\n",
//						encounter_count, _encounters_tracker.start_upload);
				// bluetooth hotspot nearby so stop writing.
				_time_info.near_hotspot_time = ts_ms();  // TODO: Should we move this to spp_client_main?
				// check if there is new data
				// if (_encounters_tracker.start_upload < encounter_count) {
					//printLog("tracker: %lu, count %lu\r\n", _encounters_tracker.start_upload, encounter_count);
					ad_match_found = 2;
				// }
			}
		}
		if (ad_type == 0x06 || ad_type == 0x07) {
			// Type 0x06 = Incomplete List of 128-bit Service Class UUIDs
			// Type 0x07 = Complete List of 128-bit Service Class UUIDs
			if (memcmp(serviceUUID, &(pResp->data.data[i + 2]), 16) == 0) {
				// printLog("Found SPP device\r\n");
				// ad_match_found = 1;
				if(enable_master && enable_slave) { // normal mode
					ad_match_found = compare_mac(pResp->address.addr);
				} else {
					printLog("cal mode\r\n");
					ad_match_found = 1; // in calibration mode
				}
			}
		}


		// Jump to next AD record
		i = i + ad_len + 1;
	}

//	// Check if already found during this epoch_minute
//	if (ad_match_found==1) {
//		// check if already got data
//		// printLog("Found match, check if seen in previous time period\r\n");
//		uint32_t timestamp = ts_ms();
//		if (timestamp > _time_info.next_minute) {
//			// Need to update key and mac address...
//			update_next_minute();
//			return 0;
//		}
//		uint32_t epoch_minute = ((timestamp - _time_info.offset_time) / 1000
//				+ _time_info.epochtimesync) / 60;
//		if (*status & (1<<2)) return 0;
//		if ((*status & (1 << 3)) == 0) {  // Check if in encounter mode
//			// in encounter mode, check if data collected already
//			int idx = in_encounters_fifo(pResp->address.addr, epoch_minute);
//#ifndef CHECK_PAST
//			idx = -1;
//#endif
//			if (idx >= 0) {
//				// printLog("Connected during this minute already\r\n");
//				ad_match_found = 0;
//			} else {
//				// calculate k_goertzels
//				calc_k_offsets(pResp->address.addr, k_goertzel_offsets);
//				// printLog("k goertzel offsets: ");
//				// print_offsets(k_goertzel_offsets);
//			}
//		}
//	}
	// printLog("%lu: ad_match_found: %d\r\n", ts_ms(), ad_match_found);
	return (ad_match_found);
}

int process_scan_response_v2(struct gecko_msg_le_gap_scan_response_evt_t *pResp) {
	// Decoding advertising packets is done here. The list of AD types can be found
	// at: https://www.bluetooth.com/specifications/assigned-numbers/Generic-Access-Profile

	int i = 0;
	int ad_match_found = 0;
	int ad_len;
	int ad_type;

	// printLog("Process scan\r\n");
	while (i < (pResp->data.len - 1)) {

		ad_len = pResp->data.data[i];
		ad_type = pResp->data.data[i + 1];

		if (ad_type == 0x03) {
			if (pResp->data.data[i + 2] == 0x6F
					&& pResp->data.data[i + 3] == 0xFD) {
				printLog("Found SPP device\r\n");
				// ad_match_found = 1;
				ad_match_found = compare_mac(pResp->address.addr);
				flushLog();
			}
		}

		// Jump to next AD record
		i = i + ad_len + 1;
	}
	// return 0;
	// printLog("ad_match_found: %d\r\n", ad_match_found);
	return (ad_match_found);
}

#include "encounter/encounter.h"
extern Encounter_record_v2 encounters[IDX_MASK + 1];
extern uint32_t c_fifo_last_idx;
// extern uint32_t p_fifo_last_idx;
// extern uint32_t p_fifo_last_idx;

//#define SCAN_DEBUG
void setup_encounter_record(uint8_t* mac_addr) {
	Encounter_record_v2 *current_encounter;
	uint32_t timestamp = ts_ms();

	if (timestamp > _time_info.next_minute)
		update_next_minute();
	// int sec_timestamp = (timestamp - (_time_info.next_minute - 60000)) / 1000;
	uint32_t epoch_minute = ((timestamp - _time_info.offset_time) / 1000
			+ _time_info.epochtimesync) / 60;

#ifdef SCAN_DEBUG
	printLog("\tTry to check if in fifo, epoch_minute: %ld %lx\r\n",
			epoch_minute, epoch_minute);
#endif
	// Check if record already exists by mac
	int idx = in_encounters_fifo(mac_addr, epoch_minute);
#ifndef CHECK_PAST
			idx = -1;
#endif
#ifdef SCAN_DEBUG
	printLog("\tp_idx: %ld, c_idx: %ld\n", p_fifo_last_idx, c_fifo_last_idx);
#endif
	if (idx < 0) {
		// No index returned
#ifdef SCAN_DEBUG
		printLog("\tCreate new encounter: mask_idx: %ld\n",
				c_fifo_last_idx & IDX_MASK);
#endif
		current_encounter = encounters + (c_fifo_last_idx & IDX_MASK);
		memset((uint8_t *) current_encounter, 0, 64);  // clear all values
		memcpy(current_encounter->mac, mac_addr, 6);

		printLog("remote MAC: ");
		print_mac(mac_addr);
		// print_mac(local_mac);

		current_encounter->minute = epoch_minute;
		// current_encounter->version = 0xFF;
		// c_fifo_last_idx++;
	}
}

#include "encounter/encounter.h"
// void print_encounter(int index);

void fake_encounter(uint8_t num) {
	Encounter_record_v2 *current_encounter;
	uint32_t timestamp = ts_ms();
	if (timestamp > _time_info.next_minute)
		update_next_minute();
//    int sec_timestamp = (timestamp - (_time_info.next_minute - 60000)) / 1000;
	uint32_t epoch_minute = ((timestamp - _time_info.offset_time) / 1000
			+ _time_info.epochtimesync) / 60;

	uint8_t mac_addr[6] = { 0, 0, 0, 0, 0, 0 };
	memset(mac_addr, num - 1, 6);
	current_encounter = encounters + (c_fifo_last_idx & IDX_MASK);
	memset((uint8_t *) current_encounter, num, 64);  // clear all values
	memcpy(current_encounter->mac, mac_addr, 6);
	current_encounter->minute = epoch_minute;
	// print_encounter(c_fifo_last_idx & IDX_MASK);
	c_fifo_last_idx++;
}

void process_ext_signals(uint32_t signals) {
	if (signals & LOG_MARK) {
		printLog("MARK\r\n");
		fake_encounter(1);
	}
	if (signals & LOG_UNMARK) {
		printLog("UNMARK\r\n");
		fake_encounter(2);
	}

	if (signals & LOG_RESET) {
		printLog("RESET MARK\r\n");
		fake_encounter(3);

	}

	if (signals & BUTTON_PRESSED) {
		printLog("Button Pressed\r\n");
	}

	if (signals & BUTTON_RELEASED) {
		printLog("Button released (reset)\r\n");
	}

}
/*
 void process_server_spp_from_btdev(struct gecko_cmd_packet* evt) {
 static uint32_t prev_shared_count = 0; // temp copy this variable

 if (evt->data.evt_gatt_server_attribute_value.value.len
 > 0) {
 uint32_t t = RTCC_CounterGet();
 sharedCount =
 evt->data.evt_gatt_server_attribute_value.value.data[0];
 printLog("%lu %lu %lu slave server SC: %d len: %d\r\n",
 t, t - prev_rtcc, t - prev_shared_count,
 sharedCount,
 evt->data.evt_gatt_server_attribute_value.value.len
 );
 if (sharedCount == 1) {
 memcpy(current_encounter->public_key,
 evt->data.evt_gatt_server_attribute_value.value.data
 + 1, 32);
 printLog("got public key\r\n");
 } else {
 // receive time data and fill array
 int16_t *time_data;
 time_data = left_t + _data_idx;
 memcpy((uint8_t *) time_data,
 evt->data.evt_gatt_server_attribute_value.value.data
 + 1, 4);
 time_data = right_t + _data_idx;
 memcpy((uint8_t *) time_data,
 evt->data.evt_gatt_server_attribute_value.value.data
 + 5, 4);
 _data_idx += 2;
 }
 prev_rtcc = t;
 prev_shared_count = prev_rtcc;
 send_spp_data();
 gecko_cmd_le_connection_get_rssi(_conn_handle);
 }
 }
 */
void process_server_spp_from_computer(struct gecko_cmd_packet* evt,
		bool sending_ota, bool sending_turbo) {
	if (evt->data.evt_gatt_server_attribute_value.attribute
			== gattdb_gatt_spp_data) {
		if (sending_ota) {
			uint32_t *index;
			int len = evt->data.evt_gatt_server_attribute_value.value.len;
			if (len == 4) {
				index =
						(uint32_t *) evt->data.evt_gatt_server_attribute_value.value.data;
				printLog("Got request for data, len %d, idx: %ld\r\n", evt->data.evt_gatt_server_attribute_value.value.len, *index);
				send_chunk(*index, chunk_offset);
			} else {
				printLog("Recevied the wrong number of bytes: %d/4\r\n", len);
			}
		} else if (sending_turbo) {
			uint32_t *params;
			int len = evt->data.evt_gatt_server_attribute_value.value.len;
			if (len == 8) {
				params =
						(uint32_t *) evt->data.evt_gatt_server_attribute_value.value.data;
				printLog(
						"Got request for turbo data, len %d, idx: %lu len: %lu\r\n",
						evt->data.evt_gatt_server_attribute_value.value.len,
						params[0], params[1]);
				send_data_turbo(params[0], params[1]);
			} else {
				printLog("Recevied the wrong number of bytes: %d/8\r\n", len);
			}
		} else {
			char *msg;
			msg = (char *) evt->data.evt_gatt_server_attribute_value.value.data;
			printLog("new message: %s\r\n", msg);
		}
	} else if (evt->data.evt_gatt_server_attribute_value.attribute
			== gattdb_data_in) {

		if (sending_ota) {
			uint32_t *index;
			int len = evt->data.evt_gatt_server_attribute_value.value.len;
			if (len == 4) {
				index =
						(uint32_t *) evt->data.evt_gatt_server_attribute_value.value.data;
				// printLog("Got request for packet, len %d, idx: %ld\r\n", evt->data.evt_gatt_server_attribute_value.value.len, *index);
				send_chunk(*index, chunk_offset);
			} else {
				printLog("Recevied the wrong number of bytes: %d/4\r\n", len);
			}
		} else {
			char *msg;
			msg = (char *) evt->data.evt_gatt_server_attribute_value.value.data;
			printLog("gatt_data_in new message: %s\r\n", msg);
		}

		printLog("gatt data_in\r\n");
	}
// printLog("Done attribute_value_id\r\n");
// parse_command(c);

}
