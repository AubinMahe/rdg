LIB_NAME := rdg

CFLAGS := -pthread -std=c11 -I inc -I lib -fvisibility=hidden -D_DEFAULT_SOURCE=__STRICT_ANSI__ -D_FORTIFY_SOURCE=2 -fPIC\
 -pedantic -pedantic-errors -Wall -Wextra -Werror -Wconversion -Wcast-align -Wcast-qual -Wdisabled-optimization -Wlogical-op\
 -Wmissing-declarations -Wmissing-include-dirs -Wredundant-decls -Wshadow -Wsign-conversion -Wswitch-default -Wundef\
 -Wwrite-strings -Wfloat-equal -fmessage-length=0

SRCS     := src/rdg.c
SRCS_TST := test/main.c

OBJS         := $(SRCS:%c=BUILD/%o)
OBJS_DBG     := $(SRCS:%c=BUILD/DEBUG/%o)
OBJS_DBG_TST := $(SRCS_TST:%c=BUILD/DEBUG/%o)
DEPS         := $(SRCS:%c=BUILD/%d)

VALGRIND_COMMON_OPTIONS :=\
# --track-fds=yes
 --error-exitcode=2

VALGRIND_MEMCHECK_OPTIONS := $(VALGRIND_COMMON_OPTIONS)\
 --leak-check=full\
 --show-leak-kinds=all\
 --track-origins=yes\
 --expensive-definedness-checks=yes

VALGRIND_HELGRIND_OPTIONS := $(VALGRIND_COMMON_OPTIONS)\
 --free-is-write=yes

LIBS_DIR := /home/aubin/Dev/Java/2021/hpms.app.rdg/lib

.PHONY: all validate memcheck helgrind clean

all: lib$(LIB_NAME).so lib$(LIB_NAME)-d.so tests-d

validate: tests-d
	@g++ test/cplusplus_ckeck.cpp -c -I inc -I lib -W -Wall -pedantic
	@rm cplusplus_ckeck.o
	LD_LIBRARY_PATH=.:../rkv:../utils ./tests-d

memcheck: tests-d
	LD_LIBRARY_PATH=.:../rkv:../utils valgrind --tool=memcheck $(VALGRIND_MEMCHECK_OPTIONS) ./tests-d

helgrind: tests-d
	LD_LIBRARY_PATH=.:../rkv:../utils valgrind --tool=helgrind $(VALGRIND_HELGRIND_OPTIONS) ./tests-d

clean:
	rm -fr BUILD bin depcache build Debug Release

lib$(LIB_NAME).so: $(OBJS)
	gcc $^ -shared -o $@ -L $(LIBS_DIR) -lrkv -lutils
	strip --discard-all --discard-locals $@

lib$(LIB_NAME)-d.so: $(OBJS_DBG)
	gcc $^ -shared -o $@ -L $(LIBS_DIR) -lrkv-d -lutils-d

tests-d: $(OBJS_DBG_TST) lib$(LIB_NAME)-d.so
	gcc $(OBJS_DBG_TST) -o $@ -pthread -L. -l$(LIB_NAME)-d -L. -lrdg-d -L../rkv -lrkv-d -L../utils -lutils-d

BUILD/%.o: %.c
	@mkdir -p $$(dirname $@)
	gcc $(CFLAGS) -O3 -g0 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT $@ -o $@ $<

BUILD/DEBUG/%.o: %.c
	@mkdir -p $$(dirname $@)
	gcc $(CFLAGS) -O0 -g3 -c -MMD -MP -MF"$(@:%.o=%.d)" -MT $@ -o $@ $<

-include $(DEPS)
