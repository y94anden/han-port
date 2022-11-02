#include <ESP8266WebServer.h>

extern "C" {
#include "embedded-filesystem/embedded-fs.c"
}

ESP8266WebServer server(80);

void webserver_setup() {
  server.on("/", ws_handle_root);
  server.on("/restart", ws_handle_restart);
  server.on("/raw", ws_handle_raw);
  server.on("/rest/current", ws_handle_rest_last);
  server.on("/history", ws_handle_history);

  server.onNotFound(ws_handle_not_found);

  server.begin(); // Actually start the server
}

void webserver_loop() { server.handleClient(); }

void ws_handle_root() {
  server.send(200, "text/html", (char *)read_file("index.html", 0));
}

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

void ws_handle_history() {
  int len = han_hist_wrapped ? HAN_HISTORY_BUF_LEN : han_hist_write;
  server.setContentLength(len * 2); // 16 bit data

  server.sendHeader("Cache-Control", String("no-cache"));
  server.sendHeader("Access-Control-Allow-Origin", String("*"));

  server.send(200, "application/octet-stream", "");
  if (han_hist_wrapped) {
    // Send the oldest data (that is soon to be overwritten)
    server.sendContent((const char *)&han_history[han_hist_write],
                       (HAN_HISTORY_BUF_LEN - han_hist_write) * 2);
  }
  // Send the data from 0 and up until latest.
  server.sendContent((const char *)han_history, han_hist_write * 2);
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
