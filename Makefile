CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic
SRCS = src/main.cpp \
       src/Lexer.cpp \
       src/Token.cpp \
       src/LanguageConfig.cpp \
       src/SymbolTable.cpp \
       src/ExportFormatter.cpp \
       src/ConfigLoader.cpp \
       src/LanguagePlugin.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = lex

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

-include $(OBJS:.o=.d)

clean:
	rm -f $(OBJS) $(OBJS:.o=.d) $(TARGET)

test: $(TARGET)
	python3 -m unittest discover -s tests -p 'test_*.py'
	node --test tests/test_web.cjs

.PHONY: all clean test
