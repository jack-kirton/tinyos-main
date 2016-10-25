#ifndef INCLUDE_MURMUR3_HASH_H
#define INCLUDE_MURMUR3_HASH_H

// From: https://raw.githubusercontent.com/jimon/murmur3unrolled/master/murmur3hash.h

#include <stdint.h>

// -----------------------------------------------------------------------------

// based on https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

inline uint32_t _rotl32(uint32_t x, int8_t r) { return (x << r) | (x >> (-r & 31)); }

#ifndef _MURMUR_SEED
#define _MURMUR_SEED 0x5f3759df
#endif

#define _ROUND32(_d) h = _rotl32(h ^ _rotl32(_d * 0xcc9e2d51, 15) * 0x1b873593, 13) * 5 + 0xe6546b64;

#define _FMIX32(_l) \
	h ^= _l;         \
	h ^= h >> 16;    \
	h *= 0x85ebca6b; \
	h ^= h >> 13;    \
	h *= 0xc2b2ae35; \
	h ^= h >> 16;

inline uint32_t murmur4(const uint32_t * data)
{
	uint32_t h = _MURMUR_SEED;
	_ROUND32(data[0])
	_FMIX32(4)
	return h;
}

inline uint32_t murmur8(const uint32_t * data)
{
	uint32_t h = _MURMUR_SEED;
	_ROUND32(data[0])
	_ROUND32(data[1])
	_FMIX32(8)
	return h;
}

inline uint32_t murmur16(const uint32_t * data)
{
	register uint32_t h = _MURMUR_SEED;
	_ROUND32(data[0])
	_ROUND32(data[1])
	_ROUND32(data[2])
	_ROUND32(data[3])
	_FMIX32(16)
	return h;
}

#endif // INCLUDE_MURMUR3_HASH_H
