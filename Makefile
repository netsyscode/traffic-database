BUILD_PATH := build/
TEST_PATH := test/
LIB_PATH := lib/
COMPONENT_PATH := component/
CC := clang++
COMPILE_OPT := -std=c++17
INC = -I$(LIB_PATH)

TARGET = controllerTest
TEST_SRCS = controllerTest.cpp
COMPONENT_SRCS = packetAggregator.cpp pcapReader.cpp indexGenerator.cpp memoryMonitor.cpp storageMonitor.cpp querier.cpp controller.cpp 
SRCS = $(addprefix $(TEST_PATH), $(TEST_SRCS)) $(addprefix $(COMPONENT_PATH), $(COMPONENT_SRCS))
# TEST_OBJS = $(TEST_SRCS:.cpp=.o)
# COMPONENT_OBJS = $(COMPONENT_SRCS:.cpp=.o)
OBJS = $(addprefix $(BUILD_PATH), $(SRCS:.cpp=.o))

all: clean $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(BUILD_PATH)$(TARGET) -lpcap -pthread

$(BUILD_PATH)$(TEST_PATH)%.o: $(TEST_PATH)%.cpp
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC) -c $< -o $@ $(COMPILE_OPT)

$(BUILD_PATH)$(COMPONENT_PATH)%.o: $(COMPONENT_PATH)%.cpp
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC) -c $< -o $@ $(COMPILE_OPT)

clean:
	rm -f $(BUILD_PATH)$(TARGET) $(BUILD_PATH)$(TEST_PATH)*.o $(BUILD_PATH)$(COMPONENT_PATH)*.o
