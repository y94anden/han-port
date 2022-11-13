#include "template-render.h"
#include <string.h>

extern "C" {
#include "embedded-filesystem/embedded-fs.h"
};

#define MAX_FIELD_NAME_LEN 10

typedef struct {
  char field[MAX_FIELD_NAME_LEN + 1];
  Datatype_t format;
  void *pointer;
} Field_t;

#define MAX_TEMPLATE_FIELDS 30
Field_t tpl_storage[MAX_TEMPLATE_FIELDS];

void tpl_setup() { tpl_clear(); }

void tpl_clear() {
  for (int i = 0; i < MAX_TEMPLATE_FIELDS; i++) {
    tpl_storage[i].field[0] = '\0';
  }
}

int tpl_field_index(PGM_P field) {
  for (int i = 0; i < MAX_TEMPLATE_FIELDS; i++) {
    if (strcmp(field, tpl_storage[i].field) == 0) {
      return i;
    }
  }
  return -1;
}

void tpl_set(PGM_P field, Datatype_t format, void *pointer) {
  int index = tpl_field_index(field);
  if (index == -1) {
    // Field not in storage. Find an empty instead.
    index = tpl_field_index("");
  }

  if (index == -1) {
    // Could not set field.
    return;
  }

  strcpy(tpl_storage[index].field, field);
  tpl_storage[index].format = format;
  tpl_storage[index].pointer = pointer;
}

PGM_P fmt_select(PGM_P s1, PGM_P s2) {
  if (s1[0] != '\0') {
    return s1;
  }
  return s2;
}

char to_byte(PGM_P p) {
  char byte;
  memcpy_P(&byte, p, 1);
  return byte;
}

PGM_P tpl_render(PGM_P p, Sendfunction_t send) {
  char tmp[64];
  char byte = to_byte(p);
  if (byte != '{' || to_byte(p + 1) != '{') {
    send(("&lt;internal rendering error&gt;"), 0);
    return p;
  }
  // Skip the two "{{"
  p++;
  p++;

  char fieldname[MAX_FIELD_NAME_LEN + 1];
  unsigned int i;
  for (i = 0; i < MAX_FIELD_NAME_LEN; i++) {
    byte = to_byte(p);
    if ((byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z') ||
        (byte >= '0' && byte <= '9') || byte == '-' || byte == '_') {
      fieldname[i] = byte;
      p++;
    } else {
      break;
    }
  }
  fieldname[i] = '\0';

  int index = tpl_field_index(fieldname);
  if (index == -1) {
    sprintf(tmp, ("&lt;Field %s not found&gt;"), fieldname);
    send(tmp, 0);
    return p;
  }

  // *p should now be either '}' or ':'.
  byte = to_byte(p);
  if (byte != '}' && byte != ':') {
    sprintf(tmp, ("&lt;Template syntax error near %s&gt;"), fieldname);
    send(tmp, 0);
    return p;
  }

  char fmtstring[10] = {0};

  if (byte == ':') {
    fmtstring[0] = '%';
    p++;
    // Don't overwrite %
    for (i = 1; i < sizeof(fmtstring) - 1; i++) {
      byte = to_byte(p);
      if ((byte >= '0' && byte <= '9') || byte == '.' || byte == 'f' ||
          byte == 'd') {
        fmtstring[i] = byte;
      } else {
        break;
      }
      p++;
    }
  }
  fmtstring[i] = '\0';

  // Now p should point to two "}}". Skip past them.
  if (to_byte(p) != '}' || to_byte(p + 1) != '}') {
    sprintf(tmp, ("&lt;Field %s not closed with double }}&gt;"), fieldname);
    send(tmp, 0);
    return p;
  }
  p++;
  p++;

  const char *ptr = tmp;
  switch (tpl_storage[index].format) {
  case STRING:
    ptr = (const char *)tpl_storage[index].pointer;
    break;
  case FLOAT:
    sprintf(tmp, fmt_select(fmtstring, "%f"),
            *(float *)tpl_storage[index].pointer);
    break;
  case UINT32:
    sprintf(tmp, fmt_select(fmtstring, "%d"),
            *(uint32_t *)tpl_storage[index].pointer);
    break;
  case INT32:
    sprintf(tmp, fmt_select(fmtstring, "%d"),
            *(int32_t *)tpl_storage[index].pointer);
    break;
  case UINT8:
    sprintf(tmp, fmt_select(fmtstring, "%d"),
            *(uint8_t *)tpl_storage[index].pointer);
    break;
  case INT8:
    sprintf(tmp, fmt_select(fmtstring, "%d"),
            *(int8_t *)tpl_storage[index].pointer);
    break;
  case CHAR:
    sprintf(tmp, fmt_select(fmtstring, "%c"),
            *(char *)tpl_storage[index].pointer);
    break;
  default:
    sprintf(tmp, ("&lt;unhandled format (%c) for %s"),
            tpl_storage[index].format, fieldname);
    break;
  }
  send(ptr, 0);

  return p;
}

void tpl_send(PGM_P filename, Sendfunction_t send) {
  PGM_P p = read_file(filename, NULL);
  if (!p) {
    send(("Internal error: Could not read file "), 0);
    send(filename, 0);
  }
  unsigned char byte = to_byte(p);
  while (byte != '\0') {
    if (byte == '{' && to_byte(p + 1) == '{') {
      p = tpl_render(p, send);
    } else {
      send(p, 1);
      p++;
    }
    byte = to_byte(p);
  }
}
