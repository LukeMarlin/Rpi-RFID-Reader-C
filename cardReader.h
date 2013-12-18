typedef struct CardReader{

char* name;
char* tag;
int GPIO_0;
int GPIO_1;
double door;
double led;
double buzzer;
int bitCount;
struct timespec lastUpdated;
pthread_mutex_t lockObj;

}CardReader;

