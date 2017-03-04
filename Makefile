INCLUDES = -I$(CLANG_DIR)/reflection
REFL_FLAGS = -std=c++1z -Xclang -freflection

all: type_synthesis concepts

type_synthesis: type_synthesis.cpp
	$(CXX) $(INCLUDES) $(REFL_FLAGS) type_synthesis.cpp -o type_synthesis

concepts: concepts.cpp
	$(CXX) $(INCLUDES) $(REFL_FLAGS) -Xclang -fconcepts-ts -O3 concepts.cpp -o concepts

reflopt: reflopt.cpp
	$(CXX) $(INCLUDES) $(REFL_FLAGS) reflopt.cpp -o reflopt

