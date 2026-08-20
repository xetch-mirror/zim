ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/..)

CC ?= cc
CPPFLAGS ?= -I$(ROOT)/include
CFLAGS ?= -std=c99 -Wall -Wextra -O2
LDFLAGS ?=
LDLIBS ?=

PROGRAM ?= $(ROOT)/zim
SRC_SOURCES := $(wildcard $(ROOT)/src/*.c)
SYNTAX_SOURCES := $(wildcard $(ROOT)/syntax/*.c)
SRC_OBJECTS := $(SRC_SOURCES:$(ROOT)/src/%.c=$(ROOT)/build/%.o)
SYNTAX_OBJECTS := $(SYNTAX_SOURCES:$(ROOT)/syntax/%.c=$(ROOT)/build/syntax_%.o)
ALL_OBJECTS := $(SRC_OBJECTS) $(SYNTAX_OBJECTS)

.PHONY: all clean run

all: $(PROGRAM)

run: $(PROGRAM)
	$(PROGRAM)

$(PROGRAM): $(ALL_OBJECTS)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(ROOT)/build/%.o: $(ROOT)/src/%.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(ROOT)/build/syntax_%.o: $(ROOT)/syntax/%.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -rf $(ROOT)/build $(PROGRAM)

-include $(ALL_OBJECTS:.o=.d)