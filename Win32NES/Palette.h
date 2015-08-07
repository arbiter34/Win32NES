#ifndef PALETTE_H
#define PALETTE_H

#include <stdint.h>

typedef struct color {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} color_t;

bool operator==(const color_t& lhs, const color_t& rhs);
bool operator!=(const color_t& lhs, const color_t& rhs);

extern const color_t NES_PALETTE[64];

#endif