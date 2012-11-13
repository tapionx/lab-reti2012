#Macros
GCCFLAGS= -ansi -pedantic -Wall -Wunused

#OUTPUT COLOR
GREEN=\033[32m
NORMAL=\033[00m


all: proxysender.exe sender.exe proxyreceiver.exe receiver.exe

#CREAZIONE FILES .exe


proxysender.exe: proxysender.o utils.o
				 @gcc -o proxysender.exe ./proxysender.o ./utils.o
				 @echo "\n $(GREEN)[PROXY SENDER DONE]$(NORMAL)"

proxyreceiver.exe: proxyreceiver.o utils.o
					@gcc -o proxyreceiver.exe ./proxyreceiver.o ./utils.o
					@echo "\n $(GREEN)[PROXY RECEIVER DONE]$(NORMAL)"

sender.exe: sender.o utils.o
			@gcc -o sender.exe ./sender.o ./utils.o
			@echo "\n $(GREEN)[SENDER DONE]$(NORMAL)"

receiver.exe: receiver.o utils.o
				@gcc -o receiver.exe ./receiver.o ./utils.o
				@echo "\n $(GREEN)[RECEIVER DONE]$(NORMAL)"


#CREAZIONE FILE OGGETTO

proxyreceiver.o: proxyreceiver.c
			@gcc $(GCCFLAGS) -c proxyreceiver.c
			
receiver.o: receiver.c
			@gcc $(GCCFLAGS) -c receiver.c
			
sender.o: sender.c
		 @gcc $(GCCFLAGS) -c sender.c
			
proxysender.o: proxysender.c
			   @gcc $(GCCFLAGS) -c proxysender.c
			
utils.o: utils.c
		@gcc $(GCCFLAGS) -c utils.c
		
		

clean:	
	@rm -f core* *.stackdump
	@rm -f *.exe
	@rm -f *.o
	@echo "CLEAN...      $(GREEN)[OK]$(NORMAL)"
