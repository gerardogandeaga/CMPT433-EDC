# Makefile source: Chnossos's answer https://stackoverflow.com/questions/31609667/c-makefile-error-no-rule-to-make-target-cpp
TARGET = edc-program
SDIR = ./src
# include directory, this includes the header files
IDIR = ./include
# where the .o files will go
ODIR = ./obj
# where the final builds should go
BDIR = ./bin

CXX = arm-linux-gnueabihf-g++
# CXX = g++
CFLAGS = -Wall -g -std=c++14 -Werror -I$(IDIR) -MMD -MP
# library linking
LDFLAGS := -L../lib
LDLIBS := -lm -lpthread

# dependecies and obj names
# creates the actual paths the header and obj files are actually mapped to
# Filter out the LCD test driver to avoid `multiple definitions 
# of main` error when compiling edc-program.
SRC = $(filter-out ./src/lcd-test.cpp, $(wildcard ./src/*.cpp))
OBJ = $(SRC:$(SDIR)/%.cpp=$(ODIR)/%.o)
DEP = $(OBJ:.o=.d)

# create the obj directory if needed

all: tmp $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@
	cp $(TARGET) $(HOME)/cmpt433/public/myApps/
	mv $(TARGET) $(BDIR)/

$(ODIR)/%.o: $(SDIR)/%.cpp
	$(CXX) -o $@ -c $< $(CFLAGS)

tmp:
	mkdir -p $(BDIR) ${ODIR}

.PHONY: clean

clean:
	rm -rf $(ODIR) $(BDIR) $(HOME)/cmpt433/public/myApps/$(TARGET)

-include $(DEP) $(DEP_LCD_TEST)
