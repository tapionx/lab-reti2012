#!/bin/bash
echo
echo 'utils.c'
echo

gcc -ansi -pedantic -Wall -Wunused -I . -c utils.c

echo
echo 'sender.c'
echo

gcc -ansi -pedantic -Wall -Wunused -I . -c sender.c

echo 
echo 'proxysender.c'
echo

gcc -ansi -pedantic -Wall -Wunused -I . -c proxysender.c

echo 
echo 'proxyreceiver.c'
echo

gcc -ansi -pedantic -Wall -Wunused -I . -c proxyreceiver.c

echo 
echo 'receiver.c'
echo

gcc -ansi -pedantic -Wall -Wunused -I . -c receiver.c

gcc -ansi -pedantic -Wall -Wunused -o sender.exe sender.o utils.o
gcc -ansi -pedantic -Wall -Wunused -o proxysender.exe proxysender.o utils.o
gcc -ansi -pedantic -Wall -Wunused -o proxyreceiver.exe proxyreceiver.o utils.o
gcc -ansi -pedantic -Wall -Wunused -o receiver.exe receiver.o utils.o

