run : shell.o circularDLL.o
	cc shell.o circularDLL.o -o shell
good5 : shell.c  circularDLL.h
	cc -c shell.c 
DLL : circularDLL.c circularDLL.h 
	cc -c circularDLL.c
clean :
	rm *.o