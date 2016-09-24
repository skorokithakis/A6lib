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

#define countof(a) (sizeof(a) / sizeof(a[0]))

#define A6_OK 0
#define A6_NOTOK 1
#define A6_TIMEOUT 2
#define A6_FAILURE 3

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


// Block until the module is ready.
byte A6::blockUntilReady(long baudRate) {
    byte response = A6_NOTOK;
    while(A6_OK != response) {
        response = begin(baudRate);
        // This means the modem has failed to initialize and we need to reboot
        // it.
        if (A6_FAILURE == response) return A6_FAILURE;
        delay(1000);
        logln("Waiting for module to be ready...");
    }
    return A6_OK;
}


// Initialize the software serial connection and change the baud rate from the
// default (autodetected) to the desired speed.
byte A6::begin(long baudRate) {
    A6conn->flush();

    if (A6_OK != setRate(baudRate))
        return A6_NOTOK;

    // Factory reset.
    A6command("AT&F", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);

    // Echo off.
    A6command("ATE0", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);

    // Switch audio to headset.
    enableSpeaker(0);

    // Set caller ID on.
    A6command("AT+CLIP=1", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);

    // Set SMS to text mode.
    A6command("AT+CMGF=1", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);

    // Turn SMS indicators off.
    A6command("AT+CNMI=1,0", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);

    // Set SMS storage to the GSM modem.
    if (A6_OK != A6command("AT+CPMS=ME,ME,ME", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL))
        // This may sometimes fail, in which case the modem needs to be
        // rebooted.
        return A6_FAILURE;

    // Set SMS character set.
    setSMScharset("UCS2");

    return A6_OK;
}


// Reboot the module by setting the specified pin HIGH, then LOW. The pin should
// be connected to a P-MOSFET, not the A6's POWER pin.
void A6::powerCycle(int pin) {
    logln("Power-cycling module...");

    powerOff(pin);

    delay(1000);

    powerOn(pin);

    // Give the module some time to settle.
    logln("Done, waiting for the module to initialize...");
    delay(20000);
    logln("Done.");

    A6conn->flush();
}


// Turn the modem power completely off.
void A6::powerOff(int pin) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}


// Turn the modem power on.
void A6::powerOn(int pin) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
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
        return A6_NOTOK;
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

    return A6_OK;
}


// Retrieve the number and locations of unread SMS messages.
int A6::getUnreadSMSLocs(int* buf, int maxItems) {
    String seqStart= "+CMGL: ";
    String response = "";

    // Issue the command and wait for the response.
    byte status = A6command("AT+CMGL=\"REC UNREAD\"", "\xff\r\nOK\r\n", "\r\nOK\r\n", A6_CMD_TIMEOUT, 2, &response);

    int seqStartLen = seqStart.length();
    int responseLen = response.length();
    int index, occurrences = 0;

    // Start looking for the +CMGL string.
    for (int i = 0; i < (responseLen - seqStartLen); i++) {
        // If we found a response and it's less than occurrences, add it.
        if (response.substring(i, i + seqStartLen) == seqStart && occurrences < maxItems) {
            // Parse the position out of the reply.
            sscanf(response.substring(i, i+12).c_str(), "+CMGL: %u,%*s", &index);

            buf[occurrences] = index;
            occurrences++;
        }
    }
    return occurrences;
}


// Return the SMS at index.
SMSmessage A6::readSMS(int index) {
    String response = "";
    char buffer[30];

    // Issue the command and wait for the response.
    sprintf(buffer, "AT+CMGR=%d", index);
    A6command(buffer, "\xff\r\nOK\r\n", "\r\nOK\r\n", A6_CMD_TIMEOUT, 2, &response);

    char message[200];
    char number[50];
    char date[50];
    char type[10];
    int respStart = 0, matched = 0;
    SMSmessage sms = (const struct SMSmessage){ "", "", "" };

    // Parse the response if it contains a valid +CLCC.
    respStart = response.indexOf("+CMGR");
    if (respStart >= 0) {
        // Parse the message header.
        matched = sscanf(response.substring(respStart).c_str(), "+CMGR: \"REC %s\",\"%s\",,\"%s\"\r\n", type, number, date);
        sms.number = String(number);
        sms.date = String(date);
        // The rest is the message, extract it.
        sms.message = response.substring(strlen(type) + strlen(number) + strlen(date) + 24, response.length() - 8);
    }
    return sms;
}

// Delete the SMS at index.
byte A6::deleteSMS(int index) {
    char buffer[20];
    sprintf(buffer, "AT+CMGD=%d", index);
    return A6command(buffer, "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);
}


// Set the SMS charset.
byte A6::setSMScharset(String charset) {
    char buffer[30];

    sprintf(buffer, "AT+CSCS=\"%s\"", charset.c_str());
    return A6command(buffer, "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);
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
long A6::detectRate() {
    int rate = 0;
    int rates[] = {9600, 115200};

    // Try to autodetect the rate.
    logln("Autodetecting connection rate...");
    for (int i = 0; i < countof(rates); i++) {
        rate = rates[i];

        A6conn->begin(rate);
        log("Trying rate ");
        log(rate);
        logln("...");

        delay(100);
        if (A6command("\rAT", "OK", "+CME", 2000, 2, NULL) == A6_OK) {
            return rate;
        }
    }

    logln("Couldn't detect the rate.");
    return A6_NOTOK;
}


// Set the A6 baud rate.
char A6::setRate(long baudRate) {
    int rate = 0;

    rate = detectRate();
    if (rate == A6_NOTOK)
        return A6_NOTOK;

    // The rate is already the desired rate, return.
    //if (rate == baudRate) return OK;

    logln("Setting baud rate on the module...");
    // Change the rate to the requested.
    char buffer[30];
    sprintf(buffer, "AT+IPR=%d", baudRate);
    A6command(buffer, "OK", "+IPR=", A6_CMD_TIMEOUT, 3, NULL);

    logln("Switching to the new rate...");
    // Begin the connection again at the requested rate.
    A6conn->begin(baudRate);
    logln("Rate set.");

    return A6_OK;
}


// Read some data from the A6 in a non-blocking manner.
String A6::read() {
    String reply = "";
    if (A6conn->available()) reply = A6conn->readString();

    // XXX: Replace NULs with \xff so we can match on them.
    for (int x = 0; x < reply.length(); x++) {
        if (reply.charAt(x) == 0) reply.setCharAt(x, 255);
    }
    return reply;
}


// Issue a command.
byte A6::A6command(const char *command, const char *resp1, const char *resp2, int timeout, int repetitions, String *response) {
    byte returnValue = A6_NOTOK;
    byte count = 0;

    // Get rid of any buffered output.
    A6conn->flush();

    while (count < repetitions && returnValue != A6_OK) {
        log("Issuing command: ");
        logln(command);

        A6conn->write(command);
        A6conn->write('\r');

        if (A6waitFor(resp1, resp2, timeout, response) == A6_OK) {
            returnValue = A6_OK;
        } else {
            returnValue = A6_NOTOK;
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
    } while (((reply.indexOf(resp1) + reply.indexOf(resp2)) == -2) && ((millis() - entry) < timeout));

    if (reply != "") {
        log("Reply in ");
        log((millis() - entry));
        log(" ms: ");
        logln(reply);
    }

    if (response != NULL) *response = reply;

    if ((millis() - entry) >= timeout) {
        retVal = A6_TIMEOUT;
        logln("Timed out.");
    } else {
        if (reply.indexOf(resp1) + reply.indexOf(resp2) > -2) {
            logln("Reply OK.");
            retVal = A6_OK;
        } else {
            logln("Reply NOT OK.");
            retVal = A6_NOTOK;
        }
    }
    return retVal;
}
