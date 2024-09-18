BUILD_PATH := build/
TEST_PATH := test/
LIB_PATH := dpdk_lib/
COMPONENT_PATH := dpdk_component/
CC := clang++
COMPILE_OPT := -std=c++17
INC = -I$(LIB_PATH)

TARGET = zOrderTest
TEST_SRCS = zOrderTest.cpp
COMPONENT_SRCS = 
SRCS = $(addprefix $(TEST_PATH), $(TEST_SRCS)) $(addprefix $(COMPONENT_PATH), $(COMPONENT_SRCS))
# TEST_OBJS = $(TEST_SRCS:.cpp=.o)
# COMPONENT_OBJS = $(COMPONENT_SRCS:.cpp=.o)
OBJS = $(addprefix $(BUILD_PATH), $(SRCS:.cpp=.o))

# PKGCONF ?= pkg-config
# ifneq ($(shell $(PKGCONF) --exists libdpdk && echo 0),0)
# $(error "no installation of DPDK found")
# endif

# PC_FILE := $(shell $(PKGCONF) --path libdpdk 2>/dev/null)
# COMPILE_OPT += -O3 $(shell $(PKGCONF) --cflags libdpdk)
# COMPILE_OPT += -DALLOW_EXPERIMENTAL_API
# LDFLAGS_SHARED = $(shell $(PKGCONF) --libs libdpdk)

all: clean $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(BUILD_PATH)$(TARGET) $(LDFLAGS) -lpcap -pthread $(LDFLAGS_SHARED)

$(BUILD_PATH)$(TEST_PATH)%.o: $(TEST_PATH)%.cpp
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC) -c $< -o $@ $(COMPILE_OPT)

$(BUILD_PATH)$(COMPONENT_PATH)%.o: $(COMPONENT_PATH)%.cpp
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC) -c $< -o $@ $(COMPILE_OPT) $(LDFLAGS) $(LDFLAGS_SHARED)

clean:
	rm -f $(BUILD_PATH)$(TARGET) $(BUILD_PATH)$(TEST_PATH)*.o $(BUILD_PATH)$(COMPONENT_PATH)*.o
