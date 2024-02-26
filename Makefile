BUILD_PATH := ./build/
CC := clang++
COMPILE_OPT := -std=c++11

all: test.o extractor.o flow.o index.o storage.o querier.o
	$(CC) $(BUILD_PATH)test.o $(BUILD_PATH)extractor.o $(BUILD_PATH)flow.o $(BUILD_PATH)index.o $(BUILD_PATH)storage.o $(BUILD_PATH)querier.o -o $(BUILD_PATH)test -lpcap

test.o: test.cpp
	$(CC) -c test.cpp -o $(BUILD_PATH)test.o $(COMPILE_OPT)

extractor.o: extractor.cpp extractor.hpp header.hpp
	$(CC) -c extractor.cpp -o $(BUILD_PATH)extractor.o $(COMPILE_OPT)

flow.o: flow.cpp flow.hpp
	$(CC) -c flow.cpp -o $(BUILD_PATH)flow.o $(COMPILE_OPT)

index.o: index.cpp index.hpp skipList.hpp
	$(CC) -c index.cpp -o $(BUILD_PATH)index.o $(COMPILE_OPT)

storage.o: storage.cpp storage.hpp
	$(CC) -c storage.cpp -o $(BUILD_PATH)storage.o $(COMPILE_OPT)

querier.o: querier.cpp querier.hpp
	$(CC) -c querier.cpp -o $(BUILD_PATH)querier.o $(COMPILE_OPT)

# skipList.o: skipList.cpp skipList.hpp
# 	$(CC) -c skipList.cpp -o $(BUILD_PATH)skipList.o $(COMPILE_OPT)

# prefixTree.o: prefixTree.cpp prefixTree.hpp
# 	$(CC) -c prefixTree.cpp -o $(BUILD_PATH)prefixTree.o $(COMPILE_OPT)

clean:
	rm $(BUILD_PATH)*.o
	rm $(BUILD_PATH)test