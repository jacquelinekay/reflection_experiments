INCLUDES = -I$(CLANG_DIR)/reflection
REFL_FLAGS = -std=c++1z -Xclang -freflection

type_synthesis: type_synthesis.cpp
	$(CXX) $(INCLUDES) $(REFL_FLAGS) type_synthesis.cpp
