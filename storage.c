/*
 * storage.c
 *
 *  Created on: Aug 26, 2020
 *      Author: nams
 */

#include "crypto_and_time/timing.h"

#include "encounter/encounter.h"

#include "spiflash/btl_storage.h"

#include "bg_types.h"
#include "native_gecko.h"
#include "gatt_db.h"
#include "app.h"

#include "em_core.h"

/***************************************************************************************************
 SERIOUS FIXME DECLARATIONS
 **************************************************************************************************/
// #define gattdb_count 0


/***************************************************************************************************
 Global Variables
 **************************************************************************************************/

extern Encounter_record_v2 encounters[IDX_MASK+1];
extern uint32_t c_fifo_last_idx;
extern uint32_t p_fifo_last_idx;

extern uint32_t encounter_count;
extern bool write_flash;

extern struct my_encounter_index _encounters_tracker;

/***************************************************************************************************
external functions
 **************************************************************************************************/
void update_counts_status(void);

/***************************************************************************************************
typedefs
 **************************************************************************************************/

typedef struct flash_buf_t flash_buf_t;
typedef flash_buf_t* flash_handle_t;
struct flash_buf_t {
	uint32_t head;
	uint32_t tail;
	uint32_t max; //of the buffer
	bool full;
};
flash_buf_t flash_buf;

/***************************************************************************************************
 functions
 **************************************************************************************************/


int in_encounters_fifo(const uint8_t * mac, uint32_t epoch_minute) {
    /*
     *
     * Search backward during the "last minutes" worth of encounters
     *
     */
	// TODO deal with rollover of c_fifo_last_idx
    Encounter_record_v2 *current_encounter;
    if (c_fifo_last_idx == 0) return -1;
    int start = c_fifo_last_idx - 1;
    do {
        current_encounter = encounters + (start & IDX_MASK);
        if (current_encounter->minute < epoch_minute) return -1;
        if (memcmp(current_encounter->mac, mac, 6) == 0) return start;
        start--;
    } while (start>=0);
    return -1;
}

int search_encounters_em(uint32_t epoch_minute) {
    /*
     *
     * Search backward for encounters for events with epoch_minute
     *
     */
	// TODO deal with rollover of c_fifo_last_idx
    Encounter_record_v2 *current_encounter;
    if (c_fifo_last_idx == 0) return -1;
    int start = c_fifo_last_idx - 1;
    int count=0;
    do {
        current_encounter = encounters + (start & IDX_MASK);
        if (current_encounter->minute < epoch_minute) return (start+1);
        if (start==0) return 0;
        start--;
        count++;
        if (count==IDX_MASK) return start;
    } while (start>=0);
    return -1;
}

uint32_t determine_counts(uint32_t flash_size) {
	uint32_t count=0;
	bool empty=true;
	// printf("flash_size<<5: %ld\r\n", (flash_size>>5));
	do {
		empty = verifyErased(count<<5, 32);
		if (!empty) count++;
		// printLog("count:%lu\r\n", count);
	} while (!empty & (count<(flash_size>>5)));
	return count;
}

bool verifyMark(uint32_t address, uint32_t len, uint8_t mark);

void find_mark_in_flash(uint8_t mark) {
	int count=encounter_count;
	bool found=false;
	do {
		// printLog("find_mark_in_flash: %d\r\n", count);
		found = verifyMark(count<<5, 32, mark);
		if (!found) count--;
	} while (!found & (count >= 0));
	printLog("done: find_mark_in_flash: %d\r\n", count);

	_encounters_tracker.start_upload = ++count;  // first element after mark
}

//void test_write_flash() {
//	uint32_t addr=0;
//	uint8_t buffer[32];
//	for(int i=0; i<32; i++) {
//		buffer[i] = 32-i;
//	}
//	int32_t retCode = storage_writeRaw(addr, buffer, 32);
//	printLog("storage_writeRaw: %ld\r\n", retCode);
//    send32bytes(buffer);
//}

void flash_erase() {
	printLog("Trying to erase\r\n");
	int num_sectors = encounter_count >> 7;
	if (4095 & (encounter_count<<5)) num_sectors++;
	int32_t retCode = storage_eraseRaw(0, num_sectors<<12);
    printLog("erase size: %d\r\n", num_sectors<<12);
	// int32_t retCode = storage_eraseRaw(0, encounter_count<<5);
	// int32_t retCode = storage_eraseRaw(0, 1<<20);
	if (retCode) {
		printLog("Problem with erasing flash. %ld\r\n", retCode);
	} else {
		encounter_count = 0;
		_encounters_tracker.start_upload = 0;
		update_counts_status();
		// gecko_cmd_gatt_server_write_attribute_value(gattdb_count, 0, 4, (const uint8*) &encounter_count);
		// untransferred_start = 0;
	}
	// bool empty = verifyErased(0, 1<<20);
	// printLog("verify erased: %d\r\n", empty);
}

int32_t storage_writeRaw_noCheck(uint32_t address, uint8_t *data, size_t numBytes);

void store_zeros_random_addr(uint32_t address) {
	uint8_t temp[32];
	memset(temp, 0, 32);
	int32_t retCode = storage_writeRaw_noCheck(address<<5, temp, 32);
	/*for (int i=0; i<32; i++) {
		printLog("%02X ", event[i]);
	}
	printLog("\r\n");*/
	if (retCode) {
		printLog("Error storing zeros to flash at addr %ld: %ld\r\n", address, retCode);
	}

}

void store_event(uint8_t *event) {
//    CORE_DECLARE_IRQ_STATE;
//    CORE_ENTER_ATOMIC();

	if (encounter_count<(1<<(20-5))) {
		int32_t retCode = storage_writeRaw(encounter_count<<5, event, 32);
		/*for (int i=0; i<32; i++) {
			printLog("%02X ", event[i]);
		}
		printLog("\r\n");*/
		if (retCode) {
			printLog("Error storing to flash %ld\r\n", retCode);
		}
		encounter_count++;
		printLog("Storing Encounter_count: %ld\r\n", encounter_count);
		update_counts_status();
	} else {
		/* Need to indicate that memory is full */
		;
	}
//    CORE_EXIT_ATOMIC(); // Enable interrupts
}

//void store_time() {
void store_time(uint8_t *data, uint8_t len) {
    uint8_t time_evt[32];
	memset(time_evt, 0, 32);
	memcpy(time_evt+4, &_time_info.epochtimesync, 4);
	memcpy(time_evt+8, &_time_info.offset_time, 4);
	memcpy(time_evt+12, &_time_info.offset_overflow, 4);
	memset(time_evt+16, 0xFF, 16);
	if (len>0) {
		if (len>16) len=16;
		memcpy(time_evt+16, data, len);
	}
	/*
	_time_info.gottime = true;
	_time_info.epochtimesync = epochtimesync;
	_time_info.next_minute = next_minute;
	_time_info.offset_overflow = offset_overflow;
	_time_info.offset_time = offset_time;
*/
    store_event(time_evt);
}

void store_special_mark(uint8_t num){
    uint8_t evt[32];
	memset(evt, num, 32);
    store_event(evt);
}

void read_encounters_tracking(void) {
	uint16 key = 0x4001;
	struct gecko_msg_flash_ps_load_rsp_t *result;

	result = gecko_cmd_flash_ps_load(key);
	if (result->result==0) {
		memcpy(&_encounters_tracker, result->value.data, result->value.len);
	} else {
		printLog("Problem reading encounters_tracker fromo persistent storage\r\n");
	}
}

void write_encounters_tracking(void) {
	uint16_t key = 0x4001;
	int result = gecko_cmd_flash_ps_save(key, 8, (uint8_t *)&_encounters_tracker)->result;
    printLog("write_encounters_tracking result 0x%04X\r\n", result);
}

extern struct {
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
} etAdvData2;

void read_name_ps(void) {
	uint16 k = 0x4000;
	uint8_t *name;
	struct gecko_msg_flash_ps_load_rsp_t *result;
    result = gecko_cmd_flash_ps_load(k);
    printLog("read_name_ps 0x%04X\r\n", result->result);

    if (result->result==0) {
    	if (result->value.len==8) {
    		name = result->value.data;
    		gecko_cmd_gatt_server_write_attribute_value(gattdb_device_name, 0, 8, name);
    		printLog("retrieved name: ");
    		for(int i=0; i<8; i++) {
    			printLog("%c", name[i]);
    		}
    		printLog("\r\n");
    		// Update advData2
    		memcpy(etAdvData2.name, name+4, 4);
    	} else { printLog("Problem reading BT name\r\n"); }
    }
}

void write_name_ps(uint8_t *name) {
	uint16_t key = 0x4000;
	int result = gecko_cmd_flash_ps_save(key, 8, name)->result;
    printLog("write_name_ps 0x%04X\r\n", result);

}

void set_name(uint8_t *name) {
	// uint8_t buffer[255];
	/*
	struct gecko_msg_gatt_server_read_attribute_value_rsp_t *result;
	result = gecko_cmd_gatt_server_read_attribute_value(gattdb_device_name, 0);
	printLog("%d, %d\r\n",  result->result, result->value.len);
	memset(buffer, 0, 255);
	memcpy(buffer, result->value.data, result->value.len);
	printLog("name: %s\r\n", buffer);
	memset(buffer, '0', 8);
	printLog("test zero pad:%08d\r\n", 0);
	*/
	write_name_ps(name);
	gecko_cmd_gatt_server_write_attribute_value(gattdb_device_name, 0, 8, name);
	/*
	result = gecko_cmd_gatt_server_read_attribute_value(gattdb_device_name, 0);
	printLog("result: %d, %d\r\n",  result->result, result->value.len);
	memset(buffer, 0, 255);
	memcpy(buffer, result->value.data, result->value.len);
	printLog("name: %s\r\n", buffer);
	*/
}
//void start_writing_flash() {
void start_writing_flash(uint8_t *data, uint8_t len) {
	write_flash = true;
	p_fifo_last_idx = c_fifo_last_idx;
	printLog("Start writing to flash\r\n");
	uint8_t blank[32];
	memset(blank, 0, 32);
	store_event(blank);
	// only encounter mode
	// if (mode==MODE_ENCOUNTER) {
		store_event(blank);
	// }
	//store_time();
	store_time(data, len);

}

void flash_store(void) {
    Encounter_record_v2 *current_encounter;
    // uint32_t start = p_fifo_last_idx;
    uint32_t timestamp = ts_ms();
    uint32_t epoch_minute = ((timestamp-_time_info.offset_time) / 1000 + _time_info.epochtimesync)/60;
    /*
    printLog("flash_store, epoch_minute: %ld, p_idx: %ld, c_idx: %ld\n", epoch_minute,
                    p_fifo_last_idx, c_fifo_last_idx);
	*/
    while (c_fifo_last_idx > p_fifo_last_idx) {

        current_encounter = encounters + (p_fifo_last_idx & IDX_MASK);
        // uart_printf("idx: %d, minute: %d\n", p_fifo_last_idx, current_encounter->minute);
        if (current_encounter->minute < epoch_minute) { // this is an old record write to flash
        	uint8_t *ptr;
        	ptr = (uint8_t *) current_encounter;
        	/*
        	send32bytes(ptr);
        	send32bytes(ptr+32);
        	*/
        	store_event(ptr);
        	store_event(ptr+32);
        	// print_encounter(p_fifo_last_idx & IDX_MASK);

        	p_fifo_last_idx++;
        } else {
            // uart_printf("Done flash_store looking back\n");
            return;
        }
    }
}


// inspired by https://github.com/embeddedartistry/embedded-resources/blob/master/examples/c/circular_buffer/circular_buffer.c

void flash_buf_reset(flash_handle_t fbuf) {
	fbuf->head = 0;
	fbuf->tail = 0;
	fbuf->full = false;
}

/*
 *
flash_handle_t flash_buf_init(size_t size) {
	flash_handle_t fbuf = malloc(sizeof(struct flash_buf_t));
 *
 */
void flash_buf_init(size_t size) {
	flash_buf.max = size;
	flash_buf_reset(&flash_buf);
}

static void advance_pointer(flash_handle_t fbuf) {
	if (fbuf->full) {
		// Handle full memory
	} else {
		fbuf->head = (fbuf->head + 1) % fbuf->max;
		fbuf->full = (fbuf->head == fbuf->tail);
	}
}

static void retreat_pointer(flash_handle_t fbuf) {
	fbuf->full = false;
	fbuf->tail = (fbuf->tail + 1) % fbuf->max;
}

bool flash_buf_empty(flash_handle_t fbuf) {
	return (!fbuf->full && (fbuf->head == fbuf->tail));
}

void flash_buf_put(flash_handle_t fbuf, uint8_t *event) {
	if (!fbuf->full) {
		int32_t retCode = storage_writeRaw((fbuf->head) << 5, event, 32);
		if (retCode) {
			printLog("Error storing to flash %ld\r\n", retCode);
		}
		printLog("Storing Encounter_count: %ld\r\n", fbuf->head);
		update_counts_status();
		advance_pointer(fbuf);
	} else {
		//TODO handle this case properly
		printLog("FIFO is full\r\n");
	}
}

/* clear froom flash buffer... */
void flash_buf_zero(flash_handle_t fbuf) {
	if (!flash_buf_empty(fbuf)) {
		store_zeros_random_addr(fbuf->tail);
		retreat_pointer(fbuf);
	}
}

void time_flash_scan_memory() {
	uint32_t start = ts_ms();
	int sum = 0;
	for (int i=0; i<32768; i++) {
		sum += verifyErased(i<<5, 32);
	}
	printLog("sum: %d, elapsed time: %ld\r\n", sum, ts_ms()-start);
}

int coarse_search() {
    int index = 0;
    int N = 15; // 32768
    index = 0;
    // printLog("index: %d\r\n", index);
    if (verifyErased(index<<5, 32)) return index;
    index = (1<<(N-1));
    // printLog("index: %d\r\n", index);
    if (verifyErased(index<<5, 32)) return index;
    for(int i=N-2; i>=0; i--) {
        index = (1<<i);
        for(int j=0; j<(1<<(N-1-i)); j++) {
            // printLog("index: %d\r\n", index);
            if (verifyErased(index<<5, 32)) return index;
            index += (1<<(i+1));
        }
    }
    return -1;
}

uint32_t fine_search(uint32_t left, uint32_t right) {
  uint32_t mid, *a, *b;
  // printf("%d %d\r\n", left, right);
  a = &left;
  b = &right;
  do {
    mid = (left+right) >> 1;
    // printf("%d %d %d\r\n", left, right, mid);
    if (verifyErased(mid<<5, 32)) *b = mid;
    else *a = mid;
  } while ((right-left)>1);
  return right;
}
/*
uint32_t find_head(flash_handle_t fbuf, uint32_t early, uint32_t late) {
	uint32_t first=0;
	uint32_t last= (fbuf->max)>>6;  // look halfway across circular buffer

	bool empty;
	empty = verifyErased(first<<5, 32);
	if (empty) return first; // need to look between last to first+ (fbuf->max)
	empty = verifyErased(last<<5, 32); // need to look between first to last
	if (empty) return last;

	uint32_t mid = (first+last) >> 1;
	if (verifyErased(mid<<5, 32)) return mid;


}
*/

