CXX = em++
CXXFLAGS = -std=c++17 -O2 -fexceptions -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString']
SRCS = src/wasm_bindings.cpp \
       src/Lexer.cpp \
       src/Token.cpp \
       src/LanguageConfig.cpp \
       src/SymbolTable.cpp \
       src/ExportFormatter.cpp \
       src/ConfigLoader.cpp \
       src/LanguagePlugin.cpp
OBJS = $(SRCS:.cpp=.wasm.o)
TARGET = web/lex.js

EMFLAGS = -s EXPORTED_FUNCTIONS=['_tokenizeString','_getLanguageNames','_registerLanguageConfig','_initModule','_free'] \
          -s EXPORT_NAME="LexModule" \
          -s MODULARIZE=1 \
          -s INVOKE_RUN=1 \
          --preload-file plugins@/plugins \
          -o $(TARGET)

all: web_dir $(TARGET)

test: all
	node tests/test_wasm.cjs

web_dir:
	mkdir -p web
	mkdir -p web/plugin_templates

$(TARGET): $(OBJS) $(wildcard plugins/*.json)
	$(CXX) $(CXXFLAGS) $(EMFLAGS) -o $@ $(OBJS)

%.wasm.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(OBJS:.o=.d)

clean:
	rm -f $(OBJS) $(OBJS:.o=.d)
	rm -f $(TARGET) $(TARGET).mem $(TARGET).wasm web/lex.data

.PHONY: all clean web_dir test
