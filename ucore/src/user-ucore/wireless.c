/*
int main()
{
	printf("Hello from Nios II!\n");
	rf212_init();
	int num;
	while (1) {
		scanf("%d", &num);
		//printf("Send %d packets!\n", num);
		uint8_t data[256];
		uint8_t len = 125;
		int i;
		for (i = 0; i < len; ++i)
			data[i] = i + 1;
		for (i = 0; i < num; ++i) {
			rf212_send(data, len);
			++data[0];
			usleep(10000);
		}
		printf("send over\n");
	}
	while (1);
	return 0;
}
*/
#include <rf212.h>
#include <ulib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

uint8_t data[256];
int timeout;
uint8_t p[131] ={"0000abcdefghijklmnopqrstuvwxyz"};

int main(int argc, char **argv)
{
    uint8_t sender = 0;
    int n = 0;
    uint32_t waitms = 1000;
    if (argc > 2) {
        sender = 1;
        char *t = argv[2];
        while (*t) {
            n = n * 10 + (*t - '0');
            ++t;
        }
        cprintf("n=%d\n", n);
    }
    if (argc > 1) {
    	waitms = 0;
    	char *t = argv[1];
    	while (*t) {
    		waitms = waitms * 10 + (*t - '0');
    		++t;
    	}
    }
    
    timeout = 0;
    
    uint8_t byte_num=0x00;
    
    uint8_t i,seq_num,firt_seq;
    uint16_t received=0,count=0, resend=0, missing=0;
    {
    	int i;
    	for (i = 5; i < 130; ++i)
    		p[i] = p[i-1] + 1;
    }
    uint32_t* number = (uint32_t*)p;
    
    if (sender == 0) {
        cprintf("I'm receiver\n");
    } else {
        cprintf("I'm sender\n");
    }
    //p[2] = 0;
    *number = 0x12345678;
    int len = 116;
    int acc = 0;
    int total = 0;
    
    while(sender == 0 || count < n){
        
        //*number = count;
        if (sender == 1) {
        	p[len - 1] = 0;
		    uint8_t checksum = 0;
		    for (i = 0; i < len; ++i)
		    	checksum += p[i];
			p[len - 1] = checksum;
	        rf212_send(len, p);

		    cprintf("#%d: %d bytes sent, number=0x%08x checksum=%x\n", count, len, *number, checksum);

		    if (waitms)
			    sleep(waitms);
        } else {            
//        	sleep(waitms / 2);
            //byte_num = JF24C_CODE_RX(data);
            
            if (byte_num == 0) {
            	if (timeout == 0)
            		cprintf("timeout ! acc/total=%d/%d\n", acc, total);
            		//cprintf("timeout !\n");
            	timeout = 1;
            	continue;
            }
            timeout = 0;
            
            int offset = 0;
            if (data[0] == 0xAA && data[1] == 0x7f && data[2] == 0x61 && data[3] == 0x88) {
            	offset = 2;
            }
            if (data[0] == 0x7f && data[1] == 0x61 && data[2] == 0x88) {
            	offset = 1;
            }

            int j;
            uint8_t checksum = 0;
            for (j = 9 + offset; j < byte_num - 3 + offset; ++j)
                checksum += data[j];
            cprintf("len=%d checksum1=%x", byte_num, checksum);
            uint8_t checksum2 = data[byte_num - 3 + offset];
			if (checksum != checksum2) {                
	            cprintf(" error! checksum2=%x\n", checksum2);
	            for (j = 0; j < byte_num; ++j)
	            	cprintf("%02x ", data[j]);
	        } else ++ acc;
	        ++total;
	        
	        cprintf("\n");
            
        }
        count++;
        (*number) ++;
    }
  
    return 0;
}
