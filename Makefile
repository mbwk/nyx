include config.mk

all: $(EXE)
.PHONY: all

%.c.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(EXE): $(OBJS)
	$(LD) -o $@ $^ $(LDFLAGS) $(LIBS)

lint:
	$(LINTER) $(SRCS)

clean: 
	rm $(BUILT)
.PHONY: clean
