# Makefile for the Kstate kernel module

# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is the Kynesim state management library.
#
# The Initial Developer of the Original Code is Kynesim, Cambridge UK.
# Portions created by the Initial Developer are Copyright (C) 2013
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Kynesim, Cambridge UK
#   gb <gb@kynesim.co.uk>
#
# ***** END LICENSE BLOCK *****

ifdef CROSS_COMPILE
CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)gcc		# because gcc knows where to find libc...
else
CC=gcc
LD=gcc
endif

# Note we assume a traditional Linux style environment in our flags
WARNING_FLAGS=-Wall -Werror
DEBUG_FLAGS=
LD_SHARED_FLAGS+=-shared
INCLUDE_FLAGS=-I .
CFLAGS+=-fPIC -g

ifeq ($(O),)
	TGTDIR=.
else
	TGTDIR=$(O)/libkstate
endif

SRCS=kstate.c
OBJS=$(SRCS:%.c=$(TGTDIR)/%.o)
DEPS=kstate.h

SHARED_NAME=libkstate.so
STATIC_NAME=libkstate.a
SHARED_TARGET=$(TGTDIR)/$(SHARED_NAME)
STATIC_TARGET=$(TGTDIR)/$(STATIC_NAME)

.PHONY: all
all: dirs  extract_header $(SHARED_TARGET) $(STATIC_TARGET) test

# Note that this is *not* out-of-tree build friendly
# On the other hand, it should not do anything if the headers are up-to-date
# XXX Consider whether to leave this in...
.PHONY: extract_header
extract_header:
	./extract_hdrs.py -kstate

.PHONY: install
install:
	-mkdir -p $(DESTDIR)/lib
	-mkdir -p $(DESTDIR)/include/kstate
	install -m 0644 kstate.h   $(DESTDIR)/include/kstate/kstate.h
	install -m 0755 $(SHARED_TARGET) $(DESTDIR)/lib/$(SHARED_NAME)
	install -m 0755 $(STATIC_TARGET) $(DESTDIR)/lib/$(STATIC_NAME)

.PHONY: dirs
dirs:
	-mkdir -p $(TGTDIR)

$(TGTDIR)/%.o: %.c
	$(CC) $(INCLUDE_FLAGS) $(CFLAGS) -o $@ $(WARNING_FLAGS) -c $^

$(TGTDIR)/%.o: $(DEPS)

$(SHARED_TARGET): $(OBJS)
	echo Objs = $(OBJS)
	$(LD) $(LD_SHARED_FLAGS) -o $(SHARED_TARGET) $(OBJS) -lc

$(STATIC_TARGET): $(STATIC_TARGET)($(OBJS))

TEST_PROG=$(TGTDIR)/check_kstate

test: $(TEST_PROG)
	$(TEST_PROG)

# We assume that 'check' has been installed, typically with
#   sudo apt-get install check
$(TEST_PROG): check_kstate.c $(STATIC_TARGET)
	$(CC) $(INCLUDE_FLAGS) $(CFLAGS) -o $@ $(WARNING_FLAGS) $^ -lcheck

.PHONY: clean
clean:
	rm -f $(TGTDIR)/*.o $(SHARED_TARGET) $(STATIC_TARGET)
	rm -f $(TEST_PROG)

.PHONY: distclean
distclean: clean
	rm -f *.bak
