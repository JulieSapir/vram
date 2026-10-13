// Stability/behaviour of bad 32B sectors over first N MB: run patterns, collect bad sector sets
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
static volatile uint32_t *bar;
#define NMB 16
static uint8_t bad[3][NMB<<15]; // per 32B sector
static inline uint32_t pat(int p,uint64_t a){uint32_t x=(uint32_t)(a>>2)*2654435761u+p*0x1234567u; return p==1?0:p==2?0xffffffff:x;}
int main(){
  int fd=open("/sys/bus/pci/devices/0000:03:00.0/resource0",O_RDWR|O_SYNC);
  bar=mmap(0,16<<20,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
  uint32_t saved=bar[0x1700/4]; volatile uint32_t *w=bar+0x700000/4;
  uint32_t g0[8]; int g0set=0; unsigned long samegarb=0;
  for(int p=0;p<3;p++){
    for(int mb=0;mb<NMB;mb++){uint64_t b=(uint64_t)mb<<20;bar[0x1700/4]=b>>16;(void)bar[0x1700/4];
      for(unsigned i=0;i<262144;i++) w[i]=pat(p,b+i*4);}
    unsigned long n=0;
    for(int mb=0;mb<NMB;mb++){uint64_t b=(uint64_t)mb<<20;bar[0x1700/4]=b>>16;(void)bar[0x1700/4];
      for(unsigned s=0;s<32768;s++){uint32_t r[8];int bd=0;
        for(int k=0;k<8;k++){r[k]=w[s*8+k]; if(r[k]!=pat(p,b+s*32+k*4)) bd=1;}
        if(bd){bad[p][(mb<<15)+s]=1;n++; if(!g0set){memcpy(g0,r,32);g0set=1;} else if(!memcmp(g0,r,32)) samegarb++;}}}
    printf("pattern %d: bad sectors %lu / %d (%.2f%%)\n",p,n,NMB<<15,100.0*n/(NMB<<15));
  }
  unsigned long both=0,any=0;
  for(int i=0;i<(NMB<<15);i++){int c=bad[0][i]+bad[1][i]+bad[2][i]; if(c)any++; if(c==3)both++;}
  printf("sectors bad in all 3 runs: %lu, bad in any: %lu, identical garbage block count: %lu\n",both,any,samegarb);
  printf("garbage: "); for(int k=0;k<8;k++)printf("%08x ",g0[k]); printf("\n");
  // stride analysis: period of bad pattern (pattern 0) in sector index
  for(int per=1;per<=1<<15;per<<=1){unsigned long m=0;for(int i=0;i<(NMB<<15);i++) if(bad[0][i]!=bad[0][i%per]) m++; if(!m){printf("bad map periodic with period %d sectors (%d bytes)\n",per,per*32);break;}}
  bar[0x1700/4]=saved;
}
