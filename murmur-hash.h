/*
 * murmur-hash.h
 *
 *  Created on: Dec 18, 2016
 *      Author: awilde
 */

#ifndef SRC_MURMUR_HASH_H_
#define SRC_MURMUR_HASH_H_
#include <stdint.h>

uint32_t murmur3_32(const uint8_t* key, size_t len, uint32_t seed);

#endif /* SRC_MURMUR_HASH_H_ */
