#include "template-render.h"
#include <ESP8266WebServer.h>

extern "C" {
#include "embedded-filesystem/embedded-fs.c"
}

ESP8266WebServer server(80);
char ws_sub_path[256];
char tmp_buf[8192];
unsigned int tmp_buf_write;
bool ws_nothing_sent_yet;

void webserver_setup() {
  server.on(F("/"), ws_handle_root);
  server.on(F("/about"), ws_handle_about);
  server.on(F("/history"), ws_handle_hist);
  server.on(F("/meter"), ws_handle_meter);
  server.on(F("/restart"), ws_handle_restart);
  server.on(F("/raw"), ws_handle_raw);
  server.on(F("/rest/current"), ws_handle_rest_last);
  server.on(F("/rest/gpio"), ws_handle_rest_gpio);
  server.on(F("/rest/entropy"), ws_handle_rest_entropy);
  server.on(F("/rest/seed"), ws_handle_seed);
  server.on(F("/rest/seed/new"), ws_handle_seed_new);
  server.on(F("/history/full"), ws_handle_history_full);
  server.on(F("/history/1"), ws_handle_history_1);
  server.on(F("/history/2"), ws_handle_history_2);
  server.on(F("/history/3"), ws_handle_history_3);
  server.on(F("/history/meter"), ws_handle_history_meter);
  server.on(F("/history/meter/add"), ws_handle_add_meter);
  server.on(F("/simulate/meter"), ws_handle_simulate_meter_history);

  server.onNotFound(ws_handle_not_found);

  // To get more headers than the two standard ("Authorization" and
  // "If-None-Match"), the desired headers must be supplied
  const char *headers[] = {("X-Sub-Path")};
  server.collectHeaders(headers, sizeof(headers) / sizeof(char *));

  // Default subpath is empty until we find a header "X-Sub-Path", in
  // which case we replace it
  ws_sub_path[0] = '\0';
  tpl_set(("X-Sub-Path"), STRING, ws_sub_path);

  server.begin(); // Actually start the server
}

void webserver_loop() { server.handleClient(); }

void ws_init_tmp_buffer() {
  ws_nothing_sent_yet = true;
  tmp_buf_write = 0;
}

void ws_send_tmp_buffer(bool final) {
  if (ws_nothing_sent_yet) {
    // Set headers and start the transmission
    if (final) {
      server.setContentLength(tmp_buf_write);
    } else {
      server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    }
    server.send(200, "text/html", "");
    ws_nothing_sent_yet = false;
  }

  if (tmp_buf_write > 0) {
    server.sendContent(tmp_buf, tmp_buf_write);
  }

  if (final) {
    server.sendContent(F(""));
    server.client().stop();
  }

  tmp_buf_write = 0;
}

void ws_send(PGM_P p, unsigned int length) {
  if (length == 0) {
    length = strlen_P(p);
  }

  while (tmp_buf_write + length > sizeof(tmp_buf)) {
    // buffer is full. Copy as much as fits into the buffer and send it.
    unsigned int reduced_len = sizeof(tmp_buf) - tmp_buf_write;
    memcpy_P(&tmp_buf[tmp_buf_write], p, reduced_len);
    tmp_buf_write += reduced_len;

    ws_send_tmp_buffer(false);

    length -= reduced_len; // Reduce the amount left to write
    p += reduced_len;      //...and move the pointer forward

    // Now. Either the rest fits in the buffer, and we exit the while loop.
    // Otherwise we run it again, sending another full buffer.
  }

  // Copy any remaining data into the buffer
  if (length > 0) {
    memcpy_P(&tmp_buf[tmp_buf_write], p, length);
    tmp_buf_write += length;
  }
}

void ws_render_html(const char *filename) {
  if (server.hasHeader(F("X-Sub-Path"))) {
    strcpy(ws_sub_path, server.header(F("X-Sub-Path")).c_str());
  } else {
    ws_sub_path[0] = '\0';
  }
  ws_init_tmp_buffer();
  tpl_send(filename, ws_send);

  // tpl_send() has now used ws_send() to fill the buffer (and possibly sent
  // some data prematurely). Now send what is left in the buffer.
  ws_send_tmp_buffer(true);
}

void ws_handle_root() { ws_render_html(("index.html")); }
void ws_handle_hist() { ws_render_html(("history.html")); }
void ws_handle_meter() { ws_render_html(("meter.html")); }
void ws_handle_about() { ws_render_html(("about.html")); }

void ws_handle_restart() {
  server.send(200, F("text/plain"), F("restarting..."));
  delay(500);
  ESP.restart();
}

void ws_handle_raw() {
  char buf[BUF_SIZE + 1];
  for (int i = 0; i < BUF_SIZE; i++) {
    buf[i] = han_buffer[(i + han_write) % BUF_SIZE];
  }
  buf[BUF_SIZE] = 0;
  server.send(200, "text/plain", buf);
}

void send_milli_value(uint32_t value) {
  char tmp[64];
  uint32_t wholenumber = value / 1000;
  sprintf(tmp, "%d.%03d", wholenumber, value - wholenumber * 1000);
  server.sendContent(tmp);
}

void ws_handle_rest_last() {
  if (han_last.time[0] == '\0') {
    server.send(200, "text/json", "{\"error\":\"no data yet\"}");
    return;
  }
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.sendHeader("Cache-Control", String("no-cache"));
  server.send(200, "text/json", "{");

  server.sendContent(F("\"time\":\""));
  server.sendContent(han_last.time);
  server.sendContent(F("\""));

  server.sendContent(F(",\"meter\":"));
  send_milli_value(han_last.meter_Wh);

  server.sendContent(F(",\"power\":"));
  send_milli_value(han_last.power_W);

  server.sendContent(F(",\"power\":["));
  send_milli_value(han_last.phasePower_W[0]);
  server.sendContent(F(","));
  send_milli_value(han_last.phasePower_W[1]);
  server.sendContent(F(","));
  send_milli_value(han_last.phasePower_W[2]);
  server.sendContent(F("]"));

  server.sendContent(F(",\"voltage\":["));
  send_milli_value(han_last.phaseVoltage_mV[0]);
  server.sendContent(F(","));
  send_milli_value(han_last.phaseVoltage_mV[1]);
  server.sendContent(F(","));
  send_milli_value(han_last.phaseVoltage_mV[2]);
  server.sendContent(F("]"));

  server.sendContent(F(",\"current\":["));
  send_milli_value(han_last.phaseCurrent_mA[0]);
  server.sendContent(F(","));
  send_milli_value(han_last.phaseCurrent_mA[1]);
  server.sendContent(F(","));
  send_milli_value(han_last.phaseCurrent_mA[2]);
  server.sendContent(F("]"));

  server.sendContent(F("}"));

  server.sendContent(F(""));
  server.client().stop();
}

void ws_handle_rest_gpio() {
  if (server.hasArg("on")) {
    gpio_on();
  } else if (server.hasArg("off")) {
    gpio_off();
  }

  sprintf(tmp_buf, "{\"gpio\":%s}", gpio_state() ? "true" : "false");
  server.setContentLength(strlen(tmp_buf));
  server.sendHeader("Cache-Control", String("no-cache"));
  server.send(200, "text/json", tmp_buf);
  server.sendContent(F(""));
  server.client().stop();
}

void ws_handle_rest_entropy() {
  char buf[65];
  entropy_get_hex(buf, sizeof(buf));

  sprintf(tmp_buf, "{\"entropy\":\"%s\"}", buf);
  server.setContentLength(strlen(tmp_buf));
  server.sendHeader("Cache-Control", String("no-cache"));
  server.send(200, "text/json", tmp_buf);
  server.sendContent(F(""));
  server.client().stop();
}

void ws_handle_seed() {
  char buf[65];
  entropy_seed_get_hex(buf, sizeof(buf));

  sprintf(tmp_buf, "{\"seed\":\"%s\"}", buf);
  server.setContentLength(strlen(tmp_buf));
  server.sendHeader("Cache-Control", String("no-cache"));
  server.send(200, "text/json", tmp_buf);
  server.sendContent(F(""));
  server.client().stop();
}

void ws_handle_seed_new() {
  entropy_seed_new();
  ws_handle_seed();
}

void ws_handle_history(bool wrapped, uint32_t write, uint32_t buflen,
                       uint8_t *buf, uint8_t datasize) {
  int len = wrapped ? buflen : write;
  server.setContentLength(len * datasize); // 8, 16 or 32 bit data

  server.sendHeader("Cache-Control", String("no-cache"));
  server.sendHeader("Access-Control-Allow-Origin", String("*"));

  server.send(200, "application/octet-stream", "");
  if (wrapped) {
    // Send the oldest data (that is soon to be overwritten)
    server.sendContent((const char *)&buf[write * datasize],
                       (buflen - write) * datasize);
  }
  // Send the data from 0 and up until latest.
  server.sendContent((const char *)buf, write * datasize);
  server.sendContent(F(""));
  server.client().stop();
}

void ws_handle_history_full() {
  ws_handle_history(han_hist_wrapped, han_hist_write, HAN_FULL_HISTORY_BUF_LEN,
                    (uint8_t *)han_history, 2);
}

void ws_handle_history_1() {
  ws_handle_history(han_hist_wrapped, han_hist_write, HAN_FULL_HISTORY_BUF_LEN,
                    (uint8_t *)han_phase_history[0], 2);
}

void ws_handle_history_2() {
  ws_handle_history(han_hist_wrapped, han_hist_write, HAN_FULL_HISTORY_BUF_LEN,
                    (uint8_t *)han_phase_history[1], 2);
}

void ws_handle_history_3() {
  ws_handle_history(han_hist_wrapped, han_hist_write, HAN_FULL_HISTORY_BUF_LEN,
                    (uint8_t *)han_phase_history[2], 2);
}

void ws_handle_history_meter() {
  ws_handle_history(han_meter_hist_wrapped, han_meter_hist_write,
                    HAN_METER_HISTORY_BUF_LEN, (uint8_t *)han_meter_history, 4);
}

void ws_handle_add_meter() {
  if (han_last_meter_date[0] != '\0') {
    // The server has added meter settings already. Refuse any more.
    server.send(418, "text/plain", "I'm a teapot");
    return;
  }

  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "No payload");
    return;
  }

  String payload = "";
  payload += server.arg("plain");
  uint32_t meterValue = payload.toInt();

  if (meterValue == 0) {
    server.send(400, "text/plain", "Payload not an integer");
    return;
  }
  han_add_meter_sample(meterValue);
  sprintf(tmp_buf, "thanx, we now have %d entries", han_meter_hist_write);
  server.send(200, "text/plain", tmp_buf);
}

void ws_handle_simulate_meter_history() {
  han_meter_history[0] = 6830555;
  han_meter_history[1] = 6831555;
  han_meter_history[2] = 6832555;
  han_meter_history[3] = 6833555;
  han_meter_history[4] = 6834555;
  han_meter_history[5] = 6835555;
  han_meter_history[6] = 6836555;
  han_meter_history[7] = 6837555;
  han_meter_history[8] = 6838555;
  han_meter_history[9] = 6839555;
  han_meter_history[10] = 6840555;
  han_meter_hist_write = 11;
  server.send(200, "text/plain", "meter history added");
}

String get_content_type(String uri) {
  int dotIndex = uri.lastIndexOf(".");
  if (dotIndex == -1) {
    return "text/plain";
  }

  String extension = uri.substring(dotIndex);

  if (extension == ".html") {
    return "text/html";
  }

  if (extension == ".css") {
    return "text/css";
  }

  if (extension == ".js") {
    return "text/javascript";
  }

  if (extension == ".jpg" || extension == ".jpeg") {
    return "image/jpeg";
  }

  if (extension == ".png") {
    return "image/png";
  }

  if (extension == ".ico") {
    return "image/x-icon";
  }

  return "text/plain";
}

void ws_handle_not_found() {
  // First character of uri() is '/'. Skip it, and assume the rest is
  // a filename.
  String uri(server.uri().substring(1));
  unsigned int fileSize;
  PGM_P contents = read_file(uri.c_str(), &fileSize);
  int returnCode = 200;
  String contentType = get_content_type(uri);

  if (!contents) {
    returnCode = 404;
    contents = read_file("404.html", &fileSize);
    contentType = "text/html";
  }

  server.send(returnCode, contentType.c_str(), contents, fileSize);
}
