#include <Arduino.h>
#include <SoftwareSerial.h>
#include <A6lib.h>

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
#define RST 2


/////////////////////////////////////////////
// Public methods.
//

A6::A6(int transmitPin, int receivePin) {
    A6conn = new SoftwareSerial(receivePin, transmitPin, false, 1024);
}


A6::~A6() {
    delete A6conn;
}


// Initialize the software serial connection and change the baud rate from the
// default (autodetected) to the desired speed.
void A6::begin(long baudRate) {
    // Give the module some time to settle.
    delay(1000);

    A6conn->flush();

    setRate(baudRate);

    // Factory reset.
    A6command("AT&F", "OK", "yy", 5000, 2);

    // Echo off.
    A6command("ATE0", "OK", "yy", 5000, 2);

    // Set caller ID on.
    A6command("AT+CLIP=1", "OK", "yy", 5000, 2);

    // Set SMS to text mode.
    A6command("AT+CMGF=1", "OK", "yy", 5000, 2);
}


// Reboot the module by setting the specified pin LOW, then HIGH. The pin should
// be connected to a MOSFET, it is not the A6's POWER rail.
void A6::powerCycle(int pin) {
    logln("Power-cycling module...");
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);

    delay(2000);

    digitalWrite(pin, HIGH);

    // Give the module some time to settle.
    delay(5000);
    logln("Power-cycle done.");

    A6conn->flush();
}


// Dial a number.
void A6::dial(String number) {
    char buffer[50];

    logln("Dialing number...");

    A6command("AT+SNFS=0", "OK", "yy", 5000, 2);
    sprintf(buffer, "ATD%s;", number.c_str());
    A6command(buffer, "OK", "yy", 5000, 2);
}


// Redial the last number.
void A6::redial() {
    logln("Redialing last number...");
    A6command("AT+DLST", "OK", "CONNECT", 5000, 2);
}


// Answer a call.
void A6::answer() {
    A6command("ATA", "OK", "yy", 5000, 2);
}


// Hang up the phone.
void A6::hangUp() {
    A6command("ATH", "OK", "yy", 5000, 2);
}


// Send an SMS.
void A6::sendSMS(String number, String text) {
    char ctrlZ[2] = { 0x1a, 0x00 };
    char buffer[100];

    log("Sending SMS to ");
    log(number);
    logln("...");

    sprintf(buffer, "AT+CMGS = \"%s\"", number.c_str());
    A6command(buffer, ">", "yy", 5000, 2);
    delay(100);
    A6conn->println(text.c_str());
    A6conn->println(ctrlZ);
    A6conn->println();
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
            rate = 1200;
            break;
        case 1:
            rate = 2400;
            break;
        case 2:
            rate = 4800;
            break;
        case 3:
            rate = 9600;
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
        if (A6command("AT", "OK", "OK", 2000, 2) == OK) {
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

    logln("Setting baud rate...");
    // Change the rate to the requested.
    char buffer[30];
    sprintf(buffer, "AT+IPR=%d", baudRate);
    A6command(buffer, "OK", "+IPR=", 5000, 3);

    logln("Switching to the new rate...");
    // Begin the connection again at the requested rate.
    A6conn->begin(baudRate);
    logln("Rate set.");
}


// Read some data from the A6 in a non-blocking manner.
String A6::read() {
    String reply = "";
    if (A6conn->available())  {
        reply = A6conn->readString();
    }
    return reply;
}


// Issue a command.
byte A6::A6command(String command, String resp1, String resp2, int timeout, int repetitions) {
    byte returnValue = NOTOK;
    byte count = 0;

    // Get rid of any buffered output.
    A6conn->flush();

    while (count < repetitions && returnValue != OK) {
        log("Issuing command: ");
        logln(command);

        A6conn->print(command);
        A6conn->print("\r");

        if (A6waitFor(resp1, resp2, timeout) == OK) {
            returnValue = OK;
        } else {
            returnValue = NOTOK;
        }
        count++;
    }
    return returnValue;
}


// Wait for responses.
byte A6::A6waitFor(String resp1, String resp2, int timeout) {
    unsigned long entry = millis();
    int count = 0;
    String reply = "";
    byte retVal = 99;
    do {
        reply = read();
        if (reply != "") {
            log("Reply in ");
            log((millis() - entry));
            log(" ms: ");
            logln(reply);
        }
        yield();
    } while ((reply.indexOf(resp1) + reply.indexOf(resp2) == -2) && millis() - entry < timeout);

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
