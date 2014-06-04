# Make Plib Test Solution.
# Powered By Push Chen
# 
#

PLIB_ROOT = ../
TEST_ROOT = ./

OUT_DIR = $(TEST_ROOT)/output

PLIB_DEFINES = -v -DPLIB -DVERSION=\"1.0.0.0\" #-std=c++11

ifeq "$(MAKECMDGOALS)" "release"
	DEFINES = $(PLIB_DEFINES) -DPLIB_RELEASE -DPLIB_USE_MEMPOOL -DPLIB_LOG_DEBUG_
	CPPFLAGS = 
	CFLAGS = -O2 -Wall $(DEFINES) -fPIC -pthread
	CXXFLAGS = -O2 -Wall $(DEFINES) -fPIC -pthread
	CMDGOAL_DEF := $(MAKECMDGOALS)
else
	ifeq "$(MAKECMDGOALS)" "withpg"
		DEFINES = $(PLIB_DEFINES) -DPLIB_WITHPG -DDEBUG -D_PLIB_ERR_DEBUG_ -DPLIB_MEMORY_DEBUG -DPLIB_OBJECT_DEBUG
		CPPFLAGS = 
		CFLAGS = -g -pg -Wall $(DEFINES) -fPIC -pthread
		CXXFLAGS = -g -pg -Wall $(DEFINES) -fPIC -pthread
		CMDGOAL_DEF := $(MAKECMDGOALS)
	else
		DEFINES = $(PLIB_DEFINES) -DPLIB_DEBUG -D_PLIB_STD_DEBUG_ -DPLIB_LOCK_DEBUG -DDEBUG
		CPPFLAGS =
		CFLAGS = -g -Wall $(DEFINES) -fPIC -pthread
		CXXFLAGS = -g -Wall $(DEFINES) -fPIC -pthread
		CMDGOAL_DEF := $(MAKECMDGOALS)
	endif
endif

vpath %.h   ./
vpath %.cpp	./

CC	 = g++
CPP	 = g++
CXX	 = g++
AR	 = ar

STATIC_LIBS = 
DYNAMIC_LIBS = 
EXECUTABLE = 
TEST_CASE = reqt pdns

RELAY_OBJECT = 

all	: PreProcess $(STATIC_LIBS) $(DYNAMIC_LIBS) $(EXECUTABLE) $(TEST_CASE) AfterMake

PreProcess :
	@if test -d $(OUT_DIR); then rm -rf $(OUT_DIR); fi
	@mkdir -p $(OUT_DIR)/static
	@mkdir -p $(OUT_DIR)/dynamic
	@mkdir -p $(OUT_DIR)/bin
	@mkdir -p $(OUT_DIR)/test
	@mkdir -p $(OUT_DIR)/data
	@mkdir -p $(OUT_DIR)/conf
	@mkdir -p $(OUT_DIR)/log

cmdgoalError :
	@echo '***************************************************'
	@echo '******You must specified a make command goal.******'
	@echo '***************************************************'

clean : 
	$(shell rm -rf $(OUT_DIR))

debug : PreProcess $(STATIC_LIBS) $(DYNAMIC_LIBS) $(EXECUTABLE) $(TEST_CASE) AfterMake
	@exit 0

release : PreProcess $(STATIC_LIBS) $(DYNAMIC_LIBS) $(EXECUTABLE) $(TEST_CASE) AfterMake
	@exit 0

withpg : PreProcess $(STATIC_LIBS) $(DYNAMIC_LIBS) $(EXECUTABLE) $(TEST_CASE) AfterMake
	@exit 0

AfterMake : 
	rm -rf *.o
	static_libs="$(STATIC_LIBS)" && \
    if [ "${static_libs}" != "" ]; then \
        for slib in $${static_libs}; do \
            mv $$slib $(OUT_DIR)/static; \
        done; \
    fi
	dynamic_libs="$(DYNAMIC_LIBS)" && \
    if [ "${dynamic_libs}" != "" ]; then \
        for dlib in $${dynamic_libs}; do \
            mv $$dlib $(OUT_DIR)/dynamic; \
        done; \
    fi
	executable="$(EXECUTABLE)" && \
	if [ "$${executable}" != "" ]; then \
	    for exc in $${executable}; do \
	        mv $$exc $(OUT_DIR)/bin; \
	    done; \
	fi
	test_case="$(TEST_CASE)" && \
	if [ "$${test_case}" != "" ]; then \
		for tcase in $${test_case}; do \
		    mv $$tcase $(OUT_DIR)/test; \
		done; \
	fi

reqt : RequestTest.o
	$(CPP) -o reqt $(TEST_ROOT)/RequestTest.cpp $(CXXFLAGS)

