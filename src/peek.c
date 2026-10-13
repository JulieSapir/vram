// read BAR0 registers of the GPU: ./peek <hex reg> ...
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
int main(int c,char**v){int fd=open("/sys/bus/pci/devices/0000:03:00.0/resource0",O_RDWR|O_SYNC);
 volatile uint32_t*b=mmap(0,16<<20,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);if(b==MAP_FAILED){perror("mmap");return 1;}
 for(int i=1;i<c;i++){uint32_t r=strtoul(v[i],0,16);printf("%06x = %08x\n",r,b[r/4]);}}
