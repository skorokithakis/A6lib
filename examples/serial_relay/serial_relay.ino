#include <A6lib.h>

// Instantiate the library with TxPin, RxPin.
A6lib A6l(7, 8);

void setup() {
    Serial.begin(115200);

    delay(1000);

    // Power-cycle the module to reset it.
    // A6l.powerCycle(0);
    A6l.blockUntilReady(9600);
}

void loop() {
    // Relay things between Serial and the module's SoftSerial.
    while (A6l.A6conn->available() > 0) {
        Serial.write(A6l.A6conn->read());
    }
    while (Serial.available() > 0) {
        A6l.A6conn->write(Serial.read());
    }
}
