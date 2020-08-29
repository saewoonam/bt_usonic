/*
 * ota.h
 *
 *  Created on: Aug 28, 2020
 *      Author: nams
 */

#ifndef OTA_H_
#define OTA_H_

void send_ota_msg(char *msg);
void send_ota_uint8(uint8_t data);
void send_ota_uint32(uint32_t data);
void send_data(void);
void send_data_turbo(uint32_t start_idx, uint32_t number_of_packets);
void send_chunk(uint32_t index);

#endif /* OTA_H_ */
