CC=g++
IDIR=..
LDIR=../.OBJECTS
DEPS = abbie.hpp $(IDIR)/chess/chess.hpp 
ODIR=.obj

_OBJ = main.o abbie.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))
OBJ_OPT = $(patsubst %,$(ODIR)/opt%,$(_OBJ))

$(ODIR)/%.o: %.cpp $(DEPS)
	$(CC) -c -g -o $@ $< -I$(IDIR)/ 

$(ODIR)/opt%.o: %.cpp $(DEPS)
	$(CC) -c -O3 -o $@ $< -I$(IDIR)/

main: $(OBJ)
	$(CC) -g -o $@ $^ -L$(LDIR) -lchess -lbenMat -lbenBrain

opt: $(OBJ_OPT)
	$(CC) -O3 -o $@ $^ -L$(LDIR) -lchessOpt -lbenMatOpt -lbenBrainOpt

.PHONY: all
all:
	make main
	make opt

.PHONY: clean
clean:
	rm $(OBJ) $(OBJ_OPT) main opt
