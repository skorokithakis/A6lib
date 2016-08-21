#include <A6lib.h>

// Instantiate the library with TxPin, RxPin.
A6 A6l(D6, D5);

void setup() {
    Serial.begin(115200);

    delay(1000);

    // Power-cycle the module to reset it.
    A6l.powerCycle(D0);
    A6l.blockUntilReady(9600);
}

void loop() {
    callInfo cinfo = A6l.checkCallStatus();
    if (cinfo.direction == DIR_INCOMING) {
       if (cinfo.number == "1234567890")
           A6l.sendSMS("1234567890", "I can't come to the phone right now, I'm a machine.");
        A6l.hangUp();
    }
    delay(1000);
}
