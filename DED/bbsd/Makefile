
PROG= t
CFILES= main.C threads.C clients.C messages.C supervisor.C acceptor.C client.C
HFILES= main.H threads.H

.include <Makefile.inc>

X= xterm +sb -title NoTitle 
CMD= telnet 0 2700

test:
	$(X) -geometry 40x10+0+0     -e $(CMD) &
	$(X) -geometry 40x10+0+128   -e $(CMD) &
	$(X) -geometry 40x10+0+256   -e $(CMD) &
	$(X) -geometry 40x10+0+384   -e $(CMD) &
	$(X) -geometry 40x10+0+512   -e $(CMD) &
	$(X) -geometry 40x10+0+640   -e $(CMD) &
	$(X) -geometry 40x10+248+0   -e $(CMD) &
	$(X) -geometry 40x10+248+128 -e $(CMD) &
	$(X) -geometry 40x10+248+256 -e $(CMD) &
	$(X) -geometry 40x10+248+384 -e $(CMD) &
	$(X) -geometry 40x10+248+512 -e $(CMD) &
	$(X) -geometry 40x10+248+640 -e $(CMD) &
	$(X) -geometry 40x10+496+0   -e $(CMD) &
	$(X) -geometry 40x10+496+128 -e $(CMD) &
	$(X) -geometry 40x10+496+256 -e $(CMD) &
	$(X) -geometry 40x10+496+384 -e $(CMD) &
	$(X) -geometry 40x10+496+512 -e $(CMD) &
	$(X) -geometry 40x10+496+640 -e $(CMD) &

