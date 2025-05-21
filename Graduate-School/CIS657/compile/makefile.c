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
CC = gcc
CFLAGS = -Wall -Werror -I../include

# Target binary
TARGET = xinu

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean

# If you have additional directories or sources, add them above as needed.
# You can add more include paths with -I flags in CFLAGS if required.