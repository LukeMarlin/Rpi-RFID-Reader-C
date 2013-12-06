#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <wiringPi.h>
#include <time.h>
#include <errno.h>
#include "cardReader.h"
#include "main.h"
#include "pins.c"

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

char values[PINS_COUNT] = {};
int READERS_COUNT = 0;

CardReader** readers;
CardReader* _readers;

#include "callbacks.c"

void initProgram(){
	wiringPiSetupGpio();
	readers = (CardReader**)malloc(sizeof(CardReader*)*PINS_COUNT);	
}

void initReaders(){
	createCardReader("Fred", PIN_23, PIN_24, &callback23, &callback24);
}

int main(void) {
	
	initProgram();
	initReaders();

	// Waste time but not CPU
	for (;;) {
		sleep(1);
	}
}

void createCardReader(char* pname, int pGPIO_0, int pGPIO_1, void(*callback0), void(*callback1)){
	CardReader* temp = malloc(sizeof(CardReader));
	
	temp->name = pname;
	temp->GPIO_0 = pGPIO_0;
	temp->GPIO_1 = pGPIO_1;
	temp->tag = (char *)malloc(sizeof(char)*FRAME_SIZE+1);
	temp->tag[FRAME_SIZE] = '\0';
	temp->bitCount = 0;
	
	
	// Set pin to input in case it's not
	pinMode(pGPIO_0, INPUT);
	pinMode(pGPIO_1, INPUT);
	
	pullUpDnControl(pGPIO_0, PUD_UP);
	pullUpDnControl(pGPIO_1, PUD_UP);

	// Bind to interrupt
	wiringPiISR(PIN_23, INT_EDGE_FALLING, callback0);
	wiringPiISR(PIN_24, INT_EDGE_FALLING, callback1);

	updateArrays(temp);

}

void updateArrays(CardReader* reader){
	_readers = realloc(_readers, sizeof(CardReader)*(READERS_COUNT+1));
	_readers[READERS_COUNT] = *reader;

	readers[reader->GPIO_0] = reader;
	readers[reader->GPIO_1] = reader;
	values[reader->GPIO_0] = '0';
	values[reader->GPIO_1] = '1';

	READERS_COUNT++;
}

int parityCheck(char** tag){
	int bitsTo1 =0;
	int i=0;
	char* temp = *tag;

	for(i; i<=MIDDLE_FRAME-1; i++){
		if(temp[i] == '1')	bitsTo1++;
	}
		
	if(bitsTo1%2 != 0)	return 0;

	bitsTo1 = 0;

	for(i=MIDDLE_FRAME; i<=FRAME_SIZE-1; i++){
		if(temp[i] == '1')	bitsTo1++;
	}

	if(bitsTo1%2 == 0)	return 0;


	return 1;
}


long int getIntFromTag(char* tag){
	char* temp = tag;

	temp[0] = '0';
	//temp[FRAME_SIZE-1] = '0';
	long int result;
	result = strtol(temp, 0, 2);
	result = result >> 1;
	return result;
}

