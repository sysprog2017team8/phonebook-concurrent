CC ?= gcc
CFLAGS_common ?= -Wall -std=gnu99
CFLAGS_orig = -O0
CFLAGS_opt  = -O0 -pthread -g -pg
CFLAGS_lockfree  = -O0 -pthread -g -pg

ifdef CHECK_LEAK
CFLAGS_common += -fsanitize=address -fno-omit-frame-pointer
endif

ifdef THREAD
CFLAGS_opt  += -D THREAD_NUM=${THREAD}
CFLAGS_lockfree  += -D THREAD_NUM=${THREAD}
endif

ifeq ($(strip $(DEBUG)),1)
CFLAGS_opt += -DDEBUG -g
endif

EXEC = phonebook_orig phonebook_opt phonebook_lock phonebook_lockfree
GIT_HOOKS := .git/hooks/applied
.PHONY: all
all: $(GIT_HOOKS) $(EXEC)

$(GIT_HOOKS):
	@scripts/install-git-hooks
	@echo

SRCS_common = main.c

tools/text_align: text_align.c tools/tool-text_align.c
	$(CC) $(CFLAGS_common) $^ -o $@

phonebook_orig: $(SRCS_common) phonebook_orig.c phonebook_orig.h
	$(CC) $(CFLAGS_common) $(CFLAGS_orig) \
		-DIMPL="\"$@.h\"" -o $@ \
		$(SRCS_common) $@.c

phonebook_opt: $(SRCS_common) phonebook_opt.c phonebook_opt.h text_align.c
	$(CC) $(CFLAGS_common) $(CFLAGS_opt) \
		-DIMPL="\"$@.h\"" -o $@ \
		$(SRCS_common) $@.c text_align.c

phonebook_lock: $(SRCS_common) phonebook_lock.c phonebook_lock.h text_align.c
	$(CC) $(CFLAGS_common) $(CFLAGS_lockfree) \
		-DIMPL="\"$@.h\"" -o $@ \
		$(SRCS_common) $@.c text_align.c

phonebook_lockfree: $(SRCS_common) phonebook_lockfree.c phonebook_lockfree.h text_align.c
	$(CC) $(CFLAGS_common) $(CFLAGS_lockfree) \
		-DIMPL="\"$@.h\"" -o $@ \
		$(SRCS_common) $@.c text_align.c

run: $(EXEC)
	echo 3 | sudo tee /proc/sys/vm/drop_caches
	watch -d -t "./phonebook_orig && echo 3 | sudo tee /proc/sys/vm/drop_caches"

cache-test: $(EXEC)
	rm -f opt.txt
	rm -f lock.txt
	rm -f lockfree.txt
	perf stat --repeat 100 \
		-e cache-misses,cache-references,instructions,cycles \
		./phonebook_opt
	perf stat --repeat 100 \
		-e cache-misses,cache-references,instructions,cycles \
		./phonebook_lock
	perf stat --repeat 100 \
		-e cache-misses,cache-references,instructions,cycles \
		./phonebook_lockfree

perf-set:
	sudo sh -c " echo 0 > /proc/sys/kernel/perf_event_paranoid"
	sudo sh -c " echo 0 > /proc/sys/kernel/kptr_restrict"

output.txt: cache-test calculate
	./calculate "create() appendByFile() findName() removeByFile() free()" opt.txt lock.txt lockfree.txt $@

plot: output.txt
	gnuplot scripts/runtime.gp

plot-cll: output.txt
	gnuplot scripts/cllruntime.gp

calculate: calculate.c
	$(CC) $(CFLAGS_common) $^ -o $@

checkstyle:
	astyle --style=kr --indent=spaces=4 --indent-switches --suffix=none *.[ch]

.PHONY: clean
clean:
	$(RM) $(EXEC) *.o perf.* \
	      	calculate orig.txt opt.txt output.txt runtime.png align.txt
