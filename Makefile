# First target is default target, just type "make"

FILE=main.c
CC=gcc
CFLAGS=-std=gnu17 -g -O0

default: run

all: run

run: myshell
        ./myshell

myshell: shell.h shell.c ${FILE}
        ${CC} ${CFLAGS} shell.c ${FILE} -o myshell

build: myshell
        make $<

clean:
        rm -f myshell
