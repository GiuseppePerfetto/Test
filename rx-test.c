/*
 * Example receiver program
 *
 * Copyright (C) 2016 Ron Pedde (ron@pedde.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h> 
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>   
#include <sys/resource.h> 
#include "nrf24lu1p.h"

#define VERSION "0.1"
#define MAX_NRF24_DEVICES 4
#define PRINT_PAYLOAD
#define msgs_number 30 //number of complete messages
#define size 32  

//To build the project, you need to do the following, if this is your first time:
//1) autogen.sh generates a lot of useful files and the Makefile.in
//2) configure checks if everything needed is correctly installed; generates Makefile.am from Makefile.in

//Then, everytime you want to build the project:
//3) while in libnrf24lu1p-master folder execute make command
//4) sudo make install
//5) move in src/rx-test
//6) execute rx-test
int debug_level;

//function for the log printing

void print_log_msg(int level, char *format, va_list args) {
    if(level <= debug_level) {
        vfprintf(stderr, format, args);
        fprintf(stderr, "\n");
    }
}	


//if the pair of frame is a valid one, print the payloads, the sequence numbers and the transmission times on a certain file
void writeOnFile (FILE*fd, unsigned char buffer_one[size],unsigned char buffer_two[size], double first_time, double second_time){

    fprintf(fd,"%s", buffer_one);
    fprintf(fd," %f \n", first_time);
    fprintf(fd,"%s", buffer_two);
    fprintf(fd," %f \n", second_time);
    
}

void receive(nrf24radio_device_t *dev, FILE*fd){

    int res;
    int i,transfered=0;
    int first,second;
    double first_time,second_time;
    char substr[3];
    //unsigned char matrix[msgs_number][size];
    unsigned char buffer_one[size];
    unsigned char buffer_two[size];
    struct timespec start, finish; 
	
    
    while(i<msgs_number) {
	    
        if (i==0){
	    	clock_gettime(CLOCK_REALTIME, &start); 
		}
		//if last parameter in this function is 0, reading blocks the program for an undefined time. Size is opportunely chosen.
        res = nrf24radio_read_packet(dev,buffer_one,size,15);
        if(res < 0) {
			fprintf(stderr, "error reading: %s\n", nrf24radio_get_errorstr());
			exit(EXIT_FAILURE);
        } else if(res==0 && i==0){
			printf("Waiting for data.. \n");
			continue;								 //jump to next iteration.
		} else if (res==0 && i!=0) {
			 break;   								 //breaks if no data arrives till timeout and we are not waiting for the first message
		} else {
			clock_gettime(CLOCK_REALTIME,&finish);
		 	first_time=(double)(finish.tv_nsec-start.tv_nsec);    //Transmission time of the first one in the pair;
			  
			if (start.tv_nsec > finish.tv_nsec) { 				  //clock underflow condition
				first_time += 1000000000; 
			}
 
			memcpy(substr,&buffer_one[26],3);
			first=atoi(substr);									  //get the first sequence number
			clock_gettime(CLOCK_REALTIME, &start);                //restart the timer
		
		#ifdef PRINT_PAYLOAD
			printf("Payload:  [%s]\n",buffer_one);
			printf("times nanoseconds: %f\n", first_time);
			printf("first seq_number: %d\n",first);

		#endif       
		
			if((first%2)==0){                                     //the first of the pair needs to have an even seq. number
				res=nrf24radio_read_packet(dev,buffer_two,size,15);

				if(res < 0) {
					fprintf(stderr, "error reading: %s\n", nrf24radio_get_errorstr());
					exit(EXIT_FAILURE);
				} else if (res==0 && i!=0) {
					 continue;   									
				} else{

					clock_gettime(CLOCK_REALTIME,&finish);

					if (start.tv_nsec > finish.tv_nsec) { 		  // clock underflow  
						second_time += 1000000000; 
					}

					memcpy(substr,&buffer_two[26],3);
					second=atoi(substr);

					if(second==first+1){                          //the pair is valid         

						second_time=(double)(finish.tv_nsec-start.tv_nsec);
						transfered=transfered+2;
						#ifdef PRINT_PAYLOAD
							printf("Payload:  [%s]\n",buffer_two);
							printf("times nanoseconds: %f\n",second_time);
							printf("second seq_number: %d\n",second);
						#endif 
						printf("Couple received \n\n\n");

						writeOnFile(fd,buffer_one,buffer_two,first_time,second_time);    //store the results


					} else{
						printf("\n\n\n");

					}
				}

				clock_gettime(CLOCK_REALTIME, &start);

				

			} else {
				printf("\n\n\n");
			}
		
		}
	i++;
    }
    
	printf("Finished, goodbye \n");
    printf("transfered single frames: %d\n",transfered);
}






int main(int argc, char *argv[]) {

    nrf24radio_device_t *p;
    nrf24radio_device_t *dev;
    int radio_id = -1;
    int count=0;
    char serialnum[12];
    FILE *fd;
    fd=fopen("/media/sf_Prov/Prova.txt","w");                 //path of the File in which you'll store the results
    
    //initialization
    if(argc > 1) {
      strcpy(serialnum,argv[1]);
    }

    fprintf(stderr, "rx-test: version %s\n", VERSION);
	count=nrf24radio_init();
	printf("found %d devices \n",count);
	debug_level = 0;
    nrf24radio_set_log_method(print_log_msg);
	
    //no device found
    if(count==0)
    {
      printf("No devices found sorry\n");
      exit(-1);
    }
	
    //binding dei device e stampa di alcune informazioni
    for(int x=0;x<count;x++)
    {
      dev = nrf24radio_get(x);
      if(!dev) {
        fprintf(stderr, "could not open device: %s - skipping\n", nrf24radio_get_errorstr());
        continue;
      }
      printf("Found device: %s\n", dev->model);
      printf("Serial: [%s] \n", dev->serial);
      printf("Firmware Version: %g\n", dev->firmware);
      int n=strcmp(serialnum,dev->serial);
      if(n==0) {
        radio_id=x;
      }
      nrf24radio_close(dev);
      if(radio_id>=0) break;
    }
	
    printf("radio_id: %d\n",radio_id);
    fflush(stdout);
	
    //if you have multiple Dongles
    if(count>1 && radio_id<0)
    {
      printf("What number device to use? ");
      int c=getchar();
      switch(c)
      {
        case 48:
          radio_id=0;
          break;
        case 49:
          radio_id=1;
          break;
          radio_id=2;
          break;
        case 51:
          radio_id=3;
          break;
      }
    }
    
    dev = nrf24radio_get(radio_id);
    if(!dev) {
        fprintf(stderr, "Could not open device: %s\n", nrf24radio_get_errorstr());
        exit(EXIT_FAILURE);
    }
    
    printf("opened device [%s] in PRX mode\n",dev->serial);
    
    uint8_t addr[5]={0xE7,0xE7,0xE7,0xE7,0xE7};         //pipe address;
    uint8_t *address=addr;
    
	//Setting of communication parameters; exit in failure. This info MUST be consistent with what you did with the transmitter.
    
    if(nrf24radio_set_channel(dev, 100) ||
       //nrf24radio_set_data_rate(dev, DATA_RATE_250KBPS) ||
       //nrf24radio_set_data_rate(dev, DATA_RATE_1MBPS) ||
       nrf24radio_set_data_rate(dev, DATA_RATE_2MBPS) ||
       //nrf24radio_set_power(dev,POWER_M18DBM) ||
       nrf24radio_set_power(dev,POWER_0DBM) ||
	   nrf24radio_set_mode(dev, MODE_PRX)||
       nrf24radio_set_address(dev, address)){
      
			fprintf(stderr, "error setting up radio: %s\n", nrf24radio_get_errorstr());
			exit(EXIT_FAILURE);
    } else  printf("All good, waiting for data..\n"); 
    
    
	receive(dev,fd);    //Initialization ok, time to listen
	
	return 0;
    
}







