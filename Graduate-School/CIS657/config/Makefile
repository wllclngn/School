# Core source files for Xinu kernel
SYSTEM_SRCS = \
	system/main.c

# Shell command source files
SHELL_SRCS = \
	shell/xsh_create.c \
	shell/xsh_createsleep.c \
	shell/xsh_psready.c \
	shell/xsh_wait.c \
	shell/xsh_signaln.c \
	shell/xsh_resumen.c

# Standard Xinu object files (add others as necessary)
SYSTEM_OBJS = $(SYSTEM_SRCS:.c=.o)
SHELL_OBJS  = $(SHELL_SRCS:.c=.o)

# All object files
OBJS = $(SYSTEM_OBJS) $(SHELL_OBJS)

# Compiler and flags
CC      = gcc
LEX     = flex
YACC    = bison -y    # Flag enables yacc-compatible filenames
CFLAGS  = -Wall -Werror -I../include
LFLAGS  = -lfl

# Target binaries
TARGET  = xinu
CONFIG  = config

all: $(TARGET) conf.h conf.c

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Configuration generation rules
${CONFIG}: lex.yy.c y.tab.c
	$(CC) ${CFLAGS} -o $@ y.tab.c ${LFLAGS}

lex.yy.c: config.l
	$(LEX) config.l

y.tab.c: config.y
	$(YACC) config.y

conf.h: $(CONFIG) Configuration
	./$(CONFIG)

conf.c: $(CONFIG) Configuration
	./$(CONFIG)

install: conf.h conf.c
	cp conf.h ../include
	cp conf.c ../system

clean:
	rm -f $(OBJS) $(TARGET) ${CONFIG} lex.yy.c y.tab.c

.PHONY: all clean install