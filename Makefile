CC=g++
LL=ar
LDIR=../.OBJECTS
IDIR=../
INC=-I$(IDIR)
ODIR=../.OBJECTS
OBJDIR=.obj
DEPS = abbie.hpp


.PHONY: main
main: $(ODIR)/libabbie.a

.PHONY: opt
opt: $(ODIR)/libabbieOpt.a

$(OBJDIR)/abbie.o: abbie.cpp $(DEPS)
	$(CC) -g -c $(INC) -o $@ $<

$(OBJDIR)/abbieOpt.o: abbie.cpp $(DEPS)
	$(CC) -O3 -c $(INC) -o $@ $<

$(ODIR)/libabbie.a: $(OBJDIR)/abbie.o
	$(LL) src $@ $^ 

$(ODIR)/libabbieOpt.a: $(OBJDIR)/abbieOpt.o
	$(LL) src $@ $^

.PHONY: all
all:
	make main
	make opt

clean:
	-rm -f $(OBJDIR)/* $(ODIR)/libabbie*
