PROJECT_NAME = $(shell basename $(CURDIR))

CC=g++
CC_FLAGS= -c -Wall -Wno-char-subscripts -Iinclude -I/usr/local/include
LD_FLAGS=-L/usr/local/lib -lnanomsg -lprotobuf -lrt

CPP_FILES= $(wildcard src/*.cpp)
OBJ_FILES=$(addprefix .bin/,$(notdir $(CPP_FILES:.cpp=.o)))

all: $(CPP_FILES) $(PROJECT_NAME)

LIBRARY= lib$(PROJECT_NAME).a
EXECUTABLE= $(PROJECT_NAME)

$(PROJECT_NAME): $(OBJ_FILES)
	@echo "[link]"
	@$(CC) $(OBJ_FILES) $(LD_FLAGS) -o $@
	@echo "[done] $(EXECUTABLE): Compilation finished!"

.bin/%.o: src/%.cpp
	@echo "[make] $@"
	@$(CC) $(CC_FLAGS) $< -o $@

lib:
	ar rcs $(LIBRARY) $(OBJ_FILES)
	@echo "[make] $(LIBRARY): Compilation finished!"

clean:
	@echo "[make] clean: Deleting object files"
	@rm -f $(OBJ_FILES) *.o core
	@echo "[make] clean: Deleting builds"
	@rm -f $(EXECUTABLE)

destroy:
	@rm -f $(EXECUTABLE)
	@rm -rf .bin/ include/ .proj/ src/ .proto

protoc:
	@protoc -I=proto --cpp_out=src proto/*.proto
	@mv src/*.pb.h include/

create:
	@mkdir .proj
	@mkdir .bin
	@mkdir .proto
	@mkdir include
	@mkdir src
	@touch src/main.cpp
	@echo "#include <iostream>\n\nint main(int argc, char** argv)\n{\n\tstd::cout << \"Hello World!\";\n\tstd::cout << std::endl;\n}\n" > src/main.cpp
	@touch .proj/project.sublime-project
	@echo "{\n\t\"folders\":\n\t[\n\t\t{\"path\": \"$(CURDIR)\"\n\t\t,\"folder_exclude_patterns\": [\".*"]\n\t\t,\"file_exclude_patterns\": [\"Makefile\"]}\n\t]\n}" > .proj/project.sublime-project

