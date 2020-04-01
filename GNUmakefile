# Copyright (c) 2020 Alex Caudill <alex.caudill@pm.me>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

CC ?= gcc
LD ?= ld
AR ?= ar
RM ?= rm
CFLAGS ?= -O2 -std=gnu99

SRCDIR := $(realpath .)
OBJDIR ?= $(SRCDIR)/obj

UNAME_S := $(shell uname -s)

define generateRule
$2 += $(patsubst %.c, %.o, $(subst $(SRCDIR), $(OBJDIR), ${1}))
$3 += $(patsubst %.c, %.d, $(subst $(SRCDIR), $(OBJDIR), ${1}))
$(patsubst %.c, %.o, $(subst $(SRCDIR), $(OBJDIR), ${1})): $(1) | objdir
	$$(CC) -c -fPIC $$(CORE_CFLAGS) $$(filter-out %.h,$$^) -o $$@ -MQ $$@ -MMD -MP -MF$(patsubst %.c, %.d, $(subst $(SRCDIR), $(OBJDIR), ${1})) 
endef

all: $(OBJDIR)/build/libtuv.a

objdir:
	find "$(SRCDIR)" -type d | sed -e "s?$(SRCDIR)?$(OBJDIR)?" | xargs mkdir -p
	mkdir -p $(OBJDIR)/build

CORE_CFLAGS = $(CFLAGS)                            \
  					-Wall                                  \
						-I$(SRCDIR)/include \
						-I$(SRCDIR)/src \
						-I$(SRCDIR)/src/unix \
						-D_POSIX_C_SOURCE=200809L              \
						-D_XOPEN_SOURCE=700                    \
						-fomit-frame-pointer                   \
						-fstack-protector                      \
						-flto                                  \
						-fno-asynchronous-unwind-tables        \
						-fno-prefetch-loop-arrays              \
						-freorder-blocks-algorithm=simple      \
						-mno-align-stringops                   \
						-fno-align-loops                       \
						-fno-align-labels                      \
						-fno-align-jumps                       \
						-ffunction-sections

LIBTUV_SRCS = $(SRCDIR)/src/fs-poll.c      \
							$(SRCDIR)/src/inet.c         \
							$(SRCDIR)/src/version.c      \
							$(SRCDIR)/src/threadpool.c   \
							$(SRCDIR)/src/uv-common.c    \
							$(SRCDIR)/src/tuv_debuglog.c

ifeq ($(UNAME_S),Linux)
LIBTUV_SRCS += $(SRCDIR)/src/unix/async.c          \
							 $(SRCDIR)/src/unix/dl.c             \
							 $(SRCDIR)/src/unix/fs.c             \
							 $(SRCDIR)/src/unix/getaddrinfo.c    \
							 $(SRCDIR)/src/unix/getnameinfo.c    \
							 $(SRCDIR)/src/unix/loop-watcher.c   \
							 $(SRCDIR)/src/unix/loop.c           \
							 $(SRCDIR)/src/unix/pipe.c           \
							 $(SRCDIR)/src/unix/process.c        \
							 $(SRCDIR)/src/unix/signal.c         \
							 $(SRCDIR)/src/unix/stream.c         \
							 $(SRCDIR)/src/unix/tcp.c            \
							 $(SRCDIR)/src/unix/thread.c         \
							 $(SRCDIR)/src/unix/timer.c          \
							 $(SRCDIR)/src/unix/tty.c            \
							 $(SRCDIR)/src/unix/udp.c            \
							 $(SRCDIR)/src/unix/proctitle.c      \
							 $(SRCDIR)/src/unix/linux.c     \
							 $(SRCDIR)/src/unix/core.c           \
							 $(SRCDIR)/src/unix/poll.c
endif

LIBTUV_OBJS =
LIBTUV_DEPS =

$(foreach file,$(LIBTUV_SRCS),$(eval $(call generateRule,$(file),LIBTUV_OBJS,LIBTUV_DEPS)))

$(OBJDIR)/build/libtuv.a: $(LIBTUV_OBJS)
	ar crs $@ $^
	ranlib $@

clean::
	rm -f $(LIBTUV_OBJS) $(LIBTUV_DEPS)
