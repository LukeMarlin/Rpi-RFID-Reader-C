#include "pins.c"
#include <pthread.h>

void handler(int PIN_ID){
	
	CardReader* reader = readers[PIN_ID];
	pthread_mutex_lock(&reader->lockObj);

	struct timespec newTime;
	if(clock_gettime(CLOCK_REALTIME, &newTime) != 0)
		printf("Error N° : %d\n", errno);

	
	//reader->bitCount++;
	
	//Buffer empty, start of frame
	printf("size = %d\n", reader->bitCount);
	if(reader->bitCount == 0){
		reader->lastUpdated = newTime;
		//printf("%d, %d \n", reader->lastUpdated.tv_sec, newTime.tv_sec);
		reader->bitCount++;
	}
	//Buffer not empty
	else{ 
		//Last bit is outdated = corrupted buffer
		if(isTimedOut(reader->lastUpdated, newTime)){
			//printf("%d : Timed out !\n", ++counter);
			printf("Line %d : buffer size %d \n", ++counter,reader->bitCount);
			reader->bitCount = 0;
		}
		//End of frame
		else if(++reader->bitCount == FRAME_SIZE) { 
			printf("Line %d : Done !\n", ++counter);
			reader->bitCount = 0;
		}
		//Add bit
		else{
			reader->bitCount++;
			reader->lastUpdated = newTime;
		}
	}

	pthread_mutex_unlock(&reader->lockObj);
	//printf("PIN: %d, Readers[%d]: %d\n", PIN_ID, PIN_ID, values[PIN_ID]);
	//fflush(stdin);
	
}

int isTimedOut(struct timespec start, struct timespec end){
	long int secDelta = end.tv_sec - start.tv_sec;
	long int nsecDelta = end.tv_nsec - start.tv_nsec;
	
	if(secDelta == 0) 
	{
		if(nsecDelta < BIT_TIMEOUT  )
		{
			 return 0;
		}
		else
		{
			printf("Timed out in same second\n");
			return 1;
		}
	}
	else
	{
		if(((1000000000 - start.tv_nsec) + end.tv_nsec) < BIT_TIMEOUT  ) 
		{
			return 0;
		}
		else
		{
			printf("Timed out in different second, nstart : %ld, nstop : %ld, nsecDela : %ld \n", start.tv_nsec, end.tv_nsec, nsecDelta );
			printf("secDelta : %ld, start tvSec : %ld, end tvSec : %ld \n ", secDelta, start.tv_sec, end.tv_sec );
			return 1;
		}
	}
	return 1;
}

void callback0(){handler(PIN_0);}
void callback1(){handler(PIN_1);}
void callback4(){handler(PIN_4);}
void callback7(){handler(PIN_7);}
void callback8(){handler(PIN_8);}
void callback9(){handler(PIN_9);}
void callback10(){handler(PIN_10);}
void callback11(){handler(PIN_11);}
void callback14(){handler(PIN_14);}
void callback15(){handler(PIN_15);}
void callback17(){handler(PIN_17);}
void callback18(){handler(PIN_18);}
void callback21(){handler(PIN_21);}
void callback22() {handler(PIN_22);}
void callback23() {handler(PIN_23);}
void callback24() {handler(PIN_24);}
void callback25() {handler(PIN_25);}
