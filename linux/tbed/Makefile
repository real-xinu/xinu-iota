CFLAGS= -c -Wall -std=gnu99
CC=gcc


SOURCE= ./src
TOOLS_P=./ConsoleTools/hdr/
TOOLS_C=./ConsoleTools/lib/*.c
HEADER=./include/headers.h
MACROS=./include/macros.h
GLOBAL=./include/global.h
BUILDDIR=build
EXE=tbed
SOURCES = $(wildcard $(SOURCE)/*.c)
OBJECTS = $(patsubst $(SOURCE)/%.c, build/%.o,$(SOURCES))
OBJECTS = $(patsubst $(SOURCE)/%.c,$(BUILDDIR)/%.o,$(SOURCES))

all: dir $(BUILDDIR)/$(EXE)

dir:
	mkdir -p $(BUILDDIR)



$(BUILDDIR)/$(EXE):$(OBJECTS)
	$(CC) -o  $@  $^   -I   $(TOOLS_P) $(TOOLS_C)


$(OBJECTS): $(BUILDDIR)/%.o : $(SOURCE)/%.c  $(MACROS) $(GLOBAL) $(HEADER)
	$(CC) $(CFLAGS)   $<  -o $@

clean:
	 rm -f $(BUILDDIR)/*o $(BUILDDIR)/$(EXE) $(SOURCE)/*.c.orig
