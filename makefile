all: motor jogoUI

motor: motor.o communication.o cursesHelpers.o
	gcc motor.o communication.o cursesHelpers.o -pthread -lncurses -o motor

jogoUI: jogoUI.o communication.o
	gcc jogoUI.o communication.o -lncurses -o jogoUI

motor.o: ./src/motor.c ./src/communication.h ./src/commons.h ./src/cursesHelpers.h
	gcc -c ./src/motor.c

jogoUI.o: ./src/jogoUI.c ./src/communication.h ./src/commons.h
	gcc -c ./src/jogoUI.c

communication.o: ./src/communication.c ./src/communication.h ./src/commons.h
	gcc -c ./src/communication.c

cursesHelpers.o: ./src/cursesHelpers.c ./src/cursesHelpers.h
	gcc -c ./src/cursesHelpers.c

clean:
	rm *.o motor jogoUI