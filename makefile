all: motor jogoUI

motor: motor.o communication.o
	gcc motor.o communication.o -pthread -o motor

jogoUI: jogoUI.o communication.o
	gcc jogoUI.o communication.o -o jogoUI

motor.o: ./src/motor.c ./src/communication.h ./src/commons.h
	gcc -c ./src/motor.c

jogoUI.o: ./src/jogoUI.c ./src/communication.h ./src/commons.h
	gcc -c ./src/jogoUI.c

communication.o: ./src/communication.c ./src/communication.h ./src/commons.h
	gcc -c ./src/communication.c

clean:
	rm *.o motor jogoUI