#include <ESP8266WebServer.h>

extern "C" {
#include "embedded-filesystem/embedded-fs.c"
}

ESP8266WebServer server(80);

void webserver_setup() {
  server.on("/", ws_handle_root);
  server.on("/restart", ws_handle_restart);

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
