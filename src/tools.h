#ifndef __TOOLS_H_
#define __TOOLS_H_

#include <stdint.h>

char *to_hex(const uint8_t *binary, uint32_t binaryLength, // Binary data
             char *string, uint32_t stringLength);         // Output as hex

#endif // __TOOLS_H_