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
	$(CC) -g -march=native -o $@ $^ -L$(LDIR) -ffast-math -fopenmp -lchess -lbenBrain -lbenMat -lOpenCL

opt: $(OBJ_OPT)
	$(CC) -O3 -march=native -o $@ $^ -L$(LDIR) -ffast-math -fopenmp -lchessOpt -lbenBrainOpt -lbenMat -lOpenCL

.PHONY: all
all:
	make main
	make opt

.PHONY: clean
clean:
	rm $(OBJ) $(OBJ_OPT) main opt
