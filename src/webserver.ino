#include "template-render.h"
#include <ESP8266WebServer.h>

extern "C" {
#include "embedded-filesystem/embedded-fs.c"
}

ESP8266WebServer server(80);
char ws_sub_path[256];

void webserver_setup() {
  server.on("/", ws_handle_root);
  server.on("/history", ws_handle_hist);
  server.on("/meter", ws_handle_meter);
  server.on("/restart", ws_handle_restart);
  server.on("/raw", ws_handle_raw);
  server.on("/rest/current", ws_handle_rest_last);
  server.on("/history/full", ws_handle_history_full);
  server.on("/history/1", ws_handle_history_1);
  server.on("/history/2", ws_handle_history_2);
  server.on("/history/3", ws_handle_history_3);
  server.on("/history/meter", ws_handle_history_meter);
  server.on("/simulate/meter", ws_handle_simulate_meter_history);

  server.onNotFound(ws_handle_not_found);

  // To get more headers than the two standard ("Authorization" and
  // "If-None-Match"), the desired headers must be supplied
  const char *headers[] = {"X-Sub-Path"};
  server.collectHeaders(headers, sizeof(headers) / sizeof(char *));

  // Default subpath is empty until we find a header "X-Sub-Path", in
  // which case we replace it
  ws_sub_path[0] = '\0';
  tpl_set("X-Sub-Path", STRING, ws_sub_path);

  server.begin(); // Actually start the server
}

void webserver_loop() { server.handleClient(); }

void ws_send(PGM_P p, unsigned int length) {
  if (length == 0) {
    length = strlen_P(p);
  }
  if (length == 0) {
    // No need to send a zero length string
    return;
  }
  server.sendContent(p, length);
}

void ws_render_html(const char *filename) {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  if (server.hasHeader("X-Sub-Path")) {
    strcpy(ws_sub_path, server.header("X-Sub-Path").c_str());
  }
  tpl_send(filename, ws_send);
}

void ws_handle_root() { ws_render_html("index.html"); }
void ws_handle_hist() { ws_render_html("history.html"); }
void ws_handle_meter() { ws_render_html("meter.html"); }

void ws_handle_restart() {
  server.send(200, "text/plain", "restarting...");
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

  server.sendContent("\"time\":\"");
  server.sendContent(han_last.time);
  server.sendContent("\"");

  server.sendContent(",\"meter\":");
  send_milli_value(han_last.meter_Wh);

  server.sendContent(",\"power\":");
  send_milli_value(han_last.power_W);

  server.sendContent(",\"power\":[");
  send_milli_value(han_last.phasePower_W[0]);
  server.sendContent(",");
  send_milli_value(han_last.phasePower_W[1]);
  server.sendContent(",");
  send_milli_value(han_last.phasePower_W[2]);
  server.sendContent("]");

  server.sendContent(",\"voltage\":[");
  send_milli_value(han_last.phaseVoltage_mV[0]);
  server.sendContent(",");
  send_milli_value(han_last.phaseVoltage_mV[1]);
  server.sendContent(",");
  send_milli_value(han_last.phaseVoltage_mV[2]);
  server.sendContent("]");

  server.sendContent(",\"current\":[");
  send_milli_value(han_last.phaseCurrent_mA[0]);
  server.sendContent(",");
  send_milli_value(han_last.phaseCurrent_mA[1]);
  server.sendContent(",");
  send_milli_value(han_last.phaseCurrent_mA[2]);
  server.sendContent("]");

  server.sendContent("}");
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
  char *contents = (char *)read_file(uri.c_str(), &fileSize);
  int returnCode = 200;
  String contentType = get_content_type(uri);

  if (!contents) {
    returnCode = 404;
    contents = (char *)read_file("404.html", &fileSize);
    contentType = "text/html";
  }

  server.send(returnCode, contentType.c_str(), contents, fileSize);
}
