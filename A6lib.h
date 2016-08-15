#ifndef A6lib_h
#define A6lib_h

#include <Arduino.h>
#include "SoftwareSerial.h"


struct callInfo {
    int index;
    int direction;
    int state;
    int mode;
    int multiparty;
    String number;
    int type;
};


class A6 {
public:
    A6(int transmitPin, int receivePin);
    ~A6();

    void begin(long baudRate);
    void powerCycle(int pin);

    void dial(String number);
    void redial();
    void answer();
    void hangUp();
    callInfo checkCallStatus();

    void sendSMS(String number, String text);

    void setVol(byte level);
    void enableSpeaker(byte enable);

    SoftwareSerial *A6conn;
private:
    String read();
    byte A6command(const char *command, const char *resp1, const char *resp2, int timeout, int repetitions, String *response);
    byte A6waitFor(const char *resp1, const char *resp2, int timeout, String *response);
    int detectRate();
    void setRate(long baudRate);
};
#endif
