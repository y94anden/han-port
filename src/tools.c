#include "tools.h"

char *to_hex(const uint8_t *binary, uint32_t binaryLength, char *string,
             uint32_t stringLength) {
  uint32_t pos = 0;
  uint8_t nibble;
  while (pos < (stringLength - 1) && (pos / 2) < binaryLength) {
    if (pos & 1) {
      // Odd number. Use lower nibble.
      nibble = binary[pos / 2] & 0x0F;
    } else {
      // Even number. User upper nibble.
      nibble = (binary[pos / 2] >> 4) & 0x0F;
    }
    if (nibble >= 0xA) {
      string[pos] = 'A' + (nibble - 0xA);
    } else {
      string[pos] = '0' + nibble;
    }
    pos++;
  }
  string[pos] = '\0';
  return string;
}
