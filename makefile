CXX = g++
CXXFLAGS = --std=c++17 -Iinclude -Iexternal/imgui -Iexternal/imgui/backends -Iexternal/json/include -I/usr/include/SDL2
LDFLAGS = -lSDL2 -lSDL2main

EXEC = nes
SRC_DIR = src
OBJ_DIR = bin
IMGUI_DIR = external/imgui
IMGUI_BACKEND_DIR = $(IMGUI_DIR)/backends

APP_SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
IMGUI_SOURCES = $(wildcard external/imgui/*.cpp)
IMGUI_BACKEND_SOURCES = $(IMGUI_BACKEND_DIR)/imgui_impl_sdl2.cpp $(IMGUI_BACKEND_DIR)/imgui_impl_sdlrenderer2.cpp
SOURCES = $(APP_SOURCES) $(IMGUI_SOURCES) $(IMGUI_BACKEND_SOURCES)

OBJECTS = $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(notdir $(SOURCES)))

all: $(EXEC)

$(EXEC): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $(EXEC)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(OBJ_DIR)/%.o: $(IMGUI_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(OBJ_DIR)/%.o: $(IMGUI_BACKEND_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(EXEC)

rebuild: clean all

.PHONY: all clean rebuild