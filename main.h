#define PINS_COUNT 26
#define FRAME_SIZE 26
#define MIDDLE_FRAME 13
#define READERS_COUNT_MAX 8
#define BIT_TIMEOUT 4000000
#define SECOND_IN_NS 1000000000

void* grantAccess(void*);
void* refuseAccess(void*);
void* updateOutput(void*);
void signalHandler(int);
void initProgram(void);
void initReaders(void);
void createCardReader(char*, int, int, int, double, double, double, double, double, double, void*, void*);
void updateArrays(CardReader*);
int parityCheck(char**);
long int getIntFromTag(char*);
int loadTagsFile(long**, char*, int*);
int checkAuthorization(long*, CardReader*);
int areCourtsOpened();
void lockSystem();
void unlockSystem();
void* blinkReaders(void*);
void* blinkReader(void*);
void* backgroundUpdater(void*);
void createLogEntry(char*, long, int);
void runScript(char*, char*);
int printd(char*, int);
