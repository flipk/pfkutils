#!/bin/sh

touch aclocal.m4
sleep 1
touch configure 
touch config.guess config.sub depcomp install-sh ltmain.sh missing
touch ylwrap ar-lib INSTALL
sleep 1
touch pfkutils_config.h.in
touch `find . -name Makefile.in -print`

exit 0
