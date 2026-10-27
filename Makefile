CC = gcc

CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Wconversion -g
CPPFLAGS = -MMD -MP

OBJDIR = build/obj
BINDIR = build/bin

TARGET = $(BINDIR)/batteryctl

SRCS = main.c battery.c sysfs.c fan.c output.c
OBJS = $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)

PREFIX = /usr/local

.PHONY: all clean install uninstall

all: $(TARGET)

$(TARGET): $(OBJS)
	mkdir -p $(BINDIR)
	$(CC) $(OBJS) -o $@

$(OBJDIR)/%.o: %.c
	mkdir -p $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -rf build

install: $(TARGET)
	install -Dm755 $(TARGET) $(PREFIX)/bin/batteryctl

uninstall:
	rm -f $(PREFIX)/bin/batteryctl

TEST_CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -g -O0 -fsanitize=address,undefined

.PHONY: test clean-tests

tests/test_sysfs: tests/test_sysfs.c sysfs.c sysfs.h
	$(CC) $(TEST_CFLAGS) -I. tests/test_sysfs.c sysfs.c -o $@

test: tests/test_sysfs
	./tests/test_sysfs

clean-tests:
	rm -f tests/test_sysfs
