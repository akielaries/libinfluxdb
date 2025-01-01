#ifndef __DATA_H__
#define __DATA_H__

typedef struct InfluxData {
  char *output;
  uint32_t output_size;
  uint32_t output_char;
  char saved_section[64];
  char saved_sub[64];
} InfluxData;

#endif
