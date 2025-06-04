# Target library
lib := libfs.a
objs := fs.o disk.o
flags:= -Wall -Wextra -Werror -c -o
fs:= fs.c fs.h disk.h
d:= disk.c disk.h

all: $(lib)

## TODO
$(lib): $(objs)
	ar rc $@ $^

fs.o: $(fs)
	gcc $(flags) $@ $<
disk.o: $(d)
	gcc $(flags) $@ $<

clean:
	rm fs.o libfs.a disk.o