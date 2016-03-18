CC           = gcc
TARGET       = spreedbox-fpgaprog
SOURCES      = $(wildcard *.c)
OBJECTS      = $(patsubst %.c,%.o,$(SOURCES))
CFLAGS		 = -O2 -g -Wall -Wextra -Werror
LDFLAGS      = -lwiringPi -lpthread -g -Wall -Wextra -Werror

PREFIX  = $(DESTDIR)/usr
BINDIR  = $(PREFIX)/bin
SBINDIR = $(PREFIX)/sbin

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -g -O0 -o $(TARGET) $(OBJECTS) $(LDFLAGS)

install: $(TARGET)
	install -D -m 755 $(TARGET) $(SBINDIR)/$(TARGET)

clean:
	rm -f $(TARGET) $(OBJECTS)

distclean: clean
