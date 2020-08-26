/***********************************************************************************************//**
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

/***************************************************************************************************
 Local Macros and Definitions
 **************************************************************************************************/

#define DISCONNECTED	0
#define SCAN_ADV		  1
#define FIND_SERVICE	2
#define FIND_CHAR		  3
#define ENABLE_NOTIF 	4
#define DATA_MODE		  5
#define DISCONNECTING 6
#define STATE_CONNECTED   7
#define STATE_SPP_MODE 8
#define ROLE_UNKNOWN -1
#define ROLE_CLIENT_MASTER 0
#define ROLE_SERVER_SLAVE 1

// SPP service UUID: 4880c12c-fdcb-4077-8920-a450d7f9b907
const uint8 serviceUUID[16] = { 0x07, 0xb9, 0xf9, 0xd7, 0x50, 0xa4, 0x20, 0x89,
		0x77, 0x40, 0xcb, 0xfd, 0x2c, 0xc1, 0x80, 0x48 };

// SPP data UUID: fec26ec4-6d71-4442-9f81-55bc21d658d6
const uint8 charUUID[16] = { 0xd6, 0x58, 0xd6, 0x21, 0xbc, 0x55, 0x81, 0x9f,
		0x42, 0x44, 0x71, 0x6d, 0xc4, 0x6e, 0xc2, 0xfe };

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

void initPDM(void);
void pdm_start(void);
void pdm_pause(void);

void populateBuffers(int k_value);
void startDMADRV_TMR(void);


bool pdm_on = false;

int32_t k_goertzel=276;
int32_t k_speaker = 276;
float pulse_width = 1.0e-3;
int out=2;
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
 Local Variables
 **************************************************************************************************/
static uint8 _conn_handle = 0xFF;
static int _main_state;
static uint32 _service_handle;
static uint16 _char_handle;

static int8 _role = ROLE_UNKNOWN;
/* Default maximum packet size is 20 bytes. This is adjusted after connection is opened based
 * on the connection parameters */
static uint8 _max_packet_size = 20;
static uint8 _min_packet_size = 20;  // Target minimum bytes for one packet

volatile int conn_interval = 400;
static void reset_variables() {
	_conn_handle = 0xFF;
	_main_state = DISCONNECTED;
	_service_handle = 0;
	_char_handle = 0;
	_role = ROLE_UNKNOWN;

	_max_packet_size = 20;
	_data_idx = 0;
	memset(left_t, 0, 32*2);
	memset(right_t, 0, 32*2);
}

static bool compare_mac(uint8_t* addr) {
	bd_addr local_addr;
	local_addr = gecko_cmd_system_get_bt_address()->address;
	uint32_t* l = (uint32_t *)(local_addr.addr);
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

static int process_scan_response(
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

static void send_spp_data_client() {
	uint8 len = 1;
	uint16 result;
	sharedCount++;
	// Stack may return "out-of-memory" error if the local buffer is full -> in that case, just keep trying until the command succeeds
	do {
		result = gecko_cmd_gatt_write_characteristic_value_without_response(
				_conn_handle, _char_handle, len, &sharedCount)->result;
	} while (result == bg_err_out_of_memory);

	if (result != 0) {
		printLog("Unexpected error: %x\r\n", result);
	}

}

static void send_spp_data() {
	uint8 len = 1;
	uint16 result;

	sharedCount++;
	if (len > 0) {
		// Stack may return "out-of-memory" error if the local buffer is full -> in that case, just keep trying until the command succeeds
		do {
			result = gecko_cmd_gatt_server_send_characteristic_notification(
					_conn_handle,
					gattdb_gatt_spp_data, len, &sharedCount)->result;
		} while (result == bg_err_out_of_memory);

		if (result != 0) {
			printLog("Unexpected error: %x\r\n", result);
		}
	}
}


void linkPRS() {
	//if (_role == ROLE_CLIENT_MASTER) {
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

void print_times(void) {
	printLog("--------n: %d\r\n", _data_idx);
#ifdef PRINT_T_ARRAY
	printLog("left_t: ");
	for (int i=0; i<_data_idx; i++) {
		printLog("%d, ", left_t[i]);
	}
	printLog("\r\n");
#endif
	int med, iqr;
	iqr = IQR(left_t, _data_idx, &med);
	printLog("--------left: %d, %d\r\n", med, iqr);
#ifdef PRINT_T_ARRAY
	printLog("right_t: ");
	for (int i=0; i<_data_idx; i++) {
		printLog("%d, ", right_t[i]);
	}
	printLog("\r\n");
#endif
	iqr = IQR(right_t, _data_idx, &med);
	printLog("--------right: %d, %d\r\n", med, iqr);
}

void spp_client_main(void) {
	uint32_t prev_shared_count = 0;
	while (1) {
		/* Event pointer for handling events */
		struct gecko_cmd_packet* evt;

		// evt = gecko_wait_event();
	    if (true) {
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
			// Not sure this is the right place for speaker init stuff
			populateBuffers(k_speaker);
			startDMADRV_TMR();
			// ***************
			reset_variables();
			gecko_cmd_gatt_set_max_mtu(247);

			// Start discovery using the default 1M PHY
			// Master_client mode
			gecko_cmd_le_gap_start_discovery(1, le_gap_discover_generic);
			_main_state = SCAN_ADV;
			// Server_slave mode
			gecko_cmd_le_gap_start_advertising(0, le_gap_general_discoverable,
					le_gap_undirected_connectable);

			gecko_cmd_hardware_set_soft_timer(32768<<2, HEARTBEAT_TIMER, false);

			break;

		case gecko_evt_le_gap_scan_response_id:
			if (_main_state == SCAN_ADV) {
			// if (true) {
				// printLog("_main_state %d\r\n", _main_state);
				// Process scan responses: this function returns 1 if we found the service we are looking for
				if (process_scan_response(&(evt->data.evt_le_gap_scan_response))
						> 0) {
					struct gecko_msg_le_gap_connect_rsp_t *pResp;

					// Match found -> stop discovery and try to connect
					gecko_cmd_le_gap_end_procedure();

					pResp = gecko_cmd_le_gap_connect(
							evt->data.evt_le_gap_scan_response.address,
							evt->data.evt_le_gap_scan_response.address_type, 1);

					if (pResp->result == bg_err_success) {
						// Make copy of connection handle for later use (for example, to cancel the connection attempt)
						_conn_handle = pResp->connection;
					} else {
						printLog(
								"gecko_cmd_le_gap_connect failed with code 0x%4.4x\r\n",
								pResp->result);
					}

				}
			}
			break;

			/* Connection opened event */
		case gecko_evt_le_connection_opened_id:
			// stop advertsing and scanning once we are connected
		    gecko_cmd_le_gap_end_procedure();
	        gecko_cmd_le_gap_stop_advertising(0);
			printLog("Connected\r\n");
			printLog("Role: %s\r\n",
					(evt->data.evt_le_connection_opened.master == 0) ?
							slave_string : master_string);
			printLog("Handle: #%d\r\n",
					evt->data.evt_le_connection_opened.connection);
			if (evt->data.evt_le_connection_opened.master == 1) {
				// Start service discovery (we are only interested in one UUID)
				gecko_cmd_gatt_discover_primary_services_by_uuid(_conn_handle,
						16, serviceUUID);
				_main_state = FIND_SERVICE;
				_role = ROLE_CLIENT_MASTER;
//	    	    uint16_t result = gecko_cmd_le_gap_end_procedure()->result;
//	    	    printLog("try to end scan, result: %x\r\n", result);
			} else {
				_conn_handle = evt->data.evt_le_connection_opened.connection;
				_main_state = STATE_CONNECTED;
				gecko_cmd_le_connection_set_timing_parameters(_conn_handle, conn_interval,
						conn_interval, 0, 200, 0, 0xFFFF);
				_role = ROLE_SERVER_SLAVE;
			}

			break;

		case gecko_evt_le_connection_closed_id:
			printLog("DISCONNECTED!\r\n");
			unlinkPRS();
			// if (_role == ROLE_SERVER_SLAVE) {
			if (pdm_on) {
				pdm_pause();
				pdm_on = false;
			}
			print_times();
			reset_variables();
			SLEEP_SleepBlockEnd(sleepEM2);  // Enable sleeping after disconnect
			gecko_cmd_hardware_set_soft_timer(32768<<1, RESTART_TIMER, true);
			break;

		case gecko_evt_le_connection_parameters_id:
			printLog("Conn.parameters: interval %u units, txsize %u\r\n",
					evt->data.evt_le_connection_parameters.interval,
					evt->data.evt_le_connection_parameters.txsize);
			break;

		case gecko_evt_gatt_mtu_exchanged_id:
			/* Calculate maximum data per one notification / write-without-response, this depends on the MTU.
			 * up to ATT_MTU-3 bytes can be sent at once  */
			_max_packet_size = evt->data.evt_gatt_mtu_exchanged.mtu - 3;
			_min_packet_size = _max_packet_size; // Try to send maximum length packets whenever possible
			printLog("MTU exchanged: %d\r\n",
					evt->data.evt_gatt_mtu_exchanged.mtu);
			break;

		case gecko_evt_gatt_service_id:

			if (evt->data.evt_gatt_service.uuid.len == 16) {
				if (memcmp(serviceUUID, evt->data.evt_gatt_service.uuid.data,
						16) == 0) {
					printLog("Service discovered\r\n");
					_service_handle = evt->data.evt_gatt_service.service;
				}
			}
			break;

		case gecko_evt_gatt_procedure_completed_id:

			switch (_main_state) {
			case FIND_SERVICE:

				if (_service_handle > 0) {
					// Service found, next search for characteristics
					gecko_cmd_gatt_discover_characteristics(_conn_handle,
							_service_handle);
					_main_state = FIND_CHAR;
				} else {
					// No service found -> disconnect
					printLog("SPP service not found?\r\n");
					gecko_cmd_le_connection_close(_conn_handle);
				}

				break;

			case FIND_CHAR:
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

			case ENABLE_NOTIF:
				_main_state = DATA_MODE;
				printLog("SPP Mode ON\r\n");
				SLEEP_SleepBlockBegin(sleepEM2); // Disable sleeping when SPP mode active
		        linkPRS();
				// pdm_start();
				// pdm_on = true;
				sharedCount = 0;
				send_spp_data_client();

				break;

			default:
				break;
			}
			break;

		case gecko_evt_gatt_characteristic_id:

			if (evt->data.evt_gatt_characteristic.uuid.len == 16) {
				if (memcmp(charUUID,
						evt->data.evt_gatt_characteristic.uuid.data, 16) == 0) {
					printLog("Char discovered\r\n");
					_char_handle =
							evt->data.evt_gatt_characteristic.characteristic;
				}
			}

			break;

		case gecko_evt_gatt_characteristic_value_id:
			if (evt->data.evt_gatt_characteristic_value.characteristic
					== _char_handle) {
				if (evt->data.evt_gatt_server_attribute_value.value.len > 0) {
					uint32_t t = RTCC_CounterGet();
					sharedCount =
							evt->data.evt_gatt_server_attribute_value.value.data[0];
					printLog("%lu, %lu, MC SC:%3d\r\n", t,
							t - prev_rtcc, sharedCount);
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
			break;

			/* Software Timer event */
		case gecko_evt_hardware_soft_timer_id:

			switch (evt->data.evt_hardware_soft_timer.handle) {
			case RESTART_TIMER:
				// Restart discovery using the default 1M PHY
				gecko_cmd_le_gap_start_discovery(1, le_gap_discover_generic);
				gecko_cmd_le_gap_start_advertising(0, le_gap_general_discoverable,
						le_gap_undirected_connectable);
				_main_state = SCAN_ADV;
				break;
			case HEARTBEAT_TIMER:
				// printLog("*********** check if reset needed\r\n");
				if (reset_needed) {
					gecko_cmd_le_connection_close(_conn_handle);
				}
				break;
			default:
				break;
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
						printLog("SPP Mode ON\r\n");
				        linkPRS();
						// pdm_start();
						// pdm_on = true;
					} else {
						printLog("SPP Mode OFF\r\n");
						_main_state = STATE_CONNECTED;
						SLEEP_SleepBlockEnd(sleepEM2); // Enable sleeping
					}

				}
			}
		}
			break;

		case gecko_evt_gatt_server_attribute_value_id: {
			if (evt->data.evt_gatt_server_attribute_value.value.len > 0) {
				uint32_t t = RTCC_CounterGet();
				sharedCount =
						evt->data.evt_gatt_server_attribute_value.value.data[0];
				printLog("%lu %lu %lu slave server SC: %d\r\n", t,
						t - prev_rtcc, t-prev_shared_count, sharedCount);
				prev_rtcc = t;
				prev_shared_count = prev_rtcc;
//				if (sharedCount>20) {
//					gecko_cmd_le_connection_close(_conn_handle);
//				} else {
//					send_spp_data();
//				}
				send_spp_data();
				gecko_cmd_le_connection_get_rssi(_conn_handle);
			}
		}
			break;
		case gecko_evt_le_connection_rssi_id: {
			uint32_t t = RTCC_CounterGet();
			printLog("%lu %lu %lu rssi: %d\r\n", t,
					t - prev_rtcc, t-prev_shared_count, evt->data.evt_le_connection_rssi.rssi);
			prev_rtcc = t;
			break;
		}
		default:
			printLog("Unhandled event\r\n");
			break;
		}
	}
}

