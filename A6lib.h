#ifndef A6lib_h
#define A6lib_h

#include <Arduino.h>
#include <SoftwareSerial.h>

class A6 {
public:
    A6(int transmitPin, int receivePin);
    ~A6();

    void begin(long baudRate);
    void powerCycle(int pin);

    void dial(String number);
    void redial();
    void hangUp();

    void sendSMS(String number, String text);

    SoftwareSerial *A6conn;
private:
    String read();
    byte A6command(String command, String resp1, String resp2, int timeout, int repetitions);
    byte A6waitFor(String resp1, String resp2, int timeout);
    int detectRate();
    void setRate(long baudRate);
};
#endif
