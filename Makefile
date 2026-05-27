# Detect OSX
OS_UPPER:=$(shell uname -s 2>/dev/null | tr [:lower:] [:upper:])
DARWIN:=$(strip $(findstring DARWIN,$(OS_UPPER)))

# Get directory of this makefile (means make can be invoked else where)
ANTWORLD_DIR:=$(abspath $(dir $(lastword $(MAKEFILE_LIST))))

# Set standard compiler and archiver flags
CXXFLAGS+=-std=c++17 -march=native -Wall -Wpedantic -Wextra -MMD -MP -I$(ANTWORLD_DIR)/include
ARFLAGS:=-rcs

ifdef DYNAMIC
    PREFIX:=$(PREFIX)_dynamic
    CXXFLAGS+=-fPIC
    LIBRARY_EXTENSION:=so
else
    LIBRARY_EXTENSION:=a
endif

ifdef DEBUG
    PREFIX:=$(PREFIX)_debug
    CXXFLAGS+=-g -O0 -DDEBUG
else
    CXXFLAGS+=-O3
endif 


# Get object, source and libary directories
OBJECT_DIRECTORY:=$(ANTWORLD_DIR)/obj$(PREFIX)
SOURCE_DIRECTORY:=$(ANTWORLD_DIR)/src
LIBRARY_DIRECTORY?=$(ANTWORLD_DIR)/lib

PACKAGES=pkg-config opengl opencv4 sfml-window
LIBANTWORLD:=$(LIBRARY_DIRECTORY)/libantworld$(PREFIX).$(LIBRARY_EXTENSION)

# Find source files
SOURCES:=$(wildcard $(SOURCE_DIRECTORY)/*.cc)

# Add object directory prefix
OBJECTS:=$(SOURCES:$(SOURCE_DIRECTORY)/%.cc=$(OBJECT_DIRECTORY)/%.o)
DEPS:=$(OBJECTS:.o=.d)

# Add GeNN include directories
CXXFLAGS +=$(shell pkg-config $(PACKAGES) --cflags)

.PHONY: all clean

all: $(LIBANTWORLD)

ifdef DYNAMIC
ifeq ($(DARWIN),DARWIN)
$(LIBANTWORLD): $(OBJECTS)
	mkdir -p $(@D)
	$(CXX) -dynamiclib -undefined dynamic_lookup $(CXXFLAGS) -o $@ $(OBJECTS)
	install_name_tool -id "@loader_path/$(@F)" $@
else
$(LIBANTWORLD): $(OBJECTS)
	mkdir -p $(@D)
	$(CXX) -shared $(CXXFLAGS) -o $@ $(OBJECTS)
endif
else
$(LIBANTWORLD): $(OBJECTS)
	mkdir -p $(@D)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)
endif

-include $(DEPS)

$(OBJECT_DIRECTORY)/%.o: $(SOURCE_DIRECTORY)/%.cc $(OBJECT_DIRECTORY)/%.d
	mkdir -p $(@D)
	$(CXX) -std=c++17 $(CXXFLAGS) -c -o $@ $<

%.d: ;

clean:
	@find $(OBJECT_DIRECTORY) -type f -name "*.o" -delete
	@find $(OBJECT_DIRECTORY) -type f -name "*.d" -delete
	@rm -f $(LIBANTWORLD)
