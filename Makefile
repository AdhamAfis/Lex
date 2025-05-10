CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic
SRCS = src/main.cpp \
       src/Lexer.cpp \
       src/Token.cpp \
       src/LanguageConfig.cpp \
       src/SymbolTable.cpp \
       src/ExportFormatter.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = lex

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean 