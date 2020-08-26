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

void newRandAddress(void) {
  bd_addr rnd_addr;
  struct gecko_msg_le_gap_set_advertise_random_address_rsp_t *response;
  // struct gecko_msg_le_gap_stop_advertising_rsp_t *stop_response;
  //struct gecko_msg_le_gap_set_advertise_configuration_rsp_t *config_response;
  struct gecko_msg_system_get_random_data_rsp_t *rnd_response;

  //stop_response =
  gecko_cmd_le_gap_stop_advertising(HANDLE_ADV);
  // printLog("stop response: %d\r\n", stop_response->result);
  // config_response = gecko_cmd_le_gap_set_advertise_configuration(HANDLE_ADV, 0x04);
  // printLog("config response: %d\r\n", config_response->result);

  do {
	  rnd_response = gecko_cmd_system_get_random_data(6);
	  // printLog("rnd response: %d %d\r\n", rnd_response->result, rnd_response->data.len);
	  rnd_response->data.data[5] &= 0x3F;  // Last two bits must 0 for valid random mac address
	  // rnd_response->data.data[0] = 0xFF;  // Make it easier to debug

	  memcpy(rnd_addr.addr, rnd_response->data.data, 6);
	  /*
	  printLog("new    board_addr    :   %02x:%02x:%02x:%02x:%02x:%02x\r\n",
	  rnd_addr.addr[5],
	  rnd_addr.addr[4],
	  rnd_addr.addr[3],
	  rnd_addr.addr[2],
	  rnd_addr.addr[1],
	  rnd_addr.addr[0]
					);
					*/
	  response = gecko_cmd_le_gap_set_advertise_random_address(HANDLE_ADV, 0x03, rnd_addr);
	  // printLog("set random address respose %ld, 0x%X\r\n", ts_ms(), response->result);
  } while(response->result>0);
  /*
   printLog("random board_addr %3d:   %02x:%02x:%02x:%02x:%02x:%02x\r\n", response->result,
  response->address_out.addr[5],
  response->address_out.addr[4],
  response->address_out.addr[3],
  response->address_out.addr[2],
  response->address_out.addr[1],
  response->address_out.addr[0]
  );
  */

  gecko_cmd_le_gap_start_advertising(HANDLE_ADV, le_gap_user_data, le_gap_non_connectable);
}

void bcnSetupAdvBeaconing(void)
{

  /* This function sets up a custom advertisement package according to iBeacon specifications.
   * The advertisement package is 30 bytes long. See the iBeacon specification for further details.
   */

  //
  // uint8_t len = sizeof(etAdvData);
  // uint8_t *pData = (uint8_t*)(&etAdvData);
  // bd_addr rnd_addr;
  // struct gecko_msg_le_gap_set_advertise_random_address_rsp_t *response;
  /* Set custom advertising data */
  update_public_key();
  update_adv_key();
  // gecko_cmd_le_gap_bt5_set_adv_data(HANDLE_ADV, 0, len, pData);

  /* Set 0 dBm Transmit Power */
  gecko_cmd_le_gap_set_advertise_tx_power(HANDLE_ADV, 0);
  gecko_cmd_le_gap_set_advertise_configuration(HANDLE_ADV, 0x04);
  /*
   *
  rnd_addr.addr[0] = 1;
  rnd_addr.addr[1] = 2;
  rnd_addr.addr[2] = 3;
  rnd_addr.addr[3] = 4;
  rnd_addr.addr[4] = 5;
  rnd_addr.addr[5] = 6;

  response = gecko_cmd_le_gap_set_advertise_random_address(HANDLE_ADV, 0x03, rnd_addr);

  printLog("board_addr:   %02x:%02x:%02x:%02x:%02x:%02x\r\n",
  rnd_addr.addr[5],
  rnd_addr.addr[4],
  rnd_addr.addr[3],
  rnd_addr.addr[2],
  rnd_addr.addr[1],
  rnd_addr.addr[0]
  );

  printLog("random board_addr %d:   %02x:%02x:%02x:%02x:%02x:%02x\r\n", response->result,
  response->address_out.addr[5],
  response->address_out.addr[4],
  response->address_out.addr[3],
  response->address_out.addr[2],
  response->address_out.addr[1],
  response->address_out.addr[0]
  );
  */

  /* Set advertising parameters. 200ms (320/1.6) advertisement interval. All channels used.
   * The first two parameters are minimum and maximum advertising interval, both in
   * units of (milliseconds * 1.6).  */
  gecko_cmd_le_gap_set_advertise_timing(HANDLE_ADV, 320, 320, 0, 0);

  /* Start advertising in user mode */
  gecko_cmd_le_gap_start_advertising(HANDLE_ADV, le_gap_user_data, le_gap_non_connectable);
  // gecko_cmd_hardware_set_soft_timer(32768*60,HANDLE_ADV,0);
  // gecko_cmd_hardware_set_soft_timer(32768,HANDLE_UPDATE_KEY,0);
  //gecko_cmd_hardware_set_soft_timer(32768>>2,HANDLE_UART,0);
  // gecko_cmd_hardware_set_soft_timer(32768,HANDLE_LED,0);

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
