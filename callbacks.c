void handler(int PIN_ID){
	
	//Getting the reader associated to the PIN that raised the event
	CardReader* reader = readers[PIN_ID];

	//Executing the function atomically
	pthread_mutex_lock(&reader->lockObj);

	//Getting current time
	struct timespec newTime;
	if(clock_gettime(CLOCK_REALTIME, &newTime) != 0)
		printf("Error N° : %d\n", errno);

	//Buffer empty, start of frame
	if(reader->bitCount == 0){
		reader->lastUpdated = newTime;
		reader->tag[reader->bitCount] = values[PIN_ID];
		reader->bitCount++;
	}
	//Buffer not empty
	else{ 
		//Last bit is outdated = corrupted buffer
		if(isTimedOut(reader->lastUpdated, newTime)){
			//printf("Buffer size %d \n", reader->bitCount);
			reader->bitCount = 1;
			reader->tag[0] = values[PIN_ID];
			reader->lastUpdated = newTime;
		}
		//End of frame
		else if(reader->bitCount == FRAME_SIZE-1) { 
			reader->tag[reader->bitCount] = values[PIN_ID];
			reader->bitCount++;
			if(parityCheck(&reader->tag)){
				long tagValue = getIntFromTag(reader->tag);	
				printf("[%s] Parity check with %d bits succeeded: %s, value = %ld => ", reader->name, reader->bitCount, reader->tag, tagValue);
				if(checkAuthorization(&tagValue) == 1){
					pthread_t thread;
					int error = 1;
					printf("Authorized !\n");
					error = pthread_create(&thread, NULL, &grantAccess, readers[PIN_ID]); 	
					if(error!=0)
						printf("error: %d", error); 
				}
				else
					printf("Refused !\n");
			}
			else{
				//printf("[%s] Parity check with %d bits failed : %s\n", reader->name, reader->bitCount, reader->tag);
			}
			reader->bitCount = 0;
		}
		//Add bit
		else{
			reader->tag[reader->bitCount] = values[PIN_ID];
			reader->bitCount++;
			reader->lastUpdated = newTime;
		}
	}

	pthread_mutex_unlock(&reader->lockObj);	
}


//Return wether the delay between last bit and previous bit is superior to BIT_TIMEOUT
int isTimedOut(struct timespec start, struct timespec end){
	long int secDelta = end.tv_sec - start.tv_sec;
	long int nsecDelta = end.tv_nsec - start.tv_nsec;
	
	if(secDelta == 0){
		if(nsecDelta < BIT_TIMEOUT  )	return 0;
	}
	else{
		if(((SECOND_IN_NS * secDelta - start.tv_nsec) + end.tv_nsec) < BIT_TIMEOUT ) 	return 0;
	}
	//printf("%ld %ld\n", secDelta, nsecDelta);
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
