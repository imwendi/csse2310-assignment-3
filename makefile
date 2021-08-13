CC = gcc
CFLAGS = -Wall -pedantic --std=gnu99 -g
CLIENT_OBJS = client.o genericClient.o clientData.o lineList.o commands.o\
	      clientbotUtils.o
CLIENTBOT_OBJS = clientbot.o genericClient.o clientData.o lineList.o\
		 commands.o clientbotUtils.o
SERVER_OBJS = server.o lineList.o commands.o serverUtils.o
.PHONY: all clean
.DEFAULT_GOAL := all

all : client clientbot server

clean :
	rm client clientbot *.o

# Compile the client
client : $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile the client bot
clientbot : $(CLIENTBOT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile the server
server : $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Pattern rule for compiling .o objects given .c files
%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<

# Dependency rules
client.o: commands.h lineList.h clientData.h genericClient.h
clientData.o: clientData.h clientbotUtils.h lineList.h
genericClient.o : lineList.h clientData.h commands.h genericClient.h
commands.o: commands.h lineList.h
lineList.o : lineList.h

clientbot.o : lineList.h clientbotUtils.h commands.h genericClient.h 
clientbotUtils.o : lineList.h commands.h clientbotUtils.h

server.o : lineList.h commands.h serverUtils.h
serverUtils.o: serverUtils.h lineList.h commands.h
