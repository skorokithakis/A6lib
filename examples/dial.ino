#include <A6lib.h>

A6 A6l(D6, D5);

void setup() {
    Serial.begin(115200);

    delay(1000);

    A6l.powerCycle(D0);
    A6l.begin(9600);
    delay(2000);
    A6l.dial("+1234567890");
    delay(8000);
    A6l.hangUp();
}

void loop() {
    while (A6l.A6conn->available() > 0) {
        Serial.write(A6l.A6conn->read());
    }
    while (Serial.available() > 0) {
        A6l.A6conn->write(Serial.read());
    }
}
