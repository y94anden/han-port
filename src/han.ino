#define BUF_SIZE 256

// Circular buffer for received data
char han_buffer[BUF_SIZE];
int han_write;
int han_read;
bool han_drop_junk;

void han_setup() {
  // Start serial inverted
  Serial.begin(115200, SERIAL_8N1, SERIAL_RX_ONLY, 1, true);
  han_write = 0;
  han_read = 0;

  // Start by dropping all bytes to first newline
  han_drop_junk = true;
}

int han_available() {
  while (han_read > BUF_SIZE) {
    han_write -= BUF_SIZE;
    han_read -= BUF_SIZE;
  }
  return han_write - han_read;
}

void han_parse() {}

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
