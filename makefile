CC=icc
CFLAGS= -openmp -ipo
CFLAGS= -openmp -O3 -no-prec-div -ip -xAVX
LFLAGS=-L.
LFLAGS=-L. -openmp  -O3
OBJDIR=obj
OBJECTS= $(OBJDIR)/LinkedList.o $(OBJDIR)/Sector.o $(OBJDIR)/AuxFunc.o $(OBJDIR)/main.o $(OBJDIR)/earth.o $(OBJDIR)/lodepng.o 
OBJECTS_P=$(OBJDIR)/LinkedList.o $(OBJDIR)/Sector.o $(OBJDIR)/precomp.o

all: compute precompute

compute: $(OBJECTS) makefile
	$(CC) $(LFLAGS) -o comp.out $(OBJECTS)

precompute: $(OBJECTS_P) makefile
	$(CC) $(LFLAGS) -o precomp.out $(OBJECTS_P)
	
$(OBJDIR)/Sector.o: Sector.cpp makefile
	$(CC) -c $(CFLAGS) -o $@ $<
	
$(OBJDIR)/precomp.o: precomp.cpp makefile Defs.h AuxFunc.h
	$(CC) -c $(CFLAGS) -o $@ $< 
	
$(OBJDIR)/main.o: main.cpp makefile  Defs.h AuxFunc.h
	$(CC) -c $(CFLAGS) -o $@ $<

$(OBJDIR)/AuxFunc.o: AuxFunc.cpp makefile
	$(CC) -c $(CFLAGS) -o $@ $<

$(OBJDIR)/LinkedList.o: LinkedList.cpp makefile
	$(CC) -c $(CFLAGS) -o $@ $<

$(OBJDIR)/%.o: %.c makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: %.cpp makefile
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

clean:	cleanx cleanp

cleanx:
	rm -rf $(OBJDIR)/*.o *.out

cleanp:
	rm -rf *.out


