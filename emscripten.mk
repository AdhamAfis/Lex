CXX = emcc
CXXFLAGS = -std=c++17 -O2 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap']
SRCS = src/wasm_bindings.cpp \
       src/Lexer.cpp \
       src/Token.cpp \
       src/LanguageConfig.cpp \
       src/SymbolTable.cpp \
       src/ExportFormatter.cpp \
       src/ConfigLoader.cpp \
       src/LanguagePlugin.cpp
OBJS = $(SRCS:.cpp=.wasm.o)  # Use .wasm.o to avoid conflicts with regular .o files
TARGET = web/lex.js

# Additional flags for the final linking step
EMFLAGS = -s EXPORTED_FUNCTIONS=['_tokenizeString','_getLanguageNames','_registerLanguageConfig'] \
          -s EXPORT_NAME="LexModule" \
          -s MODULARIZE=1 \
          --preload-file plugins \
          -o $(TARGET)

all: web_dir $(TARGET)

web_dir:
	mkdir -p web
	mkdir -p web/plugin_templates

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(EMFLAGS) -o $@ $^

%.wasm.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Only clean WebAssembly build artifacts, not the entire web directory
clean:
	rm -f $(OBJS)
	rm -f $(TARGET) $(TARGET).mem $(TARGET).wasm web/lex.data

.PHONY: all clean web_dir 