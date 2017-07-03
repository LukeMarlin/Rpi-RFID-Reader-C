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
#include <libconfig.h>
#include "cardReader.h"
#include "main.h"
#include "pins.c"

#define debug_mode 0
#if debug_mode
	#define debugf(a) (void)0
#else
	#define debugf(a) printf a
#endif

int pins[PINS_COUNT] = {PIN_0,
		PIN_1,
		PIN_2,
		PIN_3,
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
		PIN_25,
		UNUSABLE_PIN,
        PIN_27,
        PIN_28,
        PIN_29,
        PIN_30,
        PIN_31};

//Necessary global variables for the program behaviour
char values[PINS_COUNT] = {};
long* userTags;
long* userPlusTags;
long* adminTags;
int userTagsCount = 0;
int userPlusTagsCount = 0;
int adminTagsCount = 0;
long shiftRegisterValue = 0;
int READERS_COUNT = 0;
pthread_mutex_t locker = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t updateLocker = PTHREAD_MUTEX_INITIALIZER;
int isSystemLocked = 0;
FILE* logFile;
int DBTagsVersionNumber = 0;
int updateDelay = 610; //time to reload tags base and send logs (in seconds)
int bipCount = 7;
const char* logFilePath = "logFile.txt";
const char* pythonScriptsDirectory;
const char* errFilePath = "/var/log/accessControl";
FILE* errFile;

//Configuration file vars
config_t cfg;
config_setting_t* setting;

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
	signal(SIGUSR1, signalHandler);
	digitalWrite(PIN_LATCH, LOW);
	shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, 0);
	digitalWrite(PIN_LATCH, HIGH);
	// Waste time but not CPU
	for (;;) {
		sleep(1);
	}
}

int loadConfig(){
	config_init(&cfg);

	if(!config_read_file(&cfg, "config")){
		fprintf(stderr, "%s:%d - %s\n", config_error_file(&cfg),
			config_error_line(&cfg), config_error_text(&cfg));
	config_destroy(&cfg);
	return 0;
	}

	//Get opening and closing times
	if(!config_lookup_int(&cfg, "openingHour", &openingHour))
		logErr("No 'openingHour' setting in configuration file.");

	if(!config_lookup_int(&cfg, "openingMinute", &openingMinute))
		logErr("No 'openingMinute' setting in configuration file.");

	if(!config_lookup_int(&cfg, "closingHour", &closingHour))
		logErr("No 'closingHour' setting in configuration file.");

	if(!config_lookup_int(&cfg, "closingMinute", &closingMinute))
		logErr("No 'closingMinute' setting in configuration file.");

	//Loading bip count for refused access
	if(!config_lookup_int(&cfg, "bipCount", &bipCount))
		logErr("No 'bipCount' setting in configuration file.");

	//Loading delay value between each update of tags and logs
	if(!config_lookup_int(&cfg, "updateDelay", &updateDelay))
		logErr("No 'updateDelay' setting in configuration file.");

	//Loading the log file path
	if(!config_lookup_string(&cfg, "logFilePath", &logFilePath))
		logErr("No 'logFilePath' setting in configuration file.");

	//Loading python scripts directory
	if(!config_lookup_string(&cfg, "pythonScriptsDirectory", &pythonScriptsDirectory))
		logErr("No 'pythonScriptsDirectory' setting in configuration file.");

	setting = config_lookup(&cfg, "readers");

	if(setting != NULL){
		int count = config_setting_length(setting);
		int i;

		for(i = 0; i < count; i++){
			config_setting_t* reader = config_setting_get_elem(setting, i);

			const char* name;
			int GPIO_0, GPIO_1, zone;
			int doorPin, doorTime, ledPin, ledTime, buzzerPin, buzzerTime;
			void(*cb1);
			void(*cb2);


			if(!(config_setting_lookup_string(reader, "name", &name)
			&& config_setting_lookup_int(reader, "GPIO_0", &GPIO_0)
			&& config_setting_lookup_int(reader, "GPIO_1", &GPIO_1)
			&& config_setting_lookup_int(reader, "zone", &zone)
			&& config_setting_lookup_int(reader, "doorPin", &doorPin)
			&& config_setting_lookup_int(reader, "doorTime", &doorTime)
			&& config_setting_lookup_int(reader, "ledPin", &ledPin)
			&& config_setting_lookup_int(reader, "ledTime", &ledTime)
			&& config_setting_lookup_int(reader, "buzzerPin", &buzzerPin)
			&& config_setting_lookup_int(reader, "buzzerTime", &buzzerTime)))
			{
				logErr("An error occured when initializing readers. Please check the configuration file");
				continue;
			}
			else{
				cb1 = getCorrespondingCallback(GPIO_0);
				cb2 = getCorrespondingCallback(GPIO_1);
				if(cb1 == NULL || cb2 == NULL){
					logErr("One or multiple pins doesn't have corresponding callbacks");
					continue;
				}

				createCardReader(name, GPIO_0, GPIO_1, zone, doorPin, doorTime, ledPin, ledTime, buzzerPin, buzzerTime, cb1, cb2);
			}
		}
	}
}

void initProgram(){
	wiringPiSetupGpio();
	readers = (CardReader**)malloc(sizeof(CardReader*)*PINS_COUNT);
	logFile = fopen(logFilePath, "a");
	errFile = fopen(errFilePath, "a");

	loadConfig();

	pinMode(PIN_LATCH, OUTPUT);
	digitalWrite(PIN_LATCH, HIGH);
	pinMode(PIN_DATA, OUTPUT);
	digitalWrite(PIN_DATA, LOW);
	pinMode(PIN_CLOCK, OUTPUT);
	digitalWrite(PIN_CLOCK, LOW);

	initReaders();
	pthread_t bgThread;

	pthread_attr_t attr1;

	pthread_attr_init(&attr1);

	pthread_attr_setdetachstate(&attr1, PTHREAD_CREATE_DETACHED);

	pthread_create(&bgThread, &attr1, &backgroundUpdater, NULL);
}

//Here we create the readers. You must adjust the values in the config file according to your wiring and your desired behaviour
void initReaders(){
	//createCardReader("Porte principale", PIN_23, PIN_24, 1, 0, 3000, 1, 2000, 2, 2000, &callback23, &callback24);
	//createCardReader("Porte annexe", PIN_7, PIN_25, 1, 3, 3000, 4, 2000, 5, 2000, &callback7, &callback25);
}

void createCardReader(const char* pname, int pGPIO_0, int pGPIO_1, int zone, double doorPin, double doorTime, double ledPin, double ledTime, double buzzerPin, double buzzerTime, void(*callback0), void(*callback1)){
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
	updateArrays(temp);
	wiringPiISR(pGPIO_0, INT_EDGE_FALLING, callback0);
	wiringPiISR(pGPIO_1, INT_EDGE_FALLING, callback1);

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

    if(reader->zone <= 2){
        for(i=0;i<userPlusTagsCount; i++){
            if(userPlusTags[i] == *tag) return 1;
        }
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

	for(callCount = 0; callCount < bipCount; callCount++){
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
	debugf(("Opening %s...\n", tempReader->name));

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

	long changeValue = (long)pow(2, params[0]);
	if(params[2] !=-1)
		changeValue += (long)pow(2, params[1]);
	shiftRegisterValue += changeValue;

	uint8_t firstRegister = (uint8_t)(shiftRegisterValue >> 16);
	uint8_t secondRegister = (uint8_t)(shiftRegisterValue >> 8);
	uint8_t thirdRegister = (uint8_t)shiftRegisterValue;

	shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, firstRegister);
	shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, secondRegister);
	shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, thirdRegister);

	digitalWrite(PIN_LATCH, HIGH);
	pthread_mutex_unlock(&locker);

	usleep((int)(params[1]*1000));

	pthread_mutex_lock(&locker);
	digitalWrite(PIN_LATCH, LOW);

	shiftRegisterValue -= changeValue;
	firstRegister = (uint8_t)(shiftRegisterValue >> 16);
	secondRegister = (uint8_t)(shiftRegisterValue >> 8);
	thirdRegister = (uint8_t)shiftRegisterValue;

	shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, firstRegister);
	shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, secondRegister);
	shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, thirdRegister);

	digitalWrite(PIN_LATCH, HIGH);
	pthread_mutex_unlock(&locker);
	free(values);

}

void signalHandler(int mysignal){
	if(mysignal == SIGUSR1)
	{
		updateProgramData();
	}
}

void lockSystem(){
	isSystemLocked = 1;

	//Create a thread that makes every reader blink regularly
	pthread_t blinkThread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&blinkThread, &attr, &blinkReaders, NULL);
}

void unlockSystem(){
	isSystemLocked = 0;
}

void* blinkReader(void* reader){
	CardReader* tempReader = (CardReader*)reader;

	pthread_t blinkThread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	pthread_create(&blinkThread, &attr, &updateOutput, NULL);
}

void* blinkReaders(void* param){
	int i = 0;
	long changeValue = 0;

	for(i=0; i<READERS_COUNT; i++){
		changeValue += (long)pow(2, _readers[i].led);
	}

	while(isSystemLocked == 1){

		pthread_mutex_lock(&locker);
		digitalWrite(PIN_LATCH, LOW);

		shiftRegisterValue += changeValue;

		uint8_t firstRegister = (uint8_t)(shiftRegisterValue >> 16);
		uint8_t secondRegister = (uint8_t)(shiftRegisterValue >> 8);
		uint8_t thirdRegister = (uint8_t)shiftRegisterValue;

		shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, firstRegister);
		shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, secondRegister);
		shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, thirdRegister);
		digitalWrite(PIN_LATCH, HIGH);
		pthread_mutex_unlock(&locker);

		usleep((int)(250*1000));

		pthread_mutex_lock(&locker);
		digitalWrite(PIN_LATCH, LOW);

		shiftRegisterValue -= changeValue;
		firstRegister = (uint8_t)(shiftRegisterValue >> 16);
		secondRegister = (uint8_t)(shiftRegisterValue >> 8);
		thirdRegister = (uint8_t)shiftRegisterValue;

		shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, firstRegister);
		shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, secondRegister);
		shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, thirdRegister);

		digitalWrite(PIN_LATCH, HIGH);
		pthread_mutex_unlock(&locker);

		usleep(250*1000);
	}

}

void createLogEntry(const char* readerName, long tagNumber, int isAccepted){
	//Checks if the file is correctly opened
	//if(!logFile == NULL){

		time_t currentTimeStamp;
		time(&currentTimeStamp);
		struct tm* currentTime;
		currentTime = gmtime(&currentTimeStamp);

		//Writes the line at the end of the file
		fprintf(logFile, "%d-%d-%d %d:%d:%d,%s,%ld,%d;", currentTime->tm_year + 1900, currentTime->tm_mon+1, currentTime->tm_mday,
			currentTime->tm_hour, currentTime->tm_min, currentTime->tm_sec, readerName, tagNumber, isAccepted);
		fflush(logFile);
	//}
}

void* backgroundUpdater(void* param){
	for(;;){
		updateProgramData();
		sleep(updateDelay);
	}
}

int updateProgramData(){
	pthread_mutex_lock(&updateLocker);
	int oldDBTagsVersionNumber = DBTagsVersionNumber;
	char str[1000];
	char result[22];

	fclose(logFile);

	lockSystem();
	debugf(("Downloading tags database..."));
	fflush(stdout);
	sprintf(str, "python3 %sgetTags.py -t %d", pythonScriptsDirectory, oldDBTagsVersionNumber);
	runScript(str, result);
	DBTagsVersionNumber = strtol(result, NULL, 10);
	loadTagsFile(&userTags, "userTags.txt", &userTagsCount);
	loadTagsFile(&userPlusTags, "userPlusTags.txt", &userPlusTagsCount);
	loadTagsFile(&adminTags, "adminTags.txt", &adminTagsCount);

	debugf(("Done !\n"));
	debugf(("Sending logs..."));
	fflush(stdout);

	sprintf(str, "python3 %ssendLogFile.py", pythonScriptsDirectory);
	runScript(str, result);

	debugf(("Done !\n"));

	logFile = fopen("logFile.txt", "a");
	unlockSystem();

	pthread_mutex_unlock(&updateLocker);
	debugf(("Update complete\n"));
	return 1;

}
void runScript(char* command, char* result){
	char* data;
	FILE* stream;

	stream = popen(command, "r");
	fgets(result, 22, stream);
	fclose(stream);
}

void* getCorrespondingCallback(int pin){


	switch(pin){
		case 0:
			return callback0; break;
		case 1:
			return callback1; break;
		case 2:
			return callback2; break;
		case 3:
			return callback3; break;
		case 4:
			return callback4; break;
		case 7:
			return callback7; break;
		case 8:
			return callback8; break;
		case 9:
			return callback9; break;
		case 10:
			return callback10; break;
		case 11:
			return callback11; break;
		case 14:
			return callback14; break;
		case 15:
			return callback15; break;
		case 17:
			return callback17; break;
		case 18:
			return callback18; break;
		case 21:
			return callback21; break;
		case 22:
			return callback22; break;
		case 23:
			return callback23; break;
		case 24:
			return callback24; break;
		case 25:
			return callback25; break;
        case 27:
            return callback27; break;
        case 28:
            return callback28; break;
        case 29:
            return callback29; break;
        case 30:
            return callback30; break;
        case 31:
            return callback31; break;
		default:
			return NULL; break;
	}
}

void logErr(char* error){

if(errFile != NULL){
	time_t currentTimeStamp;
	time(&currentTimeStamp);
	struct tm* currentTime;
	currentTime = gmtime(&currentTimeStamp);
	fprintf(errFile, "%d-%d-%d %d:%d:%d: %s\n", currentTime->tm_year + 1900, currentTime->tm_mon+1, currentTime->tm_mday,
			currentTime->tm_hour, currentTime->tm_min, currentTime->tm_sec, error);
	fflush(errFile);
	}
}
