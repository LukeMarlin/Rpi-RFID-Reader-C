#include <stdio.h>
#include <sys/time.h>
#include <wiringPi.h>
#include "CardReader.c"

// Which GPIO pin we're using
#define PIN_0 0
#define PIN_1 1
#define PIN_4 4
#define PIN_7 7
#define PIN_8 8
#define PIN_9 9
#define PIN_10 10
#define PIN_11 11
#define PIN_14 14
#define PIN_15 15
#define PIN_17 17
#define PIN_18 18
#define PIN_21 21
#define PIN_22 22
#define PIN_23 23
#define PIN_24 24
#define PIN_25 25
#define UNUSED_PIN -1
#define READERS_COUNT 8

int pins[17] = {PIN_0, PIN_1, PIN_4, PIN_7, PIN_8, PIN_9, PIN_10, PIN_11, PIN_14, PIN_15, PIN_17, PIN_18, PIN_21, PIN_22, PIN_23, PIN_24, PIN_25};
int values[17] = {};
CardReader** readers;

// Handler for interrupt
void handle(int GPIO) {
	printf("ID : %ld", (long int)syscall(224));
	printf("%d\n", GPIO);
	fflush(stdout);
}

void initProgram(){
	wiringPiSetupGpio();
	readers = (CardReader**)malloc(sizeof(CardReader*)*READERS_COUNT);
	CardReader test;
	test.GPIO_0 = 1;
	readers[0]=&test;
	printf("Salut %d", readers[0]->GPIO_0);

}

int main(void) {
	
	initProgram();
	printf("ID : %ld\n", (long int)syscall(224));
	printf("%d\n", pins[5]);
	printf("%d\n", values[7]);
	// Set pin to output in case it's not
	pinMode(PIN_23, INPUT);
	pinMode(PIN_24, INPUT);
	pullUpDnControl(PIN_23, PUD_UP);
	pullUpDnControl(PIN_24, PUD_UP);
	
	// Bind to interrupt
	wiringPiISR(PIN_23, INT_EDGE_FALLING, &handle);
	wiringPiISR(PIN_24, INT_EDGE_FALLING, &handle);

		

	// Waste time but not CPU
	for (;;) {
		sleep(1);
	}
}

