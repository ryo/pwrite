pwrite
======
pwrite is a command to read or write a data from/to memory.

usage
=====
read a data from kmem
```
# nm /netbsd|grep ipsec_debug
/netbsd:ffffffc000619100 D ipsec_debug

# sysctl net.inet.ipsec.debug
net.inet.ipsec.debug = 0

# pwrite -k -4 ipsec_debug
*0xffffffc000619120 = 0x00000000
```

write a data to kmem
```
# sysctl net.inet.ipsec.debug
net.inet.ipsec.debug = 0

# pwrite -k -4 ipsec_debug 1
BEFORE: *0xffffffc000619120 = 0x00000000
AFTER:  *0xffffffc000619120 = 0x00000001

# sysctl net.inet.ipsec.debug
net.inet.ipsec.debug = 1
```
