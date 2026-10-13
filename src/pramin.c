// VRAM tester via BAR0 PRAMIN window (0x1700 selects 64K-aligned base, window at 0x700000, 1MB).
// usage: pramin <start_MB> <end_MB> <step_MB> <test_KB>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
static volatile uint32_t *bar;
#define RD(r) bar[(r)/4]
#define WR(r,v) (bar[(r)/4]=(v))
int main(int c,char**v){
  int fd=open("/sys/bus/pci/devices/0000:03:00.0/resource0",O_RDWR|O_SYNC);
  bar=mmap(0,16<<20,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
  if(bar==MAP_FAILED){perror("mmap");return 1;}
  uint64_t s=atoll(v[1]),e=atoll(v[2]),st=atoll(v[3]),kb=atoll(v[4]);
  uint32_t saved=RD(0x1700); printf("0x1700 was %08x\n",saved);
  uint32_t pats[]={0x00000000,0xffffffff,0x55555555,0xaaaaaaaa,0};
  unsigned n=kb*256;
  for(uint64_t mb=s;mb<e;mb+=st){
    uint64_t base=mb<<20;
    WR(0x1700,(uint32_t)(base>>16)); (void)RD(0x1700);
    volatile uint32_t *w=bar+0x700000/4;
    unsigned long err=0,bad0ac=0; uint32_t orx=0,first_got=0; unsigned first=~0u; uint32_t first_want=0;
    for(int p=0;p<5;p++){
      for(unsigned i=0;i<n;i++) w[i]= p<4?pats[p]:(uint32_t)(base+i*4)^0x9e3779b9;
      for(unsigned i=0;i<n;i++){uint32_t want=p<4?pats[p]:(uint32_t)(base+i*4)^0x9e3779b9, got=w[i];
        if(got!=want){err++; if((got&0xffffff00)==0xbad0ac00)bad0ac++; else orx|=got^want;
          if(first==~0u){first=i*4;first_got=got;first_want=want;}}}
    }
    if(!err) printf("%6lu MB ok\n",(unsigned long)mb);
    else printf("%6lu MB BAD err=%lu bad0ac=%lu bitflip_mask=%08x first@+%x want=%08x got=%08x\n",
       (unsigned long)mb,err,bad0ac,orx,first,first_want,first_got);
    fflush(stdout);
  }
  WR(0x1700,saved);
}
