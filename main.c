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

//Necessary global variables for the program behaviour
char values[PINS_COUNT] = {};
long* userTags;
long* clubTags;
long* adminTags;
int userTagsCount = 0;
int clubTagsCount = 0;
int adminTagsCount = 0;
uint8_t shiftRegisterValue = 0;
int READERS_COUNT = 0;
pthread_mutex_t locker = PTHREAD_MUTEX_INITIALIZER;
int isSystemLocked = 0;
FILE* logFile;
int DBTagsVersionNumber = 0;

//Configuration lines for opening and closing times
int openingHour = 7;
int openingMinute = 30;
int closingHour = 23;
int closingMinute = 0;

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
	
	logFile = fopen("logFile.txt", "a");
	
	pinMode(PIN_LATCH, OUTPUT);
	digitalWrite(PIN_LATCH, HIGH);	
	pinMode(PIN_DATA, OUTPUT);
	digitalWrite(PIN_DATA, LOW);
	pinMode(PIN_CLOCK, OUTPUT);
	digitalWrite(PIN_CLOCK, LOW);

	pthread_t bgThread;
	pthread_create(&bgThread, NULL, &backgroundUpdater, NULL);
}

//Here we create the readers. You must adjust the values according to your wiring and your desired behaviour
void initReaders(){
	createCardReader("Porte principale", PIN_23, PIN_24, 1, 0, 3000, 1, 2000, 2, 2000, &callback23, &callback24);
	createCardReader("Porte annexe", PIN_7, PIN_25, 1, 3, 3000, 4, 2000, 5, 2000, &callback7, &callback25);
}

void createCardReader(char* pname, int pGPIO_0, int pGPIO_1, int zone, double doorPin, double doorTime, double ledPin, double ledTime, double buzzerPin, double buzzerTime, void(*callback0), void(*callback1)){
	CardReader* temp = malloc(sizeof(CardReader));
	
	temp->name = pname;
	temp->GPIO_0 = pGPIO_0;
	temp->GPIO_1 = pGPIO_1;
	temp->zone = zone;
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

//update the readers and values arrays
void updateArrays(CardReader* reader){
	_readers = realloc(_readers, sizeof(CardReader)*(READERS_COUNT+1));
	_readers[READERS_COUNT] = *reader;

	readers[reader->GPIO_0] = reader;
	readers[reader->GPIO_1] = reader;
	values[reader->GPIO_0] = '0';
	values[reader->GPIO_1] = '1';

	READERS_COUNT++;
}

//Check the parity of a specified tag
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

//Retrieves the integer value from a binary value stored as a char array
long int getIntFromTag(char* tag){
	char* temp = tag;

	temp[0] = '0';
	long int result;
	result = strtol(temp, 0, 2);
	result = result >> 1;
	return result;
}
//Load a specific file of tags inside an array
int loadTagsFile(long** tagsArray, char* filePath, int* tagCounter){
	long* array = malloc(0);

	//Create a thread that makes every reader blink regularly
	pthread_t blinkThread;
	pthread_create(&blinkThread, NULL, &blinkReaders, NULL);

	//Open the tags file
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
				free(buffer);
				buffer = (char*)malloc(0);
				tagCount++;
				charCount = 0;
			}
		}
	*tagCounter = tagCount;
	*tagsArray = array;
	free(buffer);
	fclose(tagsFile);
	
	return 1;
	}
}

//Check if the tag can open the door according to it's presence
//on the files and the specified time
int checkAuthorization(long* tag, CardReader* reader){
	int i;

	if(reader->zone == 1){		
		for(i=0;i<userTagsCount; i++){
			if(userTags[i] == *tag && areCourtsOpened()) return 1;
		}
	}
	for(i=0;i<clubTagsCount; i++){
		if(clubTags[i] == *tag && areCourtsOpened()) return 1;
	}
	for(i=0;i<adminTagsCount; i++){
		if(adminTags[i] == *tag) return 1;
	}
	return 0;
}

//Compares current time to opening and closing time
int areCourtsOpened(){
	time_t currentTimeStamp;
	time(&currentTimeStamp);
	struct tm* currentTime;
	currentTime = gmtime(&currentTimeStamp);
	
	if(currentTime->tm_hour*60 + currentTime->tm_min >= openingHour*60 +openingMinute
	   && currentTime->tm_hour*60 + currentTime->tm_min <= closingHour*60 + closingMinute)
		return 1;
	return 0;
}

//Pilots he reader to show the user he's being refused
void* refuseAccess(void* reader){
	CardReader* tempReader = (CardReader*)reader;

	tempReader->isOpening = 1;
	double* ledValues;
	double* buzzerValues;
	int callCount = 0;

	for(callCount = 0; callCount < 7; callCount++){
		ledValues = (double*)malloc(sizeof(double)*3);
		ledValues[0] = tempReader->led;
		ledValues[1] = 100;	
		ledValues[2] = -1;
	
		updateOutput((void*)ledValues);

		buzzerValues = (double*)malloc(sizeof(double)*3);
		buzzerValues[0] = tempReader->buzzer;
		buzzerValues[1] = 100;
		buzzerValues[2] = -1;

		updateOutput((void*)buzzerValues);

		usleep(40000);
	}
	tempReader->isOpening = 0;
}

//pilots the reader to show the user he's being accepted
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
	
	double* doorValues = (double*)malloc(sizeof(double)*3);
	doorValues[0] = tempReader->door;
	doorValues[1] = tempReader->doorTime;
	doorValues[2] = -1;
	double* ledValues = (double*)malloc(sizeof(double)*3);
	ledValues[0] = tempReader->led;
	ledValues[1] = tempReader->ledTime;
	ledValues[2] = -1;
	double* buzzerValues = (double*)malloc(sizeof(double)*3);
	buzzerValues[0] = tempReader->buzzer;
	buzzerValues[1] = tempReader->buzzerTime;
	buzzerValues[2] = -1;

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

	uint8_t changeValue = (uint8_t)pow(2, params[0]);
	if(params[2] !=-1)
		changeValue += (uint8_t)pow(2, params[1]);
	shiftRegisterValue += changeValue;

	shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, shiftRegisterValue);
	digitalWrite(PIN_LATCH, HIGH);
	pthread_mutex_unlock(&locker);
	
	usleep((int)(params[1]*1000));

	pthread_mutex_lock(&locker);
	digitalWrite(PIN_LATCH, LOW);
	shiftRegisterValue -= changeValue;
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

void lockSystem(){
	isSystemLocked = 1;
}

void unlockSystem(){
	isSystemLocked = 0;
}

void* blinkReader(void* reader){
	double* values = (double*)malloc(sizeof(double)*3);
	CardReader* tempReader = (CardReader*)reader;
	
	pthread_t blinkThread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	values[0] = tempReader->led;
	printf("\n led=%f\n", tempReader->led);
	values[1] = 300;
	values[2] = 0;

	pthread_create(&blinkThread, &attr, &updateOutput, (double*)values);
}

void* blinkReaders(void* param){
	int i = 0;

		
	while(isSystemLocked == 1){

			

		for(i=0; i<READERS_COUNT; i++){
			blinkReader(readers[_readers[i].GPIO_1]);
		}
		usleep(500000);	
	}
}

void createLogEntry(char* readerName, long tagNumber, int isAccepted){
	//Checks if the file is correctly opened
//	if(!logFile == NULL){

		time_t currentTimeStamp;
		time(&currentTimeStamp);
		struct tm* currentTime;
		currentTime = gmtime(&currentTimeStamp);

		//Writes the line at the end of the file
		fprintf(logFile, "%d-%d-%d %d:%d:%d,%s,%ld,%d;", currentTime->tm_year + 1900, currentTime->tm_mon+1, currentTime->tm_mday, 
			currentTime->tm_hour, currentTime->tm_min, currentTime->tm_sec, readerName, tagNumber, isAccepted);
		fflush(logFile);
//	}	
}

void* backgroundUpdater(void* param){
	for(;;){
		lockSystem();
		int oldDBTagsVersionNumber = DBTagsVersionNumber;
		char str[21];
		char result[22];
		
		fclose(logFile);
		
		sprintf(str, "python3 ../getUserTags.py -t %d -s 1", oldDBTagsVersionNumber);
		runScript(str, result);
		DBTagsVersionNumber = strtol(result, NULL, 10);
		loadTagsFile(&userTags, "userTags.txt", &userTagsCount);

		sprintf(str, "python3 ../getUserTags.py -t %d -s 2", oldDBTagsVersionNumber);
		runScript(str, result);
		loadTagsFile(&clubTags, "clubTags.txt", &clubTagsCount);
		
		sprintf(str, "python3 ../getUserTags.py -t %d -s 3", oldDBTagsVersionNumber);
		runScript(str, result);
		loadTagsFile(&adminTags, "adminTags.txt", &adminTagsCount);
	
		sprintf(str, "python3 ../sendLogFile.py");
		runScript(str, result);
	
		logFile = fopen("logFile.txt", "a");

		unlockSystem();

		sleep(7200);
	}
}

void runScript(char* command, char* result){
	char* data;
	FILE* stream;
	
	stream = popen(command, "r");
	fgets(result, 22, stream);
}
