

#src/osipparser2/osip_message_to_str.c

COBJECTS:=sip-call.o
CFLAGS:=-I/home/yu/code/libosip2-3.6.0/out-bin/include
LFLAGS:=-L/home/yu/code/libosip2-3.6.0/out-bin/lib 
LD:=-losipparser2 -losip2

sip-call:	${COBJECTS}
	cc -o $@ $< ${CFLAGS} ${LFLAGS} ${LD}
clean:
	@rm *.o sip-call -rf
