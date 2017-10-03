
PROG= tcpgate
OBJS= tcpgate.o tcpgate_fast.o AcceptorThread.o DataThread.o \
	ConfigThread.o inet_aton.o

LIBS= ../threads/libthreads.a
INCS= -I../threads/h
DEFS= 
OPTS= -g3

all: depend
	make $(PROG)

tcpgate_all.o: depend $(OBJS)
	ld -x -r $(OBJS) -o tcpgate_all.o

$(PROG): $(OBJS) main.o
	g++ -o $(PROG) $(OBJS) main.o $(LIBS)

.C.o:
	gcc $(INCS) $(DEFS) $(OPTS) -c $*.C

inet_aton.o: ../inet_aton.c
	gcc $(INCS) $(DEFS) $(OPTS) -c  ../inet_aton.c

clean:
	rm -f $(PROG) $(PROG).exe *.o *~

depend:
	awk 'BEGIN { copy=1; } { if ( copy == 1 ) print } /^# DEPENDENCIES/ { copy=0; }' Makefile > x
	echo '' >> x
	gcc $(INCS) -M *.C *.c >> x
	mv x Makefile

# DEPENDENCIES

AcceptorThread.o: AcceptorThread.C ../threads/h/threads.H \
  /usr/include/setjmp.h /usr/include/sys/cdefs.h \
  /usr/include/machine/setjmp.h /usr/include/sys/types.h \
  /usr/include/machine/endian.h /usr/include/sys/_types.h \
  /usr/include/machine/_types.h /usr/include/sys/select.h \
  /usr/include/sys/_sigset.h /usr/include/sys/timespec.h \
  /usr/include/sys/time.h /usr/include/time.h /usr/include/unistd.h \
  /usr/include/sys/unistd.h /usr/include/stdarg.h /usr/include/stdlib.h \
  /usr/include/stdio.h ../threads/h/types.H ../threads/h/encrypt_iface.H \
  ../threads/h/magic.H ../threads/h/threads_messages.H \
  /usr/include/string.h /usr/include/strings.h \
  ../threads/h/threads_timers.H ../threads/h/threads_semaphores.H \
  ../threads/h/threads_class.H ../threads/h/threads_messages_tcp.H \
  /usr/include/netinet/in.h /usr/include/netinet6/in6.h \
  /usr/include/netdb.h AcceptorThread.H ConfigThread.H DataThread.H \
  /usr/include/sys/socket.h /usr/include/machine/param.h \
  /usr/include/errno.h
ConfigThread.o: ConfigThread.C ../threads/h/threads.H \
  /usr/include/setjmp.h /usr/include/sys/cdefs.h \
  /usr/include/machine/setjmp.h /usr/include/sys/types.h \
  /usr/include/machine/endian.h /usr/include/sys/_types.h \
  /usr/include/machine/_types.h /usr/include/sys/select.h \
  /usr/include/sys/_sigset.h /usr/include/sys/timespec.h \
  /usr/include/sys/time.h /usr/include/time.h /usr/include/unistd.h \
  /usr/include/sys/unistd.h /usr/include/stdarg.h /usr/include/stdlib.h \
  /usr/include/stdio.h ../threads/h/types.H ../threads/h/encrypt_iface.H \
  ../threads/h/magic.H ../threads/h/threads_messages.H \
  /usr/include/string.h /usr/include/strings.h \
  ../threads/h/threads_timers.H ../threads/h/threads_semaphores.H \
  ../threads/h/threads_class.H ../threads/h/threads_messages_tcp.H \
  /usr/include/netinet/in.h /usr/include/netinet6/in6.h \
  /usr/include/netdb.h AcceptorThread.H ConfigThread.H DataThread.H \
  /usr/include/sys/socket.h /usr/include/machine/param.h \
  /usr/include/errno.h /usr/include/fcntl.h
DataThread.o: DataThread.C ../threads/h/threads.H /usr/include/setjmp.h \
  /usr/include/sys/cdefs.h /usr/include/machine/setjmp.h \
  /usr/include/sys/types.h /usr/include/machine/endian.h \
  /usr/include/sys/_types.h /usr/include/machine/_types.h \
  /usr/include/sys/select.h /usr/include/sys/_sigset.h \
  /usr/include/sys/timespec.h /usr/include/sys/time.h /usr/include/time.h \
  /usr/include/unistd.h /usr/include/sys/unistd.h /usr/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/stdio.h ../threads/h/types.H \
  ../threads/h/encrypt_iface.H ../threads/h/magic.H \
  ../threads/h/threads_messages.H /usr/include/string.h \
  /usr/include/strings.h ../threads/h/threads_timers.H \
  ../threads/h/threads_semaphores.H ../threads/h/threads_class.H \
  ../threads/h/threads_messages_tcp.H /usr/include/netinet/in.h \
  /usr/include/netinet6/in6.h /usr/include/netdb.h AcceptorThread.H \
  ConfigThread.H DataThread.H /usr/include/sys/socket.h \
  /usr/include/machine/param.h /usr/include/errno.h /usr/include/fcntl.h
tcpgate.o: tcpgate.C ../threads/h/threads.H /usr/include/setjmp.h \
  /usr/include/sys/cdefs.h /usr/include/machine/setjmp.h \
  /usr/include/sys/types.h /usr/include/machine/endian.h \
  /usr/include/sys/_types.h /usr/include/machine/_types.h \
  /usr/include/sys/select.h /usr/include/sys/_sigset.h \
  /usr/include/sys/timespec.h /usr/include/sys/time.h /usr/include/time.h \
  /usr/include/unistd.h /usr/include/sys/unistd.h /usr/include/stdarg.h \
  /usr/include/stdlib.h /usr/include/stdio.h ../threads/h/types.H \
  ../threads/h/encrypt_iface.H ../threads/h/magic.H \
  ../threads/h/threads_messages.H /usr/include/string.h \
  /usr/include/strings.h ../threads/h/threads_timers.H \
  ../threads/h/threads_semaphores.H ../threads/h/threads_class.H \
  ../threads/h/threads_messages_tcp.H /usr/include/netinet/in.h \
  /usr/include/netinet6/in6.h /usr/include/netdb.h AcceptorThread.H \
  ConfigThread.H DataThread.H
tcpgate_fast.o: tcpgate_fast.C /usr/include/sys/types.h \
  /usr/include/sys/cdefs.h /usr/include/machine/endian.h \
  /usr/include/sys/_types.h /usr/include/machine/_types.h \
  /usr/include/sys/select.h /usr/include/sys/_sigset.h \
  /usr/include/sys/timespec.h /usr/include/sys/socket.h \
  /usr/include/machine/param.h /usr/include/netinet/in.h \
  /usr/include/netinet6/in6.h /usr/include/arpa/inet.h \
  /usr/include/sys/uio.h /usr/include/unistd.h /usr/include/sys/unistd.h \
  /usr/include/string.h /usr/include/strings.h /usr/include/stdlib.h \
  /usr/include/netdb.h /usr/include/stdio.h /usr/include/errno.h \
  /usr/include/fcntl.h
main.o: main.c
