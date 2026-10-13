// sampled sweep: 1MB every <step> MB across VRAM; classify bad sectors as "stuck garbage" vs other bitflips
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
static volatile uint32_t *bar;
static const uint32_t G[8]={0x264d609e,0xd3fbffe6,0xb8a48005,0x71802891,0x65765d59,0xff7a39ae,0x076be740,0x10b5f90e};
static inline uint32_t pat(uint64_t a){return (uint32_t)(a>>2)*2654435761u ^ 0x5a5a1234u;}
int main(int c,char**v){
  int fd=open("/sys/bus/pci/devices/0000:03:00.0/resource0",O_RDWR|O_SYNC);
  bar=mmap(0,16<<20,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
  uint32_t saved=bar[0x1700/4]; volatile uint32_t *w=bar+0x700000/4; int step=atoi(v[1]);
  unsigned long T=0,TG=0,TO=0,TS=0;
  for(uint64_t mb=0;mb<22528;mb+=step){uint64_t b=mb<<20;bar[0x1700/4]=b>>16;(void)bar[0x1700/4];
    for(unsigned i=0;i<262144;i++) w[i]=pat(b+i*4);}
  for(uint64_t mb=0;mb<22528;mb+=step){uint64_t b=mb<<20;bar[0x1700/4]=b>>16;(void)bar[0x1700/4];
    unsigned long g=0,o=0;
    for(unsigned s=0;s<32768;s++){uint32_t r[8];int bd=0;
      for(int k=0;k<8;k++){r[k]=w[s*8+k]; if(r[k]!=pat(b+s*32+k*4)) bd=1;}
      if(bd){ if(!memcmp(r,G,32)) g++; else o++; }}
    T+=g+o;TG+=g;TO+=o;TS+=32768;
    printf("%6lu MB  bad=%5lu (%.2f%%) stuck=%lu other=%lu\n",(unsigned long)mb,g+o,100.0*(g+o)/32768,g,o); fflush(stdout);}
  bar[0x1700/4]=saved;
  printf("TOTAL sampled sectors=%lu bad=%lu (%.3f%%, 1/%.2f) stuck=%lu other=%lu\n",TS,T,100.0*T/TS,(double)TS/T,TG,TO);
}
