SRCS = src/main.c \
       src/lib/list/list.c \
       src/lib/list/list_types.c \
       src/lib/hash/hash.c \
       src/lib/string/string.c \
       src/lib/types.c\
       src/compiler/lexer/lexer.c

all: clean test run

test: $(SRCS)
	@gcc $(SRCS) -I src -o test

run:
	@./test

clean:
	@rm -f test
