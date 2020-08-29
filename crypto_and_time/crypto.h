/*
 * crypto.h
 *
 *  Created on: Aug 26, 2020
 *      Author: nams
 */

#ifndef CRYPTO_AND_TIME_CRYPTO_H_
#define CRYPTO_AND_TIME_CRYPTO_H_

extern uint8_t private_key[32];
extern uint8_t public_key[32];
extern uint8_t shared_key[32];

void update_public_key(void);


#endif /* CRYPTO_AND_TIME_CRYPTO_H_ */
