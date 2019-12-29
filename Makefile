CC = gcc
CFLAGS = -g3 -Wall -Wextra -Wconversion -Wcast-qual -Wcast-align -g
CFLAGS += -Winline -Wfloat-equal -Wnested-externs
CFLAGS += -pedantic -std=gnu99 -Werror -D_GNU_SOURCE
PROMPT = -DPROMPT
EXECS = 33sh 33noprompt

.PHONY: all clean

all: $(EXECS)

33sh: jobs.h jobs.c sh.c
	$(CC) $(CFLAGS) jobs.c sh.c -o $@ $(PROMPT)

33noprompt: jobs.h jobs.c sh.c
	$(CC) $(CFLAGS) jobs.c sh.c -o $@

clean:
	rm -f $(EXECS)
