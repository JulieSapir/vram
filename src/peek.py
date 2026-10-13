# read-only BAR0 register peek: python3 -I peek.py <hexreg> ...
import mmap, struct, sys, os
fd = os.open("/sys/bus/pci/devices/0000:03:00.0/resource0", os.O_RDONLY | os.O_SYNC)
m = mmap.mmap(fd, 16 << 20, mmap.MAP_SHARED, mmap.PROT_READ)
for a in sys.argv[1:]:
    r = int(a, 16)
    print(f"{r:06x} = {struct.unpack_from('<I', m, r)[0]:08x}")
