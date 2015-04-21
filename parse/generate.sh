lex -o ../sources/lex.cpp lex.l
yacc --defines=../sources/parse.h -o ../sources/parse.cpp parse.y
