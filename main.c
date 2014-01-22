#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>
#include <wiringPi.h>
#include <wiringShift.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include <pthread.h>
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
long* userTags;
long* adminTags;
int userTagsCount = 0;
int adminTagsCount = 0;
double shiftRegisterValue = 0;
int READERS_COUNT = 0;
pthread_mutex_t locker = PTHREAD_MUTEX_INITIALIZER;

CardReader** readers;
CardReader* _readers;

#include "callbacks.c"


int main(void) {
	
	initProgram();
	initReaders();
	signal(SIGUSR1, signalHandler);
	digitalWrite(PIN_LATCH, LOW);	
	shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, 0);
	digitalWrite(PIN_LATCH, HIGH);	
	// Waste time but not CPU
	for (;;) {
		sleep(1);
	}
}


void initProgram(){
	wiringPiSetupGpio();
	readers = (CardReader**)malloc(sizeof(CardReader*)*PINS_COUNT);	
	loadTagsFile(userTags, "userTags.txt", &userTagsCount);
	pinMode(PIN_LATCH, OUTPUT);
	digitalWrite(PIN_LATCH, HIGH);	
	pinMode(PIN_DATA, OUTPUT);
	digitalWrite(PIN_DATA, LOW);
	pinMode(PIN_CLOCK, OUTPUT);
	digitalWrite(PIN_CLOCK, LOW);
}

void initReaders(){
	createCardReader("Porte principale", PIN_23, PIN_24, 0, 3000, 1, 2000, 2, 2000, &callback23, &callback24);
	createCardReader("Porte annexe", PIN_7, PIN_25, 3, 3000, 4, 2000, 5, 2000, &callback7, &callback25);
}

void createCardReader(char* pname, int pGPIO_0, int pGPIO_1, double doorPin, double doorTime, double ledPin, double ledTime, double buzzerPin, double buzzerTime, void(*callback0), void(*callback1)){
	CardReader* temp = malloc(sizeof(CardReader));
	
	temp->name = pname;
	temp->GPIO_0 = pGPIO_0;
	temp->GPIO_1 = pGPIO_1;
	temp->tag = (char *)malloc(sizeof(char)*FRAME_SIZE+1);
	temp->tag[FRAME_SIZE] = '\0';
	temp->bitCount = 0;
	temp->isOpening = 0;
	temp->door = doorPin;
	temp->doorTime = doorTime;
	temp->led = ledPin;
	temp->ledTime = ledTime;
	temp->buzzer = buzzerPin;
	temp->buzzerTime = buzzerTime;
	pthread_mutex_init(&temp->lockObj, NULL);
	
	// Set pin to input in case it's not
	pinMode(pGPIO_0, INPUT);
	pinMode(pGPIO_1, INPUT);
	
	pullUpDnControl(pGPIO_0, PUD_UP);
	pullUpDnControl(pGPIO_1, PUD_UP);

	// Bind to interrupt
	wiringPiISR(pGPIO_0, INT_EDGE_FALLING, callback0);
	wiringPiISR(pGPIO_1, INT_EDGE_FALLING, callback1);

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
	long int result;
	result = strtol(temp, 0, 2);
	result = result >> 1;
	return result;
}

int loadTagsFile(long* array, char* filePath, int* tagCounter){
	FILE* tagsFile = fopen(filePath, "r");
	if(tagsFile == NULL) return 0;
	else{
		int charCount = 0;
		int tagCount = 0;
		int readChar = 0;
		char* buffer = (char*)malloc(0);

		while(readChar != EOF){
			readChar = fgetc(tagsFile);
			if(readChar != '\n'){
				charCount++;
				buffer = (char*) realloc (buffer, sizeof(char) * (charCount+1));
				buffer[charCount - 1] = (char)readChar;
			}
			else{
				array = (long*)realloc(array, sizeof(long) * (tagCount+1));
				array[tagCount] = strtol(buffer, 0 , 10);
				buffer = (char*)malloc(0);
				tagCount++;
				charCount = 0;
			}
		}
	*tagCounter = tagCount;
	userTags = array;
	free(buffer);
	fclose(tagsFile);

	return 1;
	}
}

int checkAuthorization(long* tag){
	int i;
	for(i=0;i<userTagsCount; i++){
		if(userTags[i] == *tag) return 1;
	}

	return 0;
}

void* grantAccess(void* reader)
{
	CardReader* tempReader = (CardReader*)reader;
	
	tempReader->isOpening = 1;
	printf("Opening %s...\n", tempReader->name);
	
	pthread_t thread1;
	pthread_attr_t attr1;
	pthread_t thread2;
	pthread_attr_t attr2;
	pthread_t thread3;
	pthread_attr_t attr3;

	pthread_attr_init(&attr1);
	pthread_attr_init(&attr2);
	pthread_attr_init(&attr3);

	pthread_attr_setdetachstate(&attr1, PTHREAD_CREATE_DETACHED);
	pthread_attr_setdetachstate(&attr2, PTHREAD_CREATE_DETACHED);
	pthread_attr_setdetachstate(&attr3, PTHREAD_CREATE_DETACHED);
	
	int longest = tempReader->doorTime;
	if(tempReader->buzzerTime > longest)
		longest = tempReader->buzzerTime;
	else if(tempReader->ledTime >longest)
		longest = tempReader->ledTime;
	
	double* doorValues = (double*)malloc(sizeof(double)*2);
	doorValues[0] = tempReader->door;
	doorValues[1] = tempReader->doorTime;
	double* ledValues = (double*)malloc(sizeof(double)*2);
	ledValues[0] = tempReader->led;
	ledValues[1] = tempReader->ledTime;
	double* buzzerValues = (double*)malloc(sizeof(double)*2);
	buzzerValues[0] = tempReader->buzzer;
	buzzerValues[1] = tempReader->buzzerTime;

	pthread_create(&thread1, &attr1, &updateOutput, (void*)doorValues);		
	pthread_create(&thread2, &attr2, &updateOutput, (void*)ledValues);
	pthread_create(&thread3, &attr3, &updateOutput, (void*)buzzerValues);
	
	usleep((int)(longest*1000));
	pthread_attr_destroy(&attr1);
	pthread_attr_destroy(&attr2);
	pthread_attr_destroy(&attr3);
	tempReader->isOpening = 0;
	
}

void* updateOutput(void* values)
{
	double* params = (double*)values;

	pthread_mutex_lock(&locker);
	digitalWrite(PIN_LATCH, LOW);
	shiftRegisterValue += pow(2, params[0]);
	shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, shiftRegisterValue);
	digitalWrite(PIN_LATCH, HIGH);
	pthread_mutex_unlock(&locker);
	
	usleep((int)(params[1]*1000));

	pthread_mutex_lock(&locker);
	digitalWrite(PIN_LATCH, LOW);
	shiftRegisterValue -= pow(2,params[0]);
	shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, shiftRegisterValue);
	digitalWrite(PIN_LATCH, HIGH);
	pthread_mutex_unlock(&locker);
	free(values);

}

void signalHandler(int mysignal){
	if(mysignal == SIGUSR1)
	{
		
		printf("SIGNAL SIGUSR1 RECEIVED\n");
	}

}

