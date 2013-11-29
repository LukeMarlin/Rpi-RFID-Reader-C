#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <wiringPi.h>
#include "CardReader.c"
#include "main.h"
#include "pins.c"

#define UNUSED_PIN -1
#define PINS_COUNT 26
#define FRAME_SIZE 26
#define READERS_COUNT 8


int pins[PINS_COUNT] = {PIN_0, 
		PIN_1, 
		UNUSABLE_PIN, 
		UNUSABLE_PIN,  
		PIN_4, 
		UNUSABLE_PIN, 
		UNUSABLE_PIN, 
		PIN_7, 
		PIN_8, 
		PIN_9, 
		PIN_10, 
		PIN_11, 
		UNUSABLE_PIN,
		UNUSABLE_PIN,
		PIN_14, 
		PIN_15, 
		UNUSABLE_PIN,
		PIN_17, 
		PIN_18,
		UNUSABLE_PIN,
		UNUSABLE_PIN,
		PIN_21, 
		PIN_22, 
		PIN_23, 
		PIN_24, 
		PIN_25};

int values[PINS_COUNT] = {};
CardReader** readers;
CardReader _readers;

#include "callbacks.c"

void initProgram(){
	wiringPiSetupGpio();
	readers = (CardReader**)malloc(sizeof(CardReader*)*PINS_COUNT);	
	_readers = malloc(sizeof(CardReader)*READERS_COUNT;
}

void initReaders(){
_readers[0] =  createCardReader("Lecteur", PIN_23, PIN_24);

}

int main(void) {
	
	initProgram();
	
	// Set pin to output in case it's not
	pinMode(PIN_23, INPUT);
	pinMode(PIN_24, INPUT);
	pullUpDnControl(PIN_23, PUD_UP);
	pullUpDnControl(PIN_24, PUD_UP);
	
	// Bind to interrupt
	wiringPiISR(PIN_23, INT_EDGE_FALLING, &callback23);
	wiringPiISR(PIN_24, INT_EDGE_FALLING, &callback24);

	printf("PIN : %d, Readers[PIN]->GPIO_0: %d\n", PIN_23, readers[PIN_23]->GPIO_0);	

	// Waste time but not CPU
	for (;;) {
		sleep(1);
	}
}

CardReader createCardReader(char* pname,int pGPIO_0, int pGPIO_1){
	CardReader temp;
	
	temp.name = pname;
	temp.GPIO_0 = pGPIO_0;
	temp.GPIO_1 = pGPIO_1;
	temp.tag = malloc(sizeof(char)*FRAME_SIZE);
	temp.bitCount = 0;

	readers[pGPIO_0] = &temp;
	readers[pGPIO_1] = &temp;
	values[pGPIO_0] = 0;
	values[pGPIO_1] = 1;

	return temp;

}

void updateArrays(CardReader reader){
	realloc(_readers
}
