#include "pins.c"

void handler(int PIN_ID){
	CardReader* reader = readers[PIN_ID];
	printf("PIN: %d, Readers[PIN]->GPIO_0: %d\n", PIN_23, readers[PIN_23]->GPIO_0);
	fflush(stdin);
	//reader->tag[0] = '0'; //values[PIN_ID];
	/*reader->tag[reader->bitCount++] = values[PIN_ID];
	if(reader->bitCount == FRAME_SIZE){
		printf("[%s]: %s\n",reader->name, reader->tag);
		reader->bitCount = 0;
	}*/
}

void callback0(){handler(PIN_0);}
void callback1(){handler(PIN_1);}
void callback4(){handler(PIN_4);}
void callback7(){handler(PIN_7);}
void callback8(){handler(PIN_8);}
void callback9(){handler(PIN_9);}
void callback10(){handler(PIN_10);}
void callback11(){handler(PIN_11);}
void callback14(){handler(PIN_14);}
void callback15(){handler(PIN_15);}
void callback17(){handler(PIN_17);}
void callback18(){handler(PIN_18);}
void callback21(){handler(PIN_21);}
void callback22() {handler(PIN_22);}
void callback23() {handler(PIN_23);}
void callback24() {handler(PIN_24);}
void callback25() {handler(PIN_25);}
