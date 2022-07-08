export TOPDIR = $(shell pwd)

TARGET = abc
# DYNAMIC_LIB = pnlib
# STATIC_LIB = pnlib

# export OPT = -O2
# export EXTRA_OPT = -fno-inline

export Q_FLAG = 1

include $(TOPDIR)/Makefile.inc
