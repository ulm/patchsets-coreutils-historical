CFLAGS += -Wall

all: check

test: check

check: uname-test
	./uname-test *.data

clean:
	rm -f uname-test

.PHONY: all test check clean
