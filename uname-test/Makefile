CFLAGS += -Wall -Wextra -Werror

all: check

test: check

check: uname-test
	./uname-test *.data > curr.results
	diff -u curr.results good.results

clean:
	rm -f uname-test

.PHONY: all test check clean
