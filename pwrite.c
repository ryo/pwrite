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
	uintptr_t address;
	uint64_t value, newvalue;
	ssize_t rc;

	kmem = 1;
	rwsize = 4;

	while ((ch = getopt(argc, argv, "1248kp")) != -1) {
		switch (ch) {
		case '1':
		case '2':
		case '4':
		case '8':
			rwsize = ch - '0';
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
		newvalue = strtoull(argv[1], NULL, 16);
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

	value = 0;
	rc = pread(fd, (void *)&value, rwsize, (off_t)address);
	if (rc != rwsize) {
		err(EX_OSERR, "read");
	}

	if (rw)
		printf("orig: ");

	if (value == 0) {
		printf("*0x%lx = 0\n", address);
	} else {
		switch (rwsize) {
		case 1:
			printf("*0x%lx = 0x%02lx\n", address, value);
			break;
		case 2:
			printf("*0x%lx = 0x%04lx\n", address, value);
			break;
		case 4:
			printf("*0x%lx = 0x%08lx\n", address, value);
			break;
		case 8:
		default:
			printf("*0x%lx = 0x%016lx\n", address, value);
			break;
		}
	}

	if (rw) {
		rc = pwrite(fd, (void *)&newvalue, rwsize, (off_t)address);
		if (rc != rwsize) {
			err(EX_OSERR, "write");
		}

		/* verify */
		value = 0;
		rc = pread(fd, (void *)&value, rwsize, (off_t)address);
		if (rc != rwsize) {
			err(EX_OSERR, "read");
		}

		printf("new:  ");
		switch (rwsize) {
		case 1:
			printf("*0x%lx = 0x%02lx\n", address, value);
			break;
		case 2:
			printf("*0x%lx = 0x%04lx\n", address, value);
			break;
		case 4:
			printf("*0x%lx = 0x%08lx\n", address, value);
			break;
		case 8:
		default:
			printf("*0x%lx = 0x%016lx\n", address, value);
			break;
		}
	}

	close(fd);

	return 0;
}
