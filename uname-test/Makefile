CFLAGS += -Wall

all: check

test: check

check: uname-test
	for f in $(wildcard *.data) ; do \
		./uname-test $$f || exit 1 ; \
	done

clean:
	rm -f uname-test

.PHONY: all test check clean
