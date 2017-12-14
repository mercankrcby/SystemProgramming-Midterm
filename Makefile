SERVER_SRC=server.c
CLIENT_SRC=client.c
SHOWRES_SRC=showRes.c
SERVER_OUT=timeServer
CLIENT_OUT=seeWhat
SHOWRES_OUT=showResults

MAIN_FIFO_NAME=mainFifo

CC=gcc
FLAGS=-pedantic-errors -o

all: clean compile_server compile_client compile_showres


compile_server:
	${CC} ${SERVER_SRC} -lm ${FLAGS}  ${SERVER_OUT}

compile_client:
	${CC} ${CLIENT_SRC} -lm ${FLAGS} ${CLIENT_OUT}

compile_showres:
	${CC} ${SHOWRES_SRC} -lm ${FLAGS} ${SHOWRES_OUT}

clean:
	rm -rf ${SERVER_OUT} ${SHOWRES_OUT} ${CLIENT_OUT} ${MAIN_FIFO_NAME}
