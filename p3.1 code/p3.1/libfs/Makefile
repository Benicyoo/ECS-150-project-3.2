lib := libfs.a

CC := gcc
CFLAGS := -Wall -Wextra -Werror -std=c11 -g
AR := ar
ARFLAGS := rcs

objs := disk.o fs.o

all: $(lib)

$(lib): $(objs)
	$(AR) $(ARFLAGS) $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	rm -f $(objs) $(lib)

