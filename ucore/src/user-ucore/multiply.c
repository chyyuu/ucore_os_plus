#include <stdio.h>
#include <malloc.h>

#define    printf(...)      fprintf(1, __VA_ARGS__)

int char2int(char* c){
    int i=0;
    int res = 0;
    while(c[i] != '\0'){
        res *= 10;
        res += c[i] - '0';
        i++;
    }
    return res;
}
int main(int argc, char** argv){
    if (argc < 1){
        printf("argc is not enough\n");
        return 0;
    }
    int num = char2int(argv[1]);
    printf("matrix size: %d\n", num);
    int longth = 16*num*num;
    int* m1 = malloc(longth);
    int *m2 = malloc(longth);
    int *m3 = malloc(longth);
    printf("alloc successed\n");

    int i,j,s;
    for (i=0; i<num*num; i++){
        m1[i] = i;
        m2[i] = i;
        m3[i] = 0;
    }
    printf("init finished\n");

    /* multiply */
    for (i=0; i<num; i++){
        for (j=0; j<num;j++){
            int res = 0;
            for (s=0; s<num; s++){
                res += m1[i*num+s]*m2[s&num+j];
            }
            m3[i*num+j] = res;
        }
    }
    printf("multiply successed\n");
    
    /* free memory */
    free(m1);
    free(m2);
    printf("free memory successed\n");
    return 0;
}