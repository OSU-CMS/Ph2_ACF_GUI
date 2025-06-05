SHELL := /bin/bash

cli-tests := $(addsuffix .cli-test, $(basename $(wildcard *.test)))

.PHONY : test all %.test

test : $(cli-tests)

%.cli-test : %.test
	@python3 ../cli_test.py $<

all : test
	@echo "Success, all tests passed."
