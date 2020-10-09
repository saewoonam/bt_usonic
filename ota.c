/*
 * ota.c
 *
 *  Created on: Aug 26, 2020
 *      Author: nams
 */

#include "bg_types.h"
#include "native_gecko.h"
#include "gatt_db.h"
#include "app.h"
#include "spiflash/btl_storage.h"

#include "crypto_and_time/timing.h"

/****************** globals */
// extern uint8 _max_packet_size;
extern uint8 _min_packet_size;
extern uint8 _conn_handle;
extern uint32_t encounter_count;
extern uint32_t chunk_offset;
/* end globals **************/

void send_ota_array(uint8_t *data, uint8_t len) {
	uint16 result;
	printLog("Try to send_ota_array len: %d\r\n", len);
	do {
		result = gecko_cmd_gatt_server_send_characteristic_notification(_conn_handle, gattdb_gatt_spp_data, len, data)->result;
	} while(result == bg_err_out_of_memory);

}
void send_ota_msg(char *msg) {
	uint32_t len = strlen(msg);
	uint16 result;
    printLog("Try to send_ota_msg: %s\r\n", msg);

	do {
		result = gecko_cmd_gatt_server_send_characteristic_notification(_conn_handle, gattdb_gatt_spp_data, len, (uint8_t *) msg)->result;
	} while(result == bg_err_out_of_memory);

}
void send_ota_uint32(uint32_t data) {
	uint32_t len = 4;
	uint16 result;

	do {
		result = gecko_cmd_gatt_server_send_characteristic_notification(_conn_handle, gattdb_gatt_spp_data, len, (uint8_t *) &data)->result;
	} while(result == bg_err_out_of_memory);
}

void send_ota_uint8(uint8_t data) {
	uint32_t len = 1;
	uint16 result;

	do {
		result = gecko_cmd_gatt_server_send_characteristic_notification(_conn_handle, gattdb_gatt_spp_data, len, (uint8_t *) &data)->result;
	} while(result == bg_err_out_of_memory);
}

void send_chunk(uint32_t index, uint32_t offset) {
	uint32_t max;
	uint32_t len = (encounter_count-offset)<<5;
	uint8 data[256];
	uint16 result;
	int chunk_size = _min_packet_size - 4;
    // check if index is in range
	// printLog("send_chunk index: %ld, max_chunk_size %d\r\n", index, chunk_size);
	max = len/chunk_size;
	if ((len%chunk_size)== 0) {
		max = max-1;
	}
	if (index > max) {
		// send over nothing because index is too high return nothing but the index
		memcpy(data, &index, 4);
		do {
			result = gecko_cmd_gatt_server_send_characteristic_notification(_conn_handle, gattdb_gatt_spp_data, 4, data)->result;
		} while(result == bg_err_out_of_memory);
	} else {
		// send over data... two cases... full chunk and partial chunk (240 bytes)
		memcpy(data, &index, 4);
		// figure out actual size of the packet to send... usually 240 except at the end
		int xfer_len = chunk_size;
		if (index==max) {
			// this is likely a partial
			xfer_len = len%chunk_size;
			if (xfer_len==0) xfer_len=chunk_size;
		}
		// Figure out starting point in memory
		uint32_t addr = chunk_size*index + (offset<<5);
		do {
			xfer_len += 4;  // Add length of packet number
			//int32_t retCode =
			storage_readRaw(addr, data+4, xfer_len);
			result = gecko_cmd_gatt_server_send_characteristic_notification(_conn_handle, gattdb_gatt_spp_data, xfer_len, data)->result;
		} while(result == bg_err_out_of_memory);
		/*
		 *
		for(int i=0; i<xfer_len; i++) {
			printLog("%02X ", *(data+4+i));
		}
		printLog("\r\n");
		*/
	}
}
void send_data(void)
{
	uint32_t len = encounter_count<<5;
	uint8 data[256];
	uint16 result;
    int xfer_len = _min_packet_size & 0xFC;  // Make sure it is divisible by 4
    uint32_t start = ts_ms();
	uint32_t addr=0;
	int bad=0;
    int max_data_len = xfer_len - 4;
    int packet_index = 0;

	printLog("packet xfer_len: %d\r\n", xfer_len);
	printLog("len: %ld\r\n", len);
    while (len>0) {
		if (len <= max_data_len) xfer_len = len+4;
		memcpy(data, &packet_index, 4);
		printLog("sent packet_idx %d\r\n", packet_index);
		packet_index++;
		int32_t retCode = storage_readRaw(addr, data+4, xfer_len-4);
		if (retCode)  bad++;
		do {
			result = gecko_cmd_gatt_server_send_characteristic_notification(_conn_handle, gattdb_gatt_spp_data, xfer_len, data)->result;
			// printLog("send result: %u\r\n", result);
		} while(result == bg_err_out_of_memory);

		if (result != 0) {
			printLog("Unexpected error: %x\r\n", result);
		} else {
			;
		}
        addr += (xfer_len-4);
        len -= (xfer_len-4);
    	printLog("addr: %ld, len: %ld\r\n", addr, len);
    	if (packet_index%8 ==0) {
    		// wait(500);
    	}
	}
    printLog("Done xfer time:%ld\r\n",  ts_ms()-start);
	printLog("number of bad memory reads in xfer: %d\r\n", bad);

}

void send_data_turbo(uint32_t start_idx, uint32_t number_of_packets)
{
	uint32_t len = encounter_count<<5;
	uint8 data[256];
	uint16 result;
    int xfer_len = _min_packet_size & 0xFC;  // Make sure it is divisible by 4
    uint32_t start_ts = ts_ms();
	uint32_t addr=0;
	int bad=0;
    int max_data_len = xfer_len - 4;
    int packet_index = 0;

    // calculate max index
    int max_index = len / (xfer_len - 4);
	if (len % (xfer_len - 4) == 0)
		max_index--;
    if (start_idx > max_index) {
    	// don't do anything???
    	return;
	} else {
		// calculate number of bytes to read from memory
		len = number_of_packets * (xfer_len - 4);
		addr = start_idx * (xfer_len - 4);
		// check if len is greater than what is left ot transfer
		if ( (encounter_count<<5) - addr < len) {
			len = (encounter_count<<5) - addr;
		}
		while (len > 0) {
			if (len <= max_data_len)
				xfer_len = len + 4;
			memcpy(data, &packet_index, 4);
			printLog("sent packet_idx %d\r\n", packet_index);
			packet_index++;
			int32_t retCode = storage_readRaw(addr, data + 4, xfer_len - 4);
			if (retCode)
				bad++;
			do {
				result =
						gecko_cmd_gatt_server_send_characteristic_notification(
								_conn_handle, gattdb_gatt_spp_data, xfer_len,
								data)->result;
				// printLog("send result: %u\r\n", result);
			} while (result == bg_err_out_of_memory);

			if (result != 0) {
				printLog("Unexpected error: %x\r\n", result);
			} else {
				;
			}
			addr += (xfer_len - 4);
			len -= (xfer_len - 4);
			printLog("addr: %ld, len: %ld\r\n", addr, len);
		}
		printLog("Done xfer time:%ld\r\n", ts_ms() - start_ts);
		printLog("number of bad memory reads in xfer: %d\r\n", bad);
	}
}


#include "encounter/encounter.h"

extern Encounter_record_v2 encounters[IDX_MASK+1];
void send_encounter(uint32_t index) {
	uint8 data[68];
	memcpy(data, &index, 4);
	if (index < 0xFFFFFFF) {  // 0xFFFFFFFF is not a great marker.. TODO need to fix this
		memcpy(data + 4, encounters + (index & IDX_MASK), 32);
	} else {
		memset(data + 4, 0, 32); // send end of data marker
	}
	uint16 result;
	do {
		result = gecko_cmd_gatt_server_send_characteristic_notification(
				_conn_handle, gattdb_gatt_spp_data, 36, data)->result;
	} while (result == bg_err_out_of_memory);
		/*
		 *
		for(int i=0; i<xfer_len; i++) {
			printLog("%02X ", *(data+4+i));
		}
		printLog("\r\n");
		*/
}
