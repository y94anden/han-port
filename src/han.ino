#include "template-render.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define BUF_SIZE 768

/* Format of one frame

/ADN9 6534

0-0:1.0.0(221101214520W)
1-0:1.8.0(00006619.795*kWh)
1-0:2.8.0(00000000.000*kWh)
1-0:3.8.0(00003137.803*kVArh)
1-0:4.8.0(00000269.488*kVArh)
1-0:1.7.0(0000.345*kW)
1-0:2.7.0(0000.000*kW)
1-0:3.7.0(0000.000*kVAr)
1-0:4.7.0(0000.132*kVAr)
1-0:21.7.0(0000.036*kW)
1-0:22.7.0(0000.000*kW)
1-0:41.7.0(0000.105*kW)
1-0:42.7.0(0000.000*kW)
1-0:61.7.0(0000.191*kW)
1-0:62.7.0(0000.000*kW)
1-0:23.7.0(0000.000*kVAr)
1-0:24.7.0(0000.057*kVAr)
1-0:43.7.0(0000.046*kVAr)
1-0:44.7.0(0000.000*kVAr)
1-0:63.7.0(0000.000*kVAr)
1-0:64.7.0(0000.109*kVAr)
1-0:32.7.0(231.5*V)
1-0:52.7.0(229.2*V)
1-0:72.7.0(230.9*V)
1-0:31.7.0(000.3*A)
1-0:51.7.0(000.4*A)
1-0:71.7.0(000.9*A)
!6BB0
*/

// Circular buffer for received data
char han_buffer[BUF_SIZE];
int han_write;
int han_read;
bool han_drop_junk;

// Circular buffer for power history
#define HAN_FULL_HISTORY_BUF_LEN 1530 // 1530 samples = 4.25 h
#define HAN_METER_HISTORY_BUF_LEN (365 * 4 + 1)
uint16_t han_history[HAN_FULL_HISTORY_BUF_LEN];
uint16_t han_phase_history[3][HAN_FULL_HISTORY_BUF_LEN];
uint32_t han_meter_history[HAN_METER_HISTORY_BUF_LEN];
char han_last_meter_date[11];
int han_hist_write;
int han_meter_hist_write;
bool han_hist_wrapped;
bool han_meter_hist_wrapped;

typedef struct {
  char time[20];               // 0-0:1.0.0(221101214520W)
  uint32_t meter_Wh;           // 1-0:1.8.0(00006619795*kWh)
  uint32_t power_W;            // 1-0:1.7.0(0000345*kW)
  uint32_t phasePower_W[3];    // 1-0:21.7.0(0000036*kW), 1-0:41.7.0, 1-0:61.7.0
  uint32_t phaseVoltage_mV[3]; // 1-0:32.7.0(231.5*V), 1-0:52.7.0, 1-0:72.7.0
  uint32_t phaseCurrent_mA[3]; // 1-0:31.7.0(000.3*A), 1-0:51.7.0, 1-0:71.7.0
} Info_t;

Info_t han_last;
Info_t han_tmp;

void han_setup() {
  // Start serial inverted
  Serial.begin(115200, SERIAL_8N1, SERIAL_RX_ONLY, 1, true);
  han_write = 0;
  han_read = 0;

  han_hist_write = 0;
  han_hist_wrapped = false;
  han_meter_hist_write = 0;
  han_meter_hist_wrapped = false;
  han_last_meter_date[0] = '\0';

  // Start by dropping all bytes to first newline
  han_drop_junk = true;

  // Update template field pointers
  tpl_set("time", STRING, &han_last.time);
  tpl_set("meter_Wh", UINT32, &han_last.meter_Wh);
  tpl_set("power_W", UINT32, &han_last.power_W);
  tpl_set("power_L1", UINT32, &han_last.phasePower_W[0]);
  tpl_set("power_L2", UINT32, &han_last.phasePower_W[1]);
  tpl_set("power_L3", UINT32, &han_last.phasePower_W[2]);
  tpl_set("volt_L1", UINT32, &han_last.phaseVoltage_mV[0]);
  tpl_set("volt_L2", UINT32, &han_last.phaseVoltage_mV[1]);
  tpl_set("volt_L3", UINT32, &han_last.phaseVoltage_mV[2]);
  tpl_set("curr_L1", UINT32, &han_last.phaseCurrent_mA[0]);
  tpl_set("curr_L2", UINT32, &han_last.phaseCurrent_mA[1]);
  tpl_set("curr_L3", UINT32, &han_last.phaseCurrent_mA[2]);
}

uint16_t han_16(uint32_t x) {
  if (x > 0xFFFF) {
    // Make sure we do not overshoot the 16-bit history
    x = 0xFFFF;
  }
  return (uint16_t)x;
}

void han_add_meter_sample(uint32_t meter) {
  han_meter_history[han_meter_hist_write] = meter;
  han_meter_hist_write++;
  if (han_meter_hist_write > HAN_METER_HISTORY_BUF_LEN) {
    han_meter_hist_write = 0;
    han_meter_hist_wrapped = true;
  }
}

void han_check_if_time_to_add_meter_sample() {
  char date[11];
  memcpy(date, han_last.time, 10);
  date[10] = '\0';
  if (strcmp(date, han_last_meter_date) != 0 && han_last.time[11] == '0' &&
      han_last.time[12] == '0' && han_last.meter_Wh > 0) {
    // We have a new date, and the hour is 00. Add current sample to meter
    // history
    han_add_meter_sample(han_last.meter_Wh);
    strcpy(han_last_meter_date, date);
  }
}

void han_add_sample() {
  han_history[han_hist_write] = han_16(han_last.power_W);
  for (int i = 0; i < 3; i++) {
    han_phase_history[i][han_hist_write] = han_16(han_last.phasePower_W[i]);
  }

  han_hist_write++;
  if (han_hist_write > HAN_FULL_HISTORY_BUF_LEN) {
    han_hist_write = 0;
    han_hist_wrapped = true;
  }

  // Check if it is time to add new meter sample
  han_check_if_time_to_add_meter_sample();
}

int han_available() {
  while (han_read > BUF_SIZE) {
    han_write -= BUF_SIZE;
    han_read -= BUF_SIZE;
  }
  return han_write - han_read;
}

const char *han_next() {
  if (han_read >= han_write) {
    return NULL;
  }
  return &han_buffer[han_read++];
}

bool han_compare(const char pattern[], bool consume_on_match) {
  int offset = 0;
  while (pattern[offset] != '\0') {
    if (han_read + offset >= han_write) {
      return false;
    }
    if (pattern[offset] != han_buffer[(han_read + offset) % BUF_SIZE]) {
      return false;
    }

    offset++;
  }

  // Ending up here means that all characters in pattern matched.
  if (consume_on_match) {
    han_read += offset;
  }

  return true;
}

void han_parse_time() {
  han_tmp.time[0] = '2';
  han_tmp.time[1] = '0';
  han_tmp.time[4] = '-';
  han_tmp.time[7] = '-';
  han_tmp.time[10] = ' ';
  han_tmp.time[13] = ':';
  han_tmp.time[16] = ':';
  han_tmp.time[19] = '\0';
  uint32_t i;
  for (i = 2; i < 19; i++) {
    if (i == 4 || i == 7 || i == 10 || i == 13 || i == 16) {
      // Separator characters
      continue;
    }
    const char *tmp = han_next();
    if (tmp == NULL) {
      han_tmp.time[0] = '-';
      han_tmp.time[1] = '\0';
      break;
    }
    han_tmp.time[i] = *tmp;
  }
}

uint32_t han_parse_milli_int() {
  uint32_t value = 0;
  int decimals = -1;

  const char *tmp = han_next();
  while (tmp != NULL && decimals < 3) {
    if (*tmp >= '0' && *tmp <= '9') {
      value *= 10;
      value += *tmp - '0';
      if (decimals >= 0) {
        decimals++;
      }
    } else if (*tmp == '.' && decimals == -1) {
      // Start counting decimals
      decimals = 0;
    } else {
      break;
    }
    tmp = han_next();
  }

  // If no decimals was found, set nr found decimals to 0
  if (decimals == -1) {
    decimals = 0;
  }

  // Make sure three decimals are included
  while (decimals < 3) {
    value *= 10;
    decimals++;
  }

  return value;
}

void han_parse() {
  if (han_compare("/", true)) {
    // Meter ID.
    // led_on();
  } else if (han_compare("!", true)) {
    // Checksum. Copy tmp struct to current.
    han_last = han_tmp;
    han_add_sample();
    memset(&han_tmp, 0, sizeof(han_tmp));
    // led_off();
  } else if (han_compare("0-0:1.0.0(", true)) {
    // time
    han_parse_time();
  } else if (han_compare("1-0:1.8.0(", true)) {
    // meter_Wh
    han_tmp.meter_Wh = han_parse_milli_int();
  } else if (han_compare("1-0:1.7.0(", true)) {
    // power_W
    han_tmp.power_W = han_parse_milli_int();
  } else if (han_compare("1-0:21.7.0(", true)) {
    // power_W L1
    han_tmp.phasePower_W[0] = han_parse_milli_int();
  } else if (han_compare("1-0:41.7.0(", true)) {
    // power_W L2
    han_tmp.phasePower_W[1] = han_parse_milli_int();
  } else if (han_compare("1-0:61.7.0(", true)) {
    // power_W L3
    han_tmp.phasePower_W[2] = han_parse_milli_int();
  } else if (han_compare("1-0:32.7.0(", true)) {
    // phaseVoltage_mV L1
    han_tmp.phaseVoltage_mV[0] = han_parse_milli_int();
  } else if (han_compare("1-0:52.7.0(", true)) {
    // phaseVoltage_mV L2
    han_tmp.phaseVoltage_mV[1] = han_parse_milli_int();
  } else if (han_compare("1-0:72.7.0(", true)) {
    // phaseVoltage_mV L3
    han_tmp.phaseVoltage_mV[2] = han_parse_milli_int();
  } else if (han_compare("1-0:31.7.0(", true)) {
    // phaseCurrent_mA L1
    han_tmp.phaseCurrent_mA[0] = han_parse_milli_int();
  } else if (han_compare("1-0:51.7.0(", true)) {
    // phaseCurrent_mA L2
    han_tmp.phaseCurrent_mA[1] = han_parse_milli_int();
  } else if (han_compare("1-0:71.7.0(", true)) {
    // phaseCurrent_mA L3
    han_tmp.phaseCurrent_mA[2] = han_parse_milli_int();
  }

  // Skip until next newline
  const char *tmp = han_next();
  while (tmp != NULL && *tmp != '\n') {
    tmp = han_next();
  }
}

void han_loop() {
  char readByte;

  while (Serial.available()) {
    // Fill one byte in buffer
    readByte = Serial.read();
    han_buffer[han_write % BUF_SIZE] = readByte;
    han_write++;

    // Check that buffer is not full
    if (han_available() >= BUF_SIZE) {
      // Buffer is full. Drop all bytes until next newline
      han_drop_junk = true;
    }

    // Parse received data when we receive a newline
    if (readByte == '\n') {
      han_parse();
      han_drop_junk = false;
    } else if (han_drop_junk) {
      han_read = han_write;
    }
  }
}
