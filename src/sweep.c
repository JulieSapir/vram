// Full VRAM sweep through PRAMIN: write whole range, then verify (defeats L2 cache).
// usage: sweep <start_MB> <end_MB> <pass:0=addr,1=~addr,2=0x55/aa checker> 
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
static volatile uint32_t *bar;
static inline uint32_t pat(int p,uint64_t a){
  uint32_t x=(uint32_t)(a>>2)*2654435761u ^ (uint32_t)(a>>32);
  return p==0?x : p==1?~x : ((a>>2)&1?0x55555555:0xaaaaaaaa)^((a>>20)&1?~0u:0);
}
static double now(){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
int main(int c,char**v){
  int fd=open("/sys/bus/pci/devices/0000:03:00.0/resource0",O_RDWR|O_SYNC);
  bar=mmap(0,16<<20,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
  if(bar==MAP_FAILED){perror("mmap");return 1;}
  uint64_t s=atoll(v[1]),e=atoll(v[2]); int p=atoi(v[3]);
  uint32_t saved=bar[0x1700/4];
  volatile uint32_t *w=bar+0x700000/4;
  double t0=now();
  for(uint64_t mb=s;mb<e;mb++){ uint64_t b=mb<<20;
    bar[0x1700/4]=(uint32_t)(b>>16); (void)bar[0x1700/4];
    for(unsigned i=0;i<262144;i++) w[i]=pat(p,b+i*4);
  }
  double t1=now();
  unsigned long tot=0; uint32_t bitmask_all=0; unsigned long bits[32]={0};
  for(uint64_t mb=s;mb<e;mb++){ uint64_t b=mb<<20;
    bar[0x1700/4]=(uint32_t)(b>>16); (void)bar[0x1700/4];
    unsigned long err=0,acc=0; uint32_t m=0; unsigned first=0; uint32_t fw=0,fg=0;
    for(unsigned i=0;i<262144;i++){uint32_t want=pat(p,b+i*4),got=w[i];
      if(got!=want){ if(!err){first=i*4;fw=want;fg=got;} err++;
        if((got&0xffffff00)==0xbad0ac00) acc++; else {uint32_t x=got^want; m|=x; for(int k=0;k<32;k++) if(x>>k&1) bits[k]++;}}}
    if(err){ tot+=err; bitmask_all|=m;
      printf("%6lu MB err=%lu badacc=%lu mask=%08x first@+%05x want=%08x got=%08x\n",(unsigned long)mb,err,acc,m,first,fw,fg);
      fflush(stdout);}
  }
  double t2=now();
  bar[0x1700/4]=saved;
  printf("pass %d range %lu-%lu MB: total errors=%lu mask=%08x  write %.1fs (%.0f MB/s) read %.1fs (%.0f MB/s)\n",
    p,(unsigned long)s,(unsigned long)e,tot,bitmask_all,t1-t0,(e-s)/(t1-t0),t2-t1,(e-s)/(t2-t1));
  if(bitmask_all){printf("per-bit flip counts:");for(int k=0;k<32;k++)if(bits[k])printf(" b%d=%lu",k,bits[k]);printf("\n");}
}
