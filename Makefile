DOES_ICC_EXIST := $(shell icc --version 2>/dev/null)

ifdef DOES_ICC_EXIST
  $(info Using 'icc' Intel-specific compiler)
  CXX = icc
  CFLAGS = -no-prec-div -ip -xAVX
else
  $(info Using 'g++' compiler)
  CXX = g++
endif

CFLAGS += -openmp -O3 -g
LDFLAGS = -L. -openmp -O3

APPS = tvs tvs-test

SRC_DIR = ./src
INCLUDE_DIR = ./include
TEST_DIR = ./test
BIN_DIR = ./etc/bin
$(shell mkdir -p $(BIN_DIR))

ALL_SRCS = $(wildcard $(SRC_DIR)/*.cpp $(INCLUDE_DIR)/**/*.cpp)
ALL_OBJS = $(patsubst %.cpp,%.o, $(ALL_SRCS))

TEST_SRCS = $(wildcard $(TEST_DIR)/*.cpp)
DEP_SRCS = $(filter-out $(SRC_DIR)/main.cpp,$(ALL_SRCS))
TEST_OBJS = $(patsubst %.cpp,%.o,$(TEST_SRCS))
DEP_OBJS = $(patsubst %.cpp,%.o,$(DEP_SRCS))

# Uncomment these lines to check these lists
#$(warning ALL_SRCS is $(ALL_SRCS))
#$(warning ALL_OBJS is $(ALL_OBJS))
#$(warning TEST_SRCS is $(TEST_SRCS))
#$(warning DEP_SRCS is $(DEP_SRCS))
#$(warning TEST_OBJS is $(TEST_OBJS))
#$(warning DEP_OBJS is $(DEP_OBJS))

# Provides a shorthand for downloading libs from github.com
# Usage: $(call getLibFromGithub,SergiusTheBest/plog,e82f39a,include/plog)
# 'SergiusTheBest/plog' is the repo as found in the HTTP URL.
# 'e82f39a' is the specific hash ref, can be anything that's valid for `git checkout`.
# 'include/plog' is the space-delimited set of paths to actually use from the repo.
define getLibFromGithub
	@repo_sig=$$(echo $(1) | cut -d "/" -f2); \
	git_lib_dir=$$repo_sig; \
	tmp_dir=$$git_lib_dir-git; \
	if [ ! -d "$(INCLUDE_DIR)/$$git_lib_dir" ]; then \
		mkdir -p $(INCLUDE_DIR)/$$git_lib_dir; \
		pushd $(INCLUDE_DIR); \
		git clone --depth=1 https://github.com/$(1) $$tmp_dir; \
		cd $$tmp_dir && git checkout $(2); \
		cd ..; \
		required_paths=$$(echo $(3) | tr -s " " "\n"); \
		while read -r path; do \
			path=$$tmp_dir/$$path; \
			cp_args=$$([ -d "$$path" ] && echo "-r --no-target-directory"); \
			cp $$cp_args $$path $$git_lib_dir; \
		done <<< "$$required_paths"; \
		rm -rf $$tmp_dir; \
		popd; \
	fi
endef

all: download-libs $(APPS)

tvs: $(DEP_OBJS) src/main.o
	$(CXX) $(LDFLAGS) $^ -o $(BIN_DIR)/$@

tvs-test: $(TEST_OBJS) $(DEP_OBJS)
	@# bfd provides full file/line backtraces (used by catch.hpp)
	$(CXX) $(LDFLAGS) $^ -o $(BIN_DIR)/$@ -lbfd

download-libs:
	$(call getLibFromGithub,SergiusTheBest/plog,e82f39a,include/plog)
	$(call getLibFromGithub,bombela/backward-cpp,7693dd9,backward.hpp)
	$(call getLibFromGithub,philsquared/catch,2be372,single_include)
	$(call getLibFromGithub,lvandeve/lodepng,8a0f16a,lodepng.h lodepng.cpp)
	@# A horrible hack to comment out a line that prevents backward.hpp from being able to
	@# receive SIGSEGV exceptions. See the following Github issue for progress:
	@# https://github.com/philsquared/Catch/issues/611
	@sed -e '/SIGSEGV/s/^/\/\//g' -i $(INCLUDE_DIR)/catch/catch.hpp;

%.o: %.cpp
	$(CXX) $(CFLAGS) -I $(INCLUDE_DIR) -c $< -o $@

clean:
	rm $(addprefix $(BIN_DIR)/,$(APPS)) $(ALL_OBJS) $(TEST_OBJS)

test: download-libs $(APPS)
	$(BIN_DIR)/tvs-test

.PHONY:	all $(APPS) download-libs clean test
