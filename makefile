NAME = notifyck
VERSION = "$(git rev-parse --short HEAD)"
LIBS = -L/usr/X11R6/lib -lfontconfig -lXft -lX11
CFLAGS = -I/usr/X11R6/include -I/usr/include/freetype2 -I/usr/X11R6/include/freetype2
SRC = ${NAME}.c
OBJ = ${SRC:.c=.o}
DESTDIR = /usr/local
CC = gcc

.c.o:
	@echo CC -c ${CFLAGS} $<
	@${CC} -c ${CFLAGS} -DVERSION=\"${VERSION}\" $< ${LIBS} -o ${<:.c=.o}

${NAME}: ${SRC} ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ ${CFLAGS} ${OBJ} ${LIBS}

clean:
	@echo cleaning...
	@rm -f ${NAME} ${OBJ}

install: ${NAME}
	@echo installing executable file to ${DESTDIR}/bin
	@mkdir -p ${DESTDIR}/bin
	@cp -f ${NAME} ${DESTDIR}/bin/${NAME}
	@chmod 755 ${DESTDIR}/bin/${NAME}
uninstall: ${NAME}
	@echo removing executable file from ${DESTDIR}/bin
	@rm -f ${DESTDIR}/bin/${NAME}
