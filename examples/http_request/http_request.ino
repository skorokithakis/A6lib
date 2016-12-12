#include<A6lib.h>
#include <A6httplib.h>

// Instantiate the library with TxPin, RxPin.
//tx and rx pins are opposite side to antenna as of today's date.
//arduino pin 7 to pin 9 of A6
//arduino pin 8 to pin 8 of A6

A6lib A6l(7, 8);
A6httplib HttpRequest(&A6l);



void setup() 
{

	String group_iotfy="Demosensors";
	String id_iotfy="DX";
	String tag_iotfy="test";
	String value_iotfy="042";
 
  String apn = "airtelworld.com";
  
  String host="cloud.iotfy.co"; 
  String path="/api/telemetry/v1/post_text_data";
  
	Serial.begin(115200);
	delay(1000);
	

	// Power-cycle the module to reset it.
	//   A6l.powerCycle(D0);
	
	A6l.blockUntilReady(9600);
	Serial.println("All ready");
	delay(1000);
	//  A6l.dial("9810601848");
	//  delay(5000);
	Serial.println("now trying for internet");
	while(A6l.getFree(3000)!=0);
	Serial.println("freed with ATOK");


  // create the Http post request's Body here
	String data="{\"group\":\"";
	data+=group_iotfy;
	data+="\", \"device\":\"";
	data+=id_iotfy;
	data+="\", \"data\":";
	data+="[{\"tag\":\"";
	data+=tag_iotfy;
	data+="\",\"text\":\"";
	data+=value_iotfy;
	data+="\"}]}";
	Serial.println(data);

  HttpRequest.ConnectGPRS(apn);
	HttpRequest.Post(host, path, data);
}

void loop() 
{
//     Relay things between Serial and the module's SoftSerial.
    while (A6l.A6conn->available() > 0) 
    {
        Serial.write(A6l.A6conn->read());
    }
    while (Serial.available() > 0) 
    {
        A6l.A6conn->write(Serial.read());
    }
}


