
# add parallelism equal to number of cores every time.
# it seems that adding -jX to MAKEFLAGS directly doesn't work any more.
# included some "random" strings to ensure uniqueness
ifneq ($(PARALELL_WRAPPER_ABXCOEOEKCOEBMQJKHTOEUB),done)

NUM_CORES ?= $(shell grep -c "vendor_id" /proc/cpuinfo)
MAKEFLAGS +=" -j$(NUM_CORES) -l$(NUM_CORES) "

# for the default target case
parallel_wrapper_default_target_anthsqjkshbeohcbmeuthnoethoaeou:
	$(MAKE) PARALELL_WRAPPER_ABXCOEOEKCOEBMQJKHTOEUB=done

# catches everything else
% :
	$(MAKE) $@ PARALELL_WRAPPER_ABXCOEOEKCOEBMQJKHTOEUB=done

# the match for this else is at the end of the file
else

.PHONY: all clean build_info

# remove ALL implicit rules & all suffixes
MAKEFLAGS+=" -r "
BOOST_COMPAT_VERSION_THRESH = 104200
MAKE_COMPAT_VERSION_THRESH = 3.81
MAKE_COMPAT = $(shell [ $$(echo '$(MAKE_VERSION) <= $(MAKE_COMPAT_VERSION_THRESH)' | bc -l) -eq 1 ] && echo yes || echo no)

BUILD_MODE ?= release

BUILD_DIR = ../build/
EXE_DIR  = $(BUILD_DIR)EXE/
OBJ_DIR  = $(BUILD_DIR)OBJ/
DEPS_DIR = $(BUILD_DIR)DEPS/

BUILD_SUBDIRS = $(EXE_DIR) $(OBJ_DIR) $(DEPS_DIR)

# CXX = clang++
# CXX = g++

# put warning flags here - go to compiler *and* linker.
WARNING_FLAGS += -Wall -Wextra -pedantic -Weffc++ -Wconversion -Werror

# put other flags for both the compiler & linker here
EXTRA_FLAGS = -std=c++14 -D X11 -fopenmp

BOOST_VERSION_TEST_INPUT = \#include <boost/version.hpp>\nBOOST_VERSION\n
BOOST_VERSION = $(shell echo -e '$(BOOST_VERSION_TEST_INPUT)' | gcc -x c++ -E - | tail -n 1)
USE_BOOST_COMPAT = $(shell [ $(BOOST_VERSION) -le $(BOOST_COMPAT_VERSION_THRESH) ] && echo yes || echo no)
ifeq ($(USE_BOOST_COMPAT),yes)
	EXTRA_FLAGS += -D ECF_BOOST_COMPAT
endif

# add flags for debugging
ifeq ($(BUILD_MODE),debug)
	EXTRA_FLAGS += -ggdb -D DEBUG # -D _GLIBCXX_DEBUG
endif

# add flags for release
ifeq ($(BUILD_MODE),release)
	EXTRA_FLAGS += -flto -O3
	EXTRA_FLAGS += -Wno-maybe-uninitialized # silence g++
endif

LIBRARY_LINK_FLAGS += \
	$(shell pkg-config --libs cairo) \
	$(shell pkg-config --libs fontconfig) \
	$(shell pkg-config --libs x11) \
	$(shell pkg-config --libs xft) \
	-lboost_system \
	-lpthread

ifeq ($(USE_BOOST_COMPAT),yes)
	LIBRARY_LINK_FLAGS += -lboost_thread-mt
else
	LIBRARY_LINK_FLAGS += -lboost_thread
endif

INCLUDE_FLAGS += \
	-I .

GRAPHICS_INCL_FLAGS += $(shell pkg-config --cflags cairo)

CXXFLAGS += $(EXTRA_FLAGS) $(WARNING_FLAGS) $(INCLUDE_FLAGS)
LDFLAGS  += $(EXTRA_FLAGS) $(WARNING_FLAGS) $(LIBRARY_LINK_FLAGS)

# keep .o files
.PRECIOUS: $(OBJ_DIR)%.o

# define source directories
SOURCE_DIRS = algo/ flows/ graphics/ parsing/ util/ util/tests/ ./

ALL_OBJ_DIRS  = $(addprefix $(OBJ_DIR),  $(SOURCE_DIRS))
ALL_DEPS_DIRS = $(addprefix $(DEPS_DIR), $(SOURCE_DIRS))

# compute all directories that might need creation
DIRS= \
	$(ALL_OBJ_DIRS) \
	$(ALL_DEPS_DIRS) \
	$(BUILD_SUBDIRS) \
	$(BUILD_DIR)

# define executables
TEST_EXES=$(EXE_DIR)test-netlist
EXES=$(EXE_DIR)maize-router $(TEST_EXES)

all: $(EXES) test | build_info

test: $(patsubst %, run_%, $(TEST_EXES))

build_info:
	@echo "Building with makeflags ${MAKEFLAGS}"
	@echo "In build mode ${BUILD_MODE}"
ifeq ($(MAKE_COMPAT),yes)
ifneq ($(NO_MAKE_VER_WARN),yes)
	@echo -e "WARNING: #############################################################################################"
	@echo -e "WARNING: You seem to be  building with an old version of Make. Building may require multiple attempts."
	@echo -e "WARNING: Set NO_MAKE_VER_WARN=yes to hide this"
	@echo -e "WARNING: #############################################################################################"
endif
endif

# add more dependencies here:
$(EXE_DIR)maize-router: \
	$(OBJ_DIR)main.o \
	$(OBJ_DIR)algo/maze_router.o \
	$(OBJ_DIR)algo/routing.o \
	$(OBJ_DIR)flows/flows.o \
	$(OBJ_DIR)graphics/fontcache.o \
	$(OBJ_DIR)graphics/fpga_graphics_data.o \
	$(OBJ_DIR)graphics/graphics.o \
	$(OBJ_DIR)graphics/graphics_wrapper.o \
	$(OBJ_DIR)graphics/graphics_state.o \
	$(OBJ_DIR)graphics/graphics_types.o \
	$(OBJ_DIR)graphics/Surface.o \
	$(OBJ_DIR)graphics/SurfaceImpl.o \
	$(OBJ_DIR)parsing/cmdargs_parser.o \
	$(OBJ_DIR)parsing/input_parser.o \
	$(OBJ_DIR)util/logging.o \
	$(OBJ_DIR)util/thread_utils.o \


$(EXE_DIR)test-netlist: \
	$(OBJ_DIR)util/tests/netlist_test.o \


# define extra flags for particular object files
# adds graphics include flags to everything in graphics dir
$(OBJ_DIR)graphics/%.o: INCLUDE_FLAGS+=$(GRAPHICS_INCL_FLAGS)

# include all the dependency files, if any exist
EXISTING_DEP_FILES = \
	$(foreach dir,$(SOURCE_DIRS), \
		$(wildcard $(DEPS_DIR)$(dir)*.d) \
	)
ifneq (,$(EXISTING_DEP_FILES))
include $(EXISTING_DEP_FILES)
endif

# compile .c++ to .o
# second CC line generates the initial dependency file
# first sed line adds $(OBJ_DIR) prefixes to the dependency file,
# second one adds stub rules for each depended on file (make might
# complain with generated files)
.SECONDEXPANSION:
$(OBJ_DIR)%.o: %.cpp | build_info $(OBJ_DIR)$$(dir %) $(DEPS_DIR)$$(dir %)
	'$(CXX)' -c  '$(shell readlink --canonicalize $<)' -o  '$@' $(CXXFLAGS)
	@'$(CXX)' -MM '$<' -MF '$(DEPS_DIR)$<.d.tmp' $(CXXFLAGS)
	@sed -e 's|.*:|$@:|' < '$(DEPS_DIR)$<.d.tmp' > '$(DEPS_DIR)$<.d'
	@sed -e 's/.*://' -e 's/\\$$//' < '$(DEPS_DIR)$<.d.tmp'| fmt -1 | \
	 sed -e 's/^ *//' -e 's/$$/:/' >> '$(DEPS_DIR)$<.d'
	@rm -f $(DEPS_DIR)$<.d.tmp

# compile *.o's into an executable
$(EXE_DIR)%: | build_info $(EXE_DIR)
	'$(CXX)' $^ -o '$@' $(LDFLAGS)

$(patsubst %, run_%, $(EXES)): $$(patsubst run_%, %, $$(@))
	@( $^ && echo "SUCCESS: $^" ) || echo "FAIL: $^"

$(DIRS):
	mkdir -p $@

ifeq ($(MAKE_COMPAT),yes)
clean:
	rm -rf $(BUILD_DIR)
else
clean: $(BUILD_DIR:%=delete_%_dir)
endif


$(patsubst %, delete_%_dir, $(BUILD_DIR)    ): $$(patsubst %, delete_%_dir, $(BUILD_SUBDIRS))
$(patsubst %, delete_%_dir, $(OBJ_DIR)      ): $$(patsubst %, delete_%_dir, $$(ALL_OBJ_DIRS))
$(patsubst %, delete_%_dir, $(DEPS_DIR)     ): $$(patsubst %, delete_%_dir, $$(ALL_DEPS_DIRS))
$(patsubst %, delete_%_dir, $(EXE_DIR)      ): $$(patsubst %, delete_%_file/, $$(EXES))
$(patsubst %, delete_%_dir, $(ALL_OBJ_DIRS) ): $$(patsubst delete_%_dir, delete_%*.o_file/, $$(@))
$(patsubst %, delete_%_dir, $(ALL_DEPS_DIRS)): $$(patsubst delete_%_dir, delete_%*.d_file/, $$(@))

delete_%_file/:
	@$(eval DELFILE = $(@:delete_%_file/=%))
	@for f in $(DELFILE); do if [ $$f ]; then echo rm -f $$f; fi; done;
	@for f in $(DELFILE); do if [ $$f ]; then      rm -f $$f; fi; done;

$(DIRS:%=delete_%_dir):
	@$(eval DELDIR = $(@:delete_%_dir=%))
	@if [ -d "$(DELDIR)" ]; then echo rmdir "$(DELDIR)"; fi
	@if [ -e "$(DELDIR)" ]; then      rmdir --ignore-fail-on-non-empty "$$(readlink --canonicalize "$(DELDIR)")"; fi

endif
