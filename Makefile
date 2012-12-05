#Parametri richiesti dalle specifiche
GCCFLAGS= -ansi -pedantic -Wall -Wunused

#COLORI
GREEN=\033[32m
NORMAL=\033[00m

all: utils.o proxysender.exe proxyreceiver.exe

#CREAZIONE FILES .exe
proxysender.exe: proxysender.o
				 @gcc -o proxysender.exe proxysender.o utils.o
				 @echo "\n $(GREEN)[PROXY SENDER OK]$(NORMAL)"

proxyreceiver.exe: proxyreceiver.o
					@gcc -o proxyreceiver.exe proxyreceiver.o utils.o
					@echo "\n $(GREEN)[PROXY RECEIVER OK]$(NORMAL)"

#CREAZIONE FILE OGGETTO
proxyreceiver.o: proxyreceiver.c
			@gcc $(GCCFLAGS) -o proxyreceiver.o -c proxyreceiver.c

proxysender.o: proxysender.c
			   @gcc $(GCCFLAGS) -o proxysender.o -c proxysender.c

utils.o: utils.c
		@gcc $(GCCFLAGS) -o utils.o -c utils.c
		@echo "\n $(GREEN)[UTILS OK]$(NORMAL)"

# ELIMINAZIONE DEI FILE
clean:
	@rm -f core* *.stackdump
	@rm -f *.exe
	@rm -f *.o
	@echo "CLEAN...      $(GREEN)[OK]$(NORMAL)"
