/*
 * crypto.c
 *
 *  Created on: Aug 26, 2020
 *      Author: nams
 */

#include <crypto_and_time/monocypher.h>
#include <crypto_and_time/x25519-cortex-m4.h>
#include "native_gecko.h"
#include "app.h"

uint8_t private_key[32]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
uint8_t public_key[32];
uint8_t shared_key[32];


void update_public_key(void) {
	struct gecko_msg_system_get_random_data_rsp_t *rnd_response;
	rnd_response = gecko_cmd_system_get_random_data(16);
	memcpy(private_key, rnd_response->data.data, 16);
	rnd_response = gecko_cmd_system_get_random_data(16);
	memcpy(private_key+16, rnd_response->data.data, 16);

	X25519_calc_public_key(public_key, private_key);
}



uint8_t key_exchange_0[] = { 0xa5, 0x46, 0xe3, 0x6b, 0xf0, 0x52, 0x7c, 0x9d, 0x3b, 0x16, 0x15, 0x4b, 0x82, 0x46, 0x5e, 0xdd, 0x62, 0x14, 0x4c, 0x0a, 0xc1, 0xfc, 0x5a, 0x18, 0x50, 0x6a, 0x22, 0x44, 0xba, 0x44, 0x9a, 0xc4, };
#define key_exchange_0_size 32
uint8_t key_exchange_1[] = { 0xe6, 0xdb, 0x68, 0x67, 0x58, 0x30, 0x30, 0xdb, 0x35, 0x94, 0xc1, 0xa4, 0x24, 0xb1, 0x5f, 0x7c, 0x72, 0x66, 0x24, 0xec, 0x26, 0xb3, 0x35, 0x3b, 0x10, 0xa9, 0x03, 0xa6, 0xd0, 0xab, 0x1c, 0x4c, };
#define key_exchange_1_size 32

int test_encrypt(uint8_t *answer) {
    uint8_t       shared_key[32];
    uint32_t start = k_uptime_get();

    int res=0;

    for (int i=0; i<100; i++) {
        X25519_calc_shared_secret(shared_key, key_exchange_0, key_exchange_1);
        // crypto_x25519(shared_key, key_exchange_0, key_exchange_1);
        // crypto_key_exchange(shared_key, key_exchange_0, key_exchange_1);
        res += memcmp(shared_key, answer, 32);
    }
    if (res!=0) {
        return -1; }
    uint32_t stop = k_uptime_get();
    return stop-start;
}
void test_encrypt_compare(void)
{
    int res = 0;
    uint8_t shared_key[32];
    uint8_t shared_key2[32];
    uint8_t public[64];
    memset(public, 0, 64);

    printLog("Hello World! testing crypto\r\n");
    memcpy(public, key_exchange_1, 32);
    X25519_calc_shared_secret(shared_key, key_exchange_0, public);

    crypto_x25519(shared_key2, key_exchange_0, key_exchange_1);
    res += memcmp(shared_key, shared_key2, 32);
    printLog("res: %d\r\n", res);

    if (true) {
        res = test_encrypt(shared_key);
    }
    printLog("time: %d\n", res);


}
