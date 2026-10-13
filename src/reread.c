// for first 4MB: count bad sectors, and check whether repeated reads of bad sectors return the same data
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
static inline uint32_t pat(uint64_t a){return (uint32_t)(a>>2)*2654435761u ^ 0x77aa1100u;}
int main(){int fd=open("/sys/bus/pci/devices/0000:03:00.0/resource0",O_RDWR|O_SYNC);
 volatile uint32_t*bar=mmap(0,16<<20,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0),*w=bar+0x700000/4;uint32_t sv=bar[0x1700/4];
 for(int mb=0;mb<4;mb++){bar[0x1700/4]=(mb<<20)>>16;(void)bar[0x1700/4];for(unsigned i=0;i<262144;i++)w[i]=pat(((uint64_t)mb<<20)+i*4);}
 unsigned long bad=0,unstable=0,partial=0;
 for(int mb=0;mb<4;mb++){bar[0x1700/4]=(mb<<20)>>16;(void)bar[0x1700/4];
  for(unsigned s=0;s<32768;s++){uint32_t a[8],b[8];int nb=0;
   for(int k=0;k<8;k++){a[k]=w[s*8+k]; if(a[k]!=pat(((uint64_t)mb<<20)+s*32+k*4))nb++;}
   if(nb){bad++; if(nb<8)partial++; for(int r=0;r<5;r++){for(int k=0;k<8;k++)b[k]=w[s*8+k]; if(memcmp(a,b,32)){unstable++;break;}}}}}
 bar[0x1700/4]=sv; printf("bad sectors=%lu  partially-bad=%lu  changing-between-reads=%lu\n",bad,partial,unstable);}
