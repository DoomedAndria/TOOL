SRCS = src/main.c \
       src/list/list.c \
       src/list/list_types.c \
       src/hash/hash.c \
       src/string/string.c \
       src/types.c

all: clean test run

test: $(SRCS)
	@gcc $(SRCS) -I src -o test

run:
	@./test

clean:
	@rm -f test
