/*
 * driver-s6lx75.h
 *
 *  Created on: 09.06.2013
 *      Author: andreas
 */

#ifndef DRIVER_MINIMINER1_H_
#define DRIVER_MINIMINER1_H_

#define MMO_BAUD	115200

struct MMOIdentity
{
	uint8_t version;
	char    product[8];
	uint32_t serial;
} __attribute__((packed));

struct MMOState
{
    uint8_t state;
    uint8_t nonce_valid;
    uint32_t nonce;
} __attribute__((packed));

struct MMOHashData
{
	uint32_t golden_nonce;
	uint32_t nonce;
};

struct MMOInfo
{
	uint32_t baud;

	struct MMOIdentity id;
};

#endif /* DRIVER_S6LX75_H_ */
