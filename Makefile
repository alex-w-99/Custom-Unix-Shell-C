# First target is default target, just type "make"

FILE=shell.c
CC=gcc
CFLAGS=-g -O0

default: run

all: run

run: myshell
        ./myshell

myshell: ${FILE}
        ${CC} ${CFLAGS} -o myshell ${FILE}

build: myshell
        make $<

clean:
        rm -f myshell
