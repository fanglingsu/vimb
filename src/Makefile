BASEDIR=..
include $(BASEDIR)/config.mk

OBJ = $(patsubst %.c, %.o, $(wildcard *.c))

all: vimb

clean:
	$(RM) vimb *.o

vimb: $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

$(OBJ): config.h $(BASEDIR)/config.mk

-include $(OBJ:.o=.d)

config.h:
	@echo create $@ from config.def.h
	@cp config.def.h $@

%.o: %.c
	@echo "${CC} $@"
	@$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: all clean
