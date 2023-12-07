all: motor jogoUI bot

motor: motor.o communication.o cursesHelpers.o
	gcc motor.o communication.o cursesHelpers.o -pthread -lncurses -o motor

jogoUI: jogoUI.o communication.o cursesHelpers.o
	gcc jogoUI.o communication.o cursesHelpers.o -pthread -lncurses -o jogoUI

bot: bot.o
	gcc bot.o -o bot

motor.o: ./src/motor.c ./src/communication.h ./src/commons.h ./src/cursesHelpers.h
	gcc -c ./src/motor.c

jogoUI.o: ./src/jogoUI.c ./src/communication.h ./src/commons.h ./src/cursesHelpers.h
	gcc -c ./src/jogoUI.c

bot.o: ./src/bot.c
	gcc -c ./src/bot.c

communication.o: ./src/communication.c ./src/communication.h ./src/commons.h
	gcc -c ./src/communication.c

cursesHelpers.o: ./src/cursesHelpers.c ./src/cursesHelpers.h ./src/commons.h
	gcc -c ./src/cursesHelpers.c

clean:
	rm *.o motor jogoUI