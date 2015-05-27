.SUFFIXES:
.SUFFIXES: .c .o
CLNT = client
SRVR = file_op_svc
CFLAGS = -g -Wall

SRVR_OBJ = file_op_svc_proc.o file_op_xdr.o file_op_svc.o
CLNT_OBJ = client.o file_op_xdr.o file_op_clnt.o

.c.o:; gcc -c -o $@ $(CFLAGS) $<

default: $(CLNT) $(SRVR)

$(CLNT): $(CLNT_OBJ) file_op.h
	gcc -o $(CLNT) $(CLNT_OBJ)

$(SRVR): $(SRVR_OBJ) file_op.h
	gcc -o $(SRVR) $(SRVR_OBJ)

clean:
	rm *.o $(CLNT) $(SRVR)
	rm -i *~
	rm core
