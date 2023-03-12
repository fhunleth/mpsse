# Makefile for building the NIF
#
# Makefile targets:
#
# all/install   build and install the NIF
# clean         clean build products and intermediates
#
# Variables to override:
#
# MIX_APP_PATH  path to the build directory
# MIX_ENV       Mix build environment - "test" forces use of the stub
#
# CC            C compiler
# CROSSCOMPILE	crosscompiler prefix, if any
# CFLAGS	compiler flags for compiling all C files
# ERL_CFLAGS	additional compiler flags for files using Erlang header files
# ERL_EI_INCLUDE_DIR include path to ei.h (Required for crosscompile)
# ERL_EI_LIBDIR path to libei.a (Required for crosscompile)
# LDFLAGS	linker flags for linking all binaries
# ERL_LDFLAGS	additional linker flags for projects referencing Erlang libraries

PREFIX = $(MIX_APP_PATH)/priv
BUILD  = $(MIX_APP_PATH)/obj

NIF = $(PREFIX)/mpsse_nif.so

SRC = c_src/mpsse_nif.c c_src/libmpsse/src/mpsse.c c_src/libmpsse/src/fast.c c_src/libmpsse/src/support.c
CFLAGS ?= -O2 -Wall -Wextra -Wno-unused-parameter -pedantic
CFLAGS += -Ic_src -Ic_src/libmpsse
LDFLAGS += -lftdi1

# Check that we're on a supported build platform
ifeq ($(CROSSCOMPILE),)
    # Not crosscompiling, so check that we're on Linux for whether to compile the NIF.
    ifeq ($(shell uname -s),Linux)
        CFLAGS += -fPIC
        LDFLAGS += -fPIC -shared
    else
	LIBFTDI_PATH = $(shell brew --prefix libftdi)
        CFLAGS += -I$(LIBFTDI_PATH)/include
        LDFLAGS += -L$(LIBFTDI_PATH)/lib -undefined dynamic_lookup -dynamiclib
    endif
else
# Crosscompiled build
CFLAGS += -fPIC
LDFLAGS += -fPIC -shared
endif

# Set Erlang-specific compile and linker flags
ERL_CFLAGS ?= -I$(ERL_EI_INCLUDE_DIR)
ERL_LDFLAGS ?= -L$(ERL_EI_LIBDIR) -lei

HEADERS =$(wildcard c_src/*.h)
OBJ = $(SRC:c_src/%.c=$(BUILD)/%.o)

calling_from_make:
	mix compile

all: install

install: $(PREFIX) $(BUILD) $(BUILD)/libmpsse/src $(NIF)

$(OBJ): $(HEADERS) Makefile

$(BUILD)/%.o: c_src/%.c
	@echo " CC $(notdir $@)"
	$(CC) -c $(ERL_CFLAGS) $(CFLAGS) -o $@ $<

$(NIF): $(OBJ)
	@echo " LD $(notdir $@)"
	$(CC) -o $@ $(ERL_LDFLAGS) $(LDFLAGS) $^

$(PREFIX) $(BUILD) $(BUILD)/libmpsse/src:
	mkdir -p $@

clean:
	$(RM) $(NIF) $(OBJ)

.PHONY: all clean calling_from_make install

# Don't echo commands unless the caller exports "V=1"
${V}.SILENT:
