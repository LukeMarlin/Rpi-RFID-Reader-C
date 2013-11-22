#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <wiringPi.h>
#include "CardReader.c"
#include "main.h"
#include "pins.c"

// Which GPIO pin we're using
#define UNUSED_PIN -1
#define READERS_COUNT 8
#define FRAME_SIZE 26

int pins[17] = {PIN_0, PIN_1, PIN_4, PIN_7, PIN_8, PIN_9, PIN_10, PIN_11, PIN_14, PIN_15, PIN_17, PIN_18, PIN_21, PIN_22, PIN_23, PIN_24, PIN_25};
int values[17] = {};
CardReader** readers;

#include "callbacks.c"

void initProgram(){
	wiringPiSetupGpio();
	readers = (CardReader**)malloc(sizeof(CardReader*)*READERS_COUNT);	
	createCardReader("reader1", PIN_23, PIN_24);
	printf("[%d]", readers[PIN_23]->name);
}

int main(void) {
	
	initProgram();
	printf("ID : %ld\n", (long int)syscall(224));
	printf("%d\n", pins[PIN_23]);
	printf("%d\n", values[PIN_24]);
	
	// Set pin to output in case it's not
	pinMode(PIN_23, INPUT);
	pinMode(PIN_24, INPUT);
	pullUpDnControl(PIN_23, PUD_UP);
	pullUpDnControl(PIN_24, PUD_UP);
	
	// Bind to interrupt
	wiringPiISR(PIN_23, INT_EDGE_FALLING, &callback23);
	wiringPiISR(PIN_24, INT_EDGE_FALLING, &callback24);

		

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
	temp.tag = malloc(sizeof(char*)*FRAME_SIZE);
	temp.bitCount = 0;

	readers[pGPIO_0] = &temp;
	readers[pGPIO_1] = &temp;
	values[pGPIO_0] = 0;
	values[pGPIO_1] = 1;

	return temp;

}
