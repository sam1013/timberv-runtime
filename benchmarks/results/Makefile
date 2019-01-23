include ../../../config.common

ELF := $(wildcard *.elf)
BENCH := ${ELF:.elf=}
TEST := ${ELF:.elf=.test}

all: $(BENCH)
	@:

test: $(TEST)
	@:

% : %-short.hist
	@echo -n ""

# avoid deleting intermediate files
.SECONDARY:
# turn off error messages for this target, since spike always fails with 255
%.hist : %.elf
	@echo Running $^
	$(SPIKE) -D0 ${SPIKE_FLAGS} $^ 1> ${@:hist=out} 2> $@

%-short.hist : %.hist
	@echo Evaluating $^
	@awk -f hist.awk $^ > $@

%.test : %.elf
	-$(SPIKE) -D0 ${SPIKE_FLAGS} $^

clean:
	rm -f *.hist *.out *.log

cleanfull: clean
	rm -f *.elf
