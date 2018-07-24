PROG=		pwrite
SRCS=		pwrite.c ksyms_subr.c

CFLAGS+=	-D_KMEMUSER -I${BSDSRCDIR}/sys
DPADD+=	
NOMAN=		yes

BINDIR=		/usr/local/bin

.include <bsd.prog.mk>
