#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>

#include "ksyms_subr.h"

static void
usage(void)
{
	fprintf(stderr, "usage: pwrite [-pk1248] <address|symbols> [writevalue]\n");
	fprintf(stderr, "\t-k	kernel memory. using /dev/kmem (default)\n");
	fprintf(stderr, "\t-p	physical memory. using /dev/mem\n");
	fprintf(stderr, "\t-1	read/write byte\n");
	fprintf(stderr, "\t-2	read/write 2byte\n");
	fprintf(stderr, "\t-4	read/write 4byte (default)\n");
	fprintf(stderr, "\t-8	read/write 8byte\n");
	exit(EX_USAGE);
}

uintptr_t
hex(const char *str)
{
	char *p;
	unsigned long long int x;

	x = strtoull(str, &p, 16);
	if ((p - str) == strlen(str))
		return x;

	return -1;
}

int
main(int argc, char *argv[])
{
	extern char *optarg;
	extern int optind;

	char *devmem;
	int rw, ch, fd, kmem;
	size_t rwsize;
	ssize_t rc;
	uintptr_t address;
	uint64_t value64, ovalue64;
	uint32_t value32, ovalue32;
	uint16_t value16, ovalue16;
	uint8_t value8, ovalue8;
	void *valuep, *ovaluep;

	/* default value */
	kmem = 1;
	rwsize = 4;
	valuep = &value32;
	ovaluep = &ovalue32;

	while ((ch = getopt(argc, argv, "1248kp")) != -1) {
		switch (ch) {
		case '1':
			valuep = &value8;
			ovaluep = &ovalue8;
			rwsize = 1;
			break;
		case '2':
			valuep = &value16;
			ovaluep = &ovalue16;
			rwsize = 2;
			break;
		case '4':
			valuep = &value32;
			ovaluep = &ovalue32;
			rwsize = 4;
			break;
		case '8':
			valuep = &value64;
			ovaluep = &ovalue64;
			rwsize = 8;
			break;
		case 'k':
			kmem = 1;
			break;
		case 'p':
			kmem = 0;
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if ((argc != 1) && (argc != 2))
		usage();

	address = hex(argv[0]);
	if (address == -1) {
		ksyms_load();
		address = ksyms_lookup(argv[0]);
		if (address == -1) {
			errx(EX_UNAVAILABLE, "%s: symbol not found\n", argv[0]);
		}
	}

	switch (argc) {
	case 2:
		rw = 1;
		value8 = value16 = value32 = value64 =
		    strtoull(argv[1], NULL, 16);
		break;
	case 1:
		rw = 0;
		break;
	default:
		usage();
	}

	devmem = kmem ? "/dev/kmem" : "/dev/mem";
	fd = open(devmem, rw ? O_RDWR : O_RDONLY);
	if (fd < 0) {
		err(EX_OSERR, "open: %s", devmem);
	}

	ovalue8 = ovalue16 = ovalue32 = ovalue64 = 0;
	rc = pread(fd, ovaluep, rwsize, (off_t)address);
	if (rc != rwsize) {
		err(EX_OSERR, "read");
	}

	if (rw)
		printf("orig: ");

	if (value64 == 0) {
		printf("*0x%lx = 0\n", address);
	} else {
		switch (rwsize) {
		case 1:
			printf("*0x%lx = 0x%02x\n", address, ovalue8);
			break;
		case 2:
			printf("*0x%lx = 0x%04x\n", address, ovalue16);
			break;
		case 4:
			printf("*0x%lx = 0x%08x\n", address, ovalue32);
			break;
		case 8:
		default:
			printf("*0x%lx = 0x%016lx\n", address, ovalue64);
			break;
		}
	}

	if (rw) {
		rc = pwrite(fd, valuep, rwsize, (off_t)address);
		if (rc != rwsize)
			err(EX_OSERR, "write");

		/* verify */
		value8 = value16 = value32 = value64 = 0;
		rc = pread(fd, valuep, rwsize, (off_t)address);
		if (rc != rwsize) {
			err(EX_OSERR, "read");
		}

		printf("new:  ");
		switch (rwsize) {
		case 1:
			printf("*0x%lx = 0x%02x\n", address, value8);
			break;
		case 2:
			printf("*0x%lx = 0x%04x\n", address, value16);
			break;
		case 4:
			printf("*0x%lx = 0x%08x\n", address, value32);
			break;
		case 8:
		default:
			printf("*0x%lx = 0x%016lx\n", address, value64);
			break;
		}
	}

	close(fd);

	return 0;
}
