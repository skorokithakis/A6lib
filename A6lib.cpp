#include <Arduino.h>
#include "SoftwareSerial.h"
#include "A6lib.h"

#ifdef DEBUG
#define log(msg) Serial.print(msg)
#define logln(msg) Serial.println(msg)
#else
#define log(msg)
#define logln(msg)
#endif

#define OK 1
#define NOTOK 2
#define TIMEOUT 3

#define A6_CMD_TIMEOUT 2000


/////////////////////////////////////////////
// Public methods.
//

A6::A6(int transmitPin, int receivePin) {
    A6conn = new SoftwareSerial(receivePin, transmitPin, false, 1024);
    A6conn->setTimeout(100);
}


A6::~A6() {
    delete A6conn;
}


// Initialize the software serial connection and change the baud rate from the
// default (autodetected) to the desired speed.
void A6::begin(long baudRate) {
    // Give the module some time to settle.
    logln("Waiting for the module to initialize...");
    delay(12000);
    logln("Done.");

    A6conn->flush();

    setRate(baudRate);

    // Factory reset.
    A6command("AT&F", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);

    // Echo off.
    A6command("ATE0", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);

    // Set caller ID on.
    A6command("AT+CLIP=1", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);

    // Set SMS to text mode.
    A6command("AT+CMGF=1", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);

    // Switch audio to headset.
    enableSpeaker(0);
}


// Reboot the module by setting the specified pin HIGH, then LOW. The pin should
// be connected to a P-MOSFET, not the A6's POWER pin.
void A6::powerCycle(int pin) {
    logln("Power-cycling module...");

    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);

    delay(1000);

    digitalWrite(pin, HIGH);

    // Give the module some time to settle.
    logln("Power-cycle done.");

    A6conn->flush();
}


// Dial a number.
void A6::dial(String number) {
    char buffer[50];

    logln("Dialing number...");

    sprintf(buffer, "ATD%s;", number.c_str());
    A6command(buffer, "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);
}


// Redial the last number.
void A6::redial() {
    logln("Redialing last number...");
    A6command("AT+DLST", "OK", "CONNECT", A6_CMD_TIMEOUT, 2, NULL);
}


// Answer a call.
void A6::answer() {
    A6command("ATA", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);
}


// Hang up the phone.
void A6::hangUp() {
    A6command("ATH", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);
}


// Check whether there is an active call.
callInfo A6::checkCallStatus() {
    char number[50];
    String response = "";
    int respStart = 0, matched = 0;
    callInfo cinfo = (const struct callInfo){ 0 };

    // Issue the command and wait for the response.
    A6command("AT+CLCC", "OK", "+CLCC", A6_CMD_TIMEOUT, 2, &response);

    // Parse the response if it contains a valid +CLCC.
    respStart = response.indexOf("+CLCC");
    if (respStart >= 0) {
        matched = sscanf(response.substring(respStart).c_str(), "+CLCC: %d,%d,%d,%d,%d,\"%s\",%d", &cinfo.index, &cinfo.direction, &cinfo.state, &cinfo.mode, &cinfo.multiparty, number, &cinfo.type);
        cinfo.number = String(number);
    }

    return cinfo;
}


// Send an SMS.
byte A6::sendSMS(String number, String text) {
    char ctrlZ[2] = { 0x1a, 0x00 };
    char buffer[100];

    if (text.length() > 159) {
        // We can't send messages longer than 160 characters.
        return NOTOK;
    }

    log("Sending SMS to ");
    log(number);
    logln("...");

    sprintf(buffer, "AT+CMGS=\"%s\"", number.c_str());
    A6command(buffer, ">", "yy", A6_CMD_TIMEOUT, 2, NULL);
    delay(100);
    A6conn->println(text.c_str());
    A6conn->println(ctrlZ);
    A6conn->println();

    return OK;
}


// Set the volume for the speaker. level should be a number between 5 and
// 8 inclusive.
void A6::setVol(byte level) {
    char buffer[30];

    // level should be between 5 and 8.
    level = min(max(level, 5), 8);
    sprintf(buffer, "AT+CLVL=%d", level);
    A6command(buffer, "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);
}


// Enable the speaker, rather than the headphones. Pass 0 to route audio through
// headphones, 1 through speaker.
void A6::enableSpeaker(byte enable) {
    char buffer[30];

    // enable should be between 0 and 1.
    enable = min(max(enable, 0), 1);
    sprintf(buffer, "AT+SNFS=%d", enable);
    A6command(buffer, "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);
}



/////////////////////////////////////////////
// Private methods.
//


// Autodetect the connection rate.
int A6::detectRate() {
    int rate = 0;

    // Try to autodetect the rate.
    logln("Autodetecting connection rate...");
    for (int i = 0; i < 8; i++) {
        switch (i) {
        case 0:
            // Try the usual rate first, to speed things up.
            rate = 9600;
            break;
        case 1:
            rate = 1200;
            break;
        case 2:
            rate = 2400;
            break;
        case 3:
            rate = 4800;
            break;
        case 4:
            rate = 19200;
            break;
        case 5:
            rate = 38400;
            break;
        case 6:
            rate = 57600;
            break;
        case 7:
            rate = 115200;
            break;
        }

        A6conn->begin(rate);
        log("Trying rate ");
        log(rate);
        logln("...");

        delay(100);
        if (A6command("AT", "OK", "OK", 2000, 2, NULL) == OK) {
            return rate;
        }
    }

    // Return a default rate.
    return 9600;
}


// Set the A6 baud rate.
void A6::setRate(long baudRate) {
    int rate = detectRate();

    // The rate is already the desired rate, return.
    if (rate == baudRate) return;

    logln("Setting baud rate on the module...");
    // Change the rate to the requested.
    char buffer[30];
    sprintf(buffer, "AT+IPR=%d", baudRate);
    A6command(buffer, "OK", "+IPR=", A6_CMD_TIMEOUT, 3, NULL);

    logln("Switching to the new rate...");
    // Begin the connection again at the requested rate.
    A6conn->begin(baudRate);
    logln("Rate set.");
}


// Read some data from the A6 in a non-blocking manner.
String A6::read() {
    String reply = "";
    if (A6conn->available()) reply = A6conn->readString();
    return reply;
}


// Issue a command.
byte A6::A6command(const char *command, const char *resp1, const char *resp2, int timeout, int repetitions, String *response) {
    byte returnValue = NOTOK;
    byte count = 0;

    // Get rid of any buffered output.
    A6conn->flush();

    while (count < repetitions && returnValue != OK) {
        log("Issuing command: ");
        logln(command);

        A6conn->print(command);
        A6conn->print("\r");

        if (A6waitFor(resp1, resp2, timeout, response) == OK) {
            returnValue = OK;
        } else {
            returnValue = NOTOK;
        }
        count++;
    }
    return returnValue;
}


// Wait for responses.
byte A6::A6waitFor(const char *resp1, const char *resp2, int timeout, String *response) {
    unsigned long entry = millis();
    int count = 0;
    String reply = "";
    byte retVal = 99;
    do {
        reply += read();
        yield();
    } while ((reply.indexOf(resp1) + reply.indexOf(resp2) == -2) && millis() - entry < timeout);

    if (reply != "") {
        log("Reply in ");
        log((millis() - entry));
        log(" ms: ");
        logln(reply);
    }

    if (response != NULL) *response = reply;

    if ((millis() - entry) >= timeout) {
        retVal = TIMEOUT;
        logln("Timed out.");
    } else {
        if (reply.indexOf(resp1) + reply.indexOf(resp2) > -2) {
            logln("Reply OK.");
            retVal = OK;
        } else {
            logln("Reply NOT OK.");
            retVal = NOTOK;
        }
    }
    return retVal;
}
