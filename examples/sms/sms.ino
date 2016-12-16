#include <A6lib.h>

// Instantiate the library with TxPin, RxPin.
A6lib A6l(7, 8);
//tx and rx pins are opposite side to antenna as of today's date.
//arduino pin 7 to pin 9 of A6
//arduino pin 8 to pin 8 of A6


int unreadSMSLocs[30] = {0};
int unreadSMSNum = 0;
SMSmessage sms;

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
       if (cinfo.number == "919999999999")
        {
          //add + before your country code as it doesn't appear in the if messages don't get send ex-
          // CLIP: "91**********",145,,,,1
          // String new_number="+" + cinfo.number;
          // Serial.println(new_number);
           A6l.sendSMS(new_number, "I can't come to the phone right now, I'm a machine.");
        }
        A6l.hangUp();
    }

    // Get the memory locations of unread SMS messages.
    unreadSMSNum = A6l.getUnreadSMSLocs(unreadSMSLocs, 30);

    for (int i=0; i < unreadSMSNum; i++) {
        Serial.print("New message at index: ");
        Serial.println(unreadSMSLocs[i], DEC);

        sms = A6l.readSMS(unreadSMSLocs[i]);
        Serial.println(sms.number);
        Serial.println(sms.date);
        Serial.println(sms.message);
    }
    delay(1000);
}
