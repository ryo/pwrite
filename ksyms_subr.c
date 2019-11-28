#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define _PATH_CAT	"/bin/cat"
#define _PATH_NM	"/usr/bin/nm"
#define _PATH_SORT	"/usr/bin/sort"

struct symtbl {
	const char *symbol;
	uintptr_t addr;
};

static struct symtbl *symtbls;
static unsigned long symtbl_cur = 0;
static unsigned long symtbl_max = 0;

static struct symtbl *
symtbl_alloc(void)
{
	struct symtbl *symtbl, *newtbls;

	if (symtbl_cur >= symtbl_max) {
#define SYMTBL_NGROW	(1024 * 512)
		if (symtbl_max == 0) {
			symtbl_max += SYMTBL_NGROW;
			newtbls = malloc(sizeof(struct symtbl) * SYMTBL_NGROW);
		} else {
			symtbl_max += SYMTBL_NGROW;
			newtbls = realloc(symtbls, sizeof(struct symtbl) * symtbl_max);
		}
		if (newtbls == NULL)
			return NULL;
		symtbls = newtbls;
	}

	symtbl = symtbls + symtbl_cur++;
	return symtbl;
}

static int
symtbl_add(const char *symbol, const char *addrstr)
{
	struct symtbl *symtbl;
	const char *sym;
	uintptr_t addr;

	addr = strtoull(addrstr, NULL, 16);

	symtbl = symtbl_alloc();
	if (symtbl == NULL)
		return 1;

	symtbl->symbol = strdup(symbol);
	symtbl->addr = addr;

	return 0;
}

void
ksyms_dump(void)
{
	struct symtbl *symtbl;
	unsigned long i;

	for (i = 0, symtbl = symtbls; i < symtbl_cur; i++, symtbl++) {
		printf("%u: %lx %s\n", i, symtbl->addr, symtbl->symbol);
	}
}

const char *
ksyms_lookupsym(uintptr_t addr)
{
	struct symtbl *base, *sym;
	unsigned long lim;
	static char buf[1024];

	if (addr == 0)
		return "NULL";

	base = symtbls;

	for (lim = symtbl_cur; lim != 0; lim >>= 1) {
		sym = base + (lim >> 1);
		if (sym->addr == addr)
			return sym->symbol;
		if (sym->addr < addr) {
			base = sym + 1;
			lim--;
		}
	}

	if ((sym > symtbls) && (sym->addr > addr))
		sym--;

	snprintf(buf, sizeof(buf), "%s+0x%llx", sym->symbol, addr - sym->addr);
	return buf;
}

uintptr_t
ksyms_lookup(const char *name)
{
	struct symtbl *tbl;

	for (tbl = symtbls; tbl < symtbls + symtbl_cur; tbl++) {
		if (strcmp(tbl->symbol, name) == 0) {
			return tbl->addr;
		}
	}
	return -1;
}

void
ksyms_destroy(void)
{
	if (symtbls != NULL) {
		free(symtbls);
		symtbls = NULL;
	}
}

int
ksyms_load(void)
{
	FILE *fh;
	char tmpfile1[512];
	char tmpfile2[512];
	char cmdline[512];	/* "/bin/cat /dev/ksyms > /tmp/$$.2; /usr/bin/nm /tmp/$$.2 | sort > $$.2" */
	char linebuf[1024];
	pid_t pid = getpid();

	snprintf(tmpfile1, sizeof(tmpfile1), "/tmp/softintdump.tmp1.%u", pid);
	snprintf(tmpfile2, sizeof(tmpfile2), "/tmp/softintdump.tmp2.%u", pid);
	snprintf(cmdline, sizeof(cmdline), "%s %s > %s; %s %s | %s > %s", _PATH_CAT, _PATH_KSYMS, tmpfile1, _PATH_NM, tmpfile1, _PATH_SORT, tmpfile2);

	/* XXX */
	system(cmdline);
	unlink(tmpfile1);
	fh = fopen(tmpfile2, "r");
	unlink(tmpfile2);

	if (fh == NULL)
		return -1;

	while (fgets(linebuf, sizeof(linebuf), fh) != NULL) {
#define MAXPARAMS	8
		char *p, *tokens[MAXPARAMS], *last;
		int i;

		/* chop */
		i = strlen(linebuf);
		if (i > 0)
			linebuf[i - 1] = '\0';

		for (i = 0, p = strtok_r(linebuf, " ", &last);
		     p != NULL;
		     (p = strtok_r(NULL, " ", &last)), i++) {
			if (i < MAXPARAMS - 1)
				tokens[i] = p;
		}
		tokens[i] = NULL;

		if (tokens[2] != NULL) {
			if (symtbl_add(tokens[2], tokens[0]) != 0) {
				pclose(fh);
				ksyms_destroy();
				return -1;
			}
		}
	}

	pclose(fh);

	return 0;
}

#if 0
int
main(int argc, char *argv[])
{
	ksyms_load();

	printf("%s\n", ksyms_lookupsym(0xffffffff80e38440ULL));
	printf("0x%llx\n", ksyms_lookup("sigill_debug"));

}
#endif
