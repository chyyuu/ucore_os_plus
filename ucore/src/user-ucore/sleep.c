#include <ulib.h>
#include <stdio.h>
#include <malloc.h>

#define printf(...)                     fprintf(1, __VA_ARGS__)

int main(int argc, char** argv){
    int current_ptr = 0xaff01000;
    int i;
    for (i=0; i<20; i++){
        *(unsigned char* )current_ptr = 'a';
        current_ptr += 0x1000;
    }

    int ptr = current_ptr;
    sleep(1000);
    i = 0;
    while(i++ < 7){
        printf("I am going to alloc 1Mb memory\n");
        sleep(100);
       /* printf("write 'a' to addr: 0xaff15000 \n");
        *(unsigned char* )ptr = 'a';
        printf("write 'a' to addr: 0xaff15010 \n");
        *(unsigned char* )(ptr + 0x0010) = 'a';
        printf("write 'a' to addr: 0xaff16000 \n");
        *(unsigned char* )(ptr + 0x1000) = 'a';
        printf("write 'a' to addr: 0xaff16010 \n");
        *(unsigned char* )(ptr + 0x1010) = 'a';
        printf("write 'a' to addr: 0xaff17000 \n");
        *(unsigned char* )(ptr + 0x2000) = 'a';
        printf("write 'a' to addr: 0xaff17010 \n");
        *(unsigned char* )(ptr + 0x2010) = 'a'; */
        char * addr = malloc(0x1000000);
        memset(addr, 0, 0x1000000);   
    }
    while(1){
        printf("I am sleeping now\n$ ");
        sleep(2000);
    }
}