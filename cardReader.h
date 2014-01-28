typedef struct CardReader{

char* name;
char* tag;
int zone;
int GPIO_0;
int GPIO_1;
double door;
double doorTime;
double led;
double ledTime;
double buzzer;
double buzzerTime;
int bitCount;
int isOpening;
struct timespec lastUpdated;
pthread_mutex_t lockObj;

}CardReader;

