#ifndef A6lib_h
#define A6lib_h

#include <Arduino.h>
#include "SoftwareSerial.h"


enum call_direction {
    DIR_OUTGOING = 0,
    DIR_INCOMING = 1
};

enum call_state {
    CALL_ACTIVE = 0,
    CALL_HELD = 1,
    CALL_DIALING = 2,
    CALL_ALERTING = 3,
    CALL_INCOMING = 4,
    CALL_WAITING = 5,
    CALL_RELEASE = 7
};

enum call_mode {
    MODE_VOICE = 0,
    MODE_DATA = 1,
    MODE_FAX = 2,
    MODE_VOICE_THEN_DATA_VMODE = 3,
    MODE_VOICE_AND_DATA_VMODE = 4,
    MODE_VOICE_AND_FAX_VMODE = 5,
    MODE_VOICE_THEN_DATA_DMODE = 6,
    MODE_VOICE_AND_DATA_DMODE = 7,
    MODE_VOICE_AND_FAX_FMODE = 8,
    MODE_UNKNOWN = 9
};

struct callInfo {
    int index;
    call_direction direction;
    call_state state;
    call_mode mode;
    int multiparty;
    String number;
    int type;
};


class A6 {
public:
    A6(int transmitPin, int receivePin);
    ~A6();

    char begin(long baudRate);
    void blockUntilReady(long baudRate);

    void powerCycle(int pin);
    void powerOn(int pin);
    void powerOff(int pin);

    void dial(String number);
    void redial();
    void answer();
    void hangUp();
    callInfo checkCallStatus();

    byte sendSMS(String number, String text);

    void setVol(byte level);
    void enableSpeaker(byte enable);

    SoftwareSerial *A6conn;
private:
    String read();
    byte A6command(const char *command, const char *resp1, const char *resp2, int timeout, int repetitions, String *response);
    byte A6waitFor(const char *resp1, const char *resp2, int timeout, String *response);
    long detectRate();
    char setRate(long baudRate);
};
#endif
