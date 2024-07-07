X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

VERSION = 1.0.0

INC = -I. -I/usr/include -I$(X11INC)
LIB = -L$(X11LIB) -lX11 -lXi -lXtst

CFLAGS = -Wall -Wextra -std=c99 -pedantic -DVERSION=\"$(VERSION)\" $(INC) -O3
LDFLAGS = $(LIB) -s

all: build

build: quickMacro.c
	$(CC) $(CFLAGS) $(LDFLAGS) quickMacro.c -o quickMacro
	chmod +x quickMacro

clean:
	rm quickMacro

install: build
	cp quickMacro /usr/bin/
	cp quickMacro.1 /usr/share/man/man1/quickMacro.1.gz
	sed -i /usr/share/man/man1/quickMacro.1.gz -e "s/VERSION/$(VERSION)/"

uninstall:
	rm /usr/bin/quickMacro
	rm /usr/share/man/man1/quickMacro.1.gz
