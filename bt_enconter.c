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
/****************** globals */
extern uint8 _max_packet_size;
extern uint8 _min_packet_size;
extern uint8 _conn_handle;
extern uint32_t encounter_count;
extern uint8_t local_mac[6];
extern uint8 serviceUUID[16];

void update_public_key(void);

/* end globals **************/

struct
{
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
{0x6F, 0xFD},

0x17, /* Length of service data */
0x16, /* Type */
{0x6F, 0xFD},  /* EN service */

/* 128 bit / 16 byte UUID */
{0xC0, 0x19, 0x01, 0x00,
 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00,
 0x00, 0x02, 0x19, 0xC0},

 {0x40, 0, 0, 0}

};

#define HANDLE_ADV	0

void get_local_mac(void) {
	bd_addr local_addr;
	local_addr = gecko_cmd_system_get_bt_address()->address;
	memcpy(local_mac, local_addr.addr,  6);
}

void set_new_mac_address(void) {
	bd_addr rnd_addr;
	struct gecko_msg_le_gap_set_advertise_random_address_rsp_t *response;
	// struct gecko_msg_le_gap_stop_advertising_rsp_t *stop_response;
	//struct gecko_msg_le_gap_set_advertise_configuration_rsp_t *config_response;
	struct gecko_msg_system_get_random_data_rsp_t *rnd_response;

	//stop_response =
	gecko_cmd_le_gap_stop_advertising(HANDLE_ADV);

	do {
		rnd_response = gecko_cmd_system_get_random_data(6);
		rnd_response->data.data[5] &= 0x3F; // Last two bits must 0 for valid random mac address
		memcpy(rnd_addr.addr, rnd_response->data.data, 6);
		response = gecko_cmd_le_gap_set_advertise_random_address(HANDLE_ADV,
				0x02, rnd_addr);
	} while (response->result > 0);
	memcpy(local_mac, rnd_addr.addr, 6);

//  gecko_cmd_le_gap_start_advertising(HANDLE_ADV, le_gap_user_data, le_gap_non_connectable);
}

void setup_adv(void) {
//	uint8_t len = sizeof(etAdvData);
//	uint8_t *pData = (uint8_t*) (&etAdvData);
//	/* Set custom advertising data */
//	uint16_t res = gecko_cmd_le_gap_bt5_set_adv_data(HANDLE_ADV, 0, len, pData)->result;
//	printLog("Set adv data %u\r\n", res);

	/* Set 0 dBm Transmit Power */
	gecko_cmd_le_gap_set_advertise_tx_power(HANDLE_ADV, 0);
	// gecko_cmd_le_gap_set_advertise_configuration(HANDLE_ADV, 0x04);

	/* Set advertising parameters. 200ms (320/1.6) advertisement interval. All channels used.
	 * The first two parameters are minimum and maximum advertising interval, both in
	 * units of (milliseconds * 1.6).  */
	gecko_cmd_le_gap_set_advertise_timing(HANDLE_ADV, 320, 320, 0, 0);
}

void start_adv(void) {
#ifdef UPDATE_KEY_WITH_ADV
	update_public_key();
#endif
	// set_new_mac_address();

	//	/* Set custom advertising data */
//	gecko_cmd_le_gap_bt5_set_adv_data(HANDLE_ADV, 0, len, pData);
//
//	/* Set 0 dBm Transmit Power */
//	gecko_cmd_le_gap_set_advertise_tx_power(HANDLE_ADV, 0);
//	gecko_cmd_le_gap_set_advertise_configuration(HANDLE_ADV, 0x04);
//
//	/* Set advertising parameters. 200ms (320/1.6) advertisement interval. All channels used.
//	 * The first two parameters are minimum and maximum advertising interval, both in
//	 * units of (milliseconds * 1.6).  */
//	gecko_cmd_le_gap_set_advertise_timing(HANDLE_ADV, 320, 320, 0, 0);

	/* Start advertising in user mode */
	uint16_t res = gecko_cmd_le_gap_start_advertising(HANDLE_ADV, le_gap_general_discoverable,
	// uint16_t res = gecko_cmd_le_gap_start_advertising(HANDLE_ADV, le_gap_user_data,
			le_gap_undirected_connectable)
			// le_gap_connectable_non_scannable)
			// le_gap_connectable_scannable)
			// le_gap_non_connectable)
			->result;
	printLog("Start adv result: %x\r\n", res);
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
	gecko_cmd_le_gap_set_discovery_type(le_gap_phy_1m,0);
	/* discover all devices on 1M PHY*/
	gecko_cmd_le_gap_start_discovery(le_gap_phy_1m,le_gap_discover_observation);
}

void setConnectionTiming(uint16_t *params) {
	// Set connection parameters
	if (_conn_handle < 0xFF) {
		gecko_cmd_le_connection_set_timing_parameters(_conn_handle, params[0],
				params[1], 0, params[2], 0, 0xFFFF);
	}
}

uint8_t findServiceInAdvertisement(uint8_t *data, uint8_t len)
{
  uint8_t adFieldLength;
  uint8_t adFieldType;
  uint8_t i = 0;
  // Parse advertisement packet
  while ((i+3) < len) {
    adFieldLength = data[i];
    adFieldType = data[i + 1];
    if (adFieldType == 0x03) {
      // Look for exposure notification
      if (data[i + 2]==0x6F && data[i+3]==0xFD) {
        return 1;
      }
    }
    // advance to the next AD struct
    i += adFieldLength + 1;  // Need +1 because FieldLength byte is not included in length
  }
  return 0;
}

static bool compare_mac(uint8_t* addr) {
//	bd_addr local_addr;
//	local_addr = gecko_cmd_system_get_bt_address()->address;
//	uint32_t* l = (uint32_t *)(local_addr.addr);
	get_local_mac();
	uint32_t* l = (uint32_t *) local_mac;
	uint32_t* r = (uint32_t *) addr;

//	for (int i = 0; i < 6; i++) {
//		printLog("%2x ", addr[i]);
//	}
//	printLog("local address: ");
//				for (i = 0; i < 5; i++) {
//					printLog("%2.2x:", local_addr.addr[5 - i]);
//				}
//	printLog("%2.2x\r\n", local_addr.addr[0]);
//    printLog("\r\n");
    // printLog("%lu, %lu\r\n", *l, *r);
//	if (*l > *r) {
	if (*l < *r) {
		return 1;
	}
	return 0;
}

int process_scan_response(
		struct gecko_msg_le_gap_scan_response_evt_t *pResp) {
	// Decoding advertising packets is done here. The list of AD types can be found
	// at: https://www.bluetooth.com/specifications/assigned-numbers/Generic-Access-Profile

	int i = 0;
	int ad_match_found = 0;
	int ad_len;
	int ad_type;

	char name[32];

    char dev_name[]="Empty Ex";
    printLog("Process scan\r\n");
	while (i < (pResp->data.len - 1)) {

		ad_len = pResp->data.data[i];
		ad_type = pResp->data.data[i + 1];

		if (ad_type == 0x08 || ad_type == 0x09) {
			// Type 0x08 = Shortened Local Name
			// Type 0x09 = Complete Local Name
			memcpy(name, &(pResp->data.data[i + 2]), ad_len - 1);
			name[ad_len - 1] = 0;
			printLog("found: %s\r\n", name);
			if (memcmp(dev_name, name, 8)==0)  {
				printLog("Name match, addr compare: %d\r\n", compare_mac(pResp->address.addr));
				flushLog();
			}
		}

		// 4880c12c-fdcb-4077-8920-a450d7f9b907
		if (ad_type == 0x06 || ad_type == 0x07) {
			// Type 0x06 = Incomplete List of 128-bit Service Class UUIDs
			// Type 0x07 = Complete List of 128-bit Service Class UUIDs
			if (memcmp(serviceUUID, &(pResp->data.data[i + 2]), 16) == 0) {
				printLog("Found SPP device\r\n");
				// ad_match_found = 1;
				ad_match_found = compare_mac(pResp->address.addr);
			}
		}

		// Jump to next AD record
		i = i + ad_len + 1;
	}
	// return 0;
	printLog("ad_match_found: %d\r\n", ad_match_found);
	return (ad_match_found);
}

int process_scan_response_v2(
		struct gecko_msg_le_gap_scan_response_evt_t *pResp) {
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
			if (pResp->data.data[i + 2] == 0x6F && pResp->data.data[i + 3] == 0xFD) {
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

