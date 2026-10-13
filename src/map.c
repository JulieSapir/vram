// write pattern over [s,e) MB, read back, print histogram of bad offsets by various address bits
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
static volatile uint32_t *bar;
static inline uint32_t pat(uint64_t a){return (uint32_t)(a>>2)*2654435761u ^ (uint32_t)(a>>32);}
int main(int c,char**v){
  int fd=open("/sys/bus/pci/devices/0000:03:00.0/resource0",O_RDWR|O_SYNC);
  bar=mmap(0,16<<20,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
  uint64_t s=atoll(v[1]),e=atoll(v[2]); uint32_t saved=bar[0x1700/4];
  volatile uint32_t *w=bar+0x700000/4;
  for(uint64_t mb=s;mb<e;mb++){uint64_t b=mb<<20;bar[0x1700/4]=b>>16;(void)bar[0x1700/4];
    for(unsigned i=0;i<262144;i++) w[i]=pat(b+i*4);}
  static unsigned long h256[256]; static unsigned long hval_top=0; uint32_t topv=0; unsigned long tot=0;
  FILE*f=fopen("/root/gpucheck/bad_addrs.txt","w");
  // find 64KB-chunk granularity: record per-4KB block error count for the first MB
  for(uint64_t mb=s;mb<e;mb++){uint64_t b=mb<<20;bar[0x1700/4]=b>>16;(void)bar[0x1700/4];
    for(unsigned i=0;i<262144;i++){uint64_t a=b+i*4;uint32_t g=w[i],x=pat(a);
      if(g!=x){tot++; h256[(a>>8)&0xff]++; if(tot<200000) fprintf(f,"%010lx %08x %08x\n",(unsigned long)a,x,g);
        if(g==0x264d609e) hval_top++;}}}
  fclose(f); bar[0x1700/4]=saved;
  printf("total bad=%lu, ==264d609e: %lu\n",tot,hval_top);
  printf("by bits[15:8] (256B blocks within 64K):\n");
  for(int i=0;i<256;i++){printf("%5lu%s",h256[i],(i%16==15)?"\n":" ");}
  // which address had value 264d609e?
  for(uint64_t a=0;a<(22528ull<<20);a+=4) if(pat(a)==0x264d609e){printf("pat==264d609e at %lx\n",(unsigned long)a);}
}
