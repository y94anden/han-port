#ifndef TEMPLATE_RENDER_H
#define TEMPLATE_RENDER_H

#include <Arduino.h>

typedef enum {
  UINT32,
  INT32,
  UINT8,
  INT8,
  FLOAT,
  STRING,
  CHAR,
} Datatype_t;

typedef void (*Sendfunction_t)(PGM_P p, unsigned int length);

void tpl_setup();
void tpl_clear();
void tpl_send(PGM_P filename, Sendfunction_t send);
void tpl_set(PGM_P field, Datatype_t format, void *pointer);

#endif // TEMPLATE_RENDER_H