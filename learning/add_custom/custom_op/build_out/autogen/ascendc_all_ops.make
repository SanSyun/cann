
# shared library name
LIBRARY_NAME := ascend_all_ops

# src file list
SRCS := /mnt/workspace/gitCode/my_projects/learning/add_custom/custom_op/op_host/add_custom_template.cpp

# output dir
BUILD_DIR := /mnt/workspace/gitCode/my_projects/learning/add_custom/custom_op/build_out/autogen
LIB_DIR := $(BUILD_DIR)/

# target objs
OBJS_CPP := $(SRCS:%.cpp=$(BUILD_DIR)/$(LIBRARY_NAME)/%.o)
OBJS := $(OBJS_CPP:%.cc=$(BUILD_DIR)/$(LIBRARY_NAME)/%.o)

CXX := /usr/bin/c++
CXXFLAGS := -fPIC -D_GLIBCXX_USE_CXX11_ABI=0 -std=c++11  -I/home/developer/Ascend/cann-8.5.2/include
LDFLAGS := -shared

TARGET := $(LIB_DIR)/lib$(LIBRARY_NAME).so

all: $(TARGET)
	rm -rf $(BUILD_DIR)/$(LIBRARY_NAME)/

$(shell mkdir -p $(dir $(OBJS)) $(LIB_DIR))

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^  -lexe_graph -lregister -ltiling_api -L/home/developer/Ascend/cann-8.5.2/lib64

$(BUILD_DIR)/$(LIBRARY_NAME)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD_DIR)/$(LIBRARY_NAME)/%.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
