#include <cstdlib>
#include <cstdio>
#include <assert.h>
#include "Vocabulary.h"
#include "structs.h"
#include "DependenceGraph.h"
#include <set>
#include <iostream>
#include <unistd.h>
#include <fstream>
#include <string>
#include <cstring>
#include <sstream>
#include <ctime>
#include <time.h>

using namespace std;

FILE* summary;
FILE* unit;
extern FILE* yyin;
extern vector<Rule> G_NLP;
extern set<int> U;
extern int *atomState;
extern int *ruleState;
extern int yyparse();
FILE* result_out;
FILE* count_out;        // 保存所有输入文件中的“Atoms(b_U(P)) \ U”数量


void io(const char* iPathName, const char* oPathName, const char* sPathName) {
    yyin = fopen(iPathName, "r");
    unit = fopen(oPathName, "w+");
    summary = fopen(sPathName, "a+");
    
    if (! yyin) {
        printf("IO Error: cannot open the input file.\n" );
        assert(0);
    }
    if (!summary || !unit) {
        printf("IO Error: cannot open the output file.\n");
        assert(0);
    }
}

int main(int argc, char** argv) {
      //--------------------------------------
    int nodecount = 0;
    FILE* result;
    if(argc > 2) {
        yyin = fopen(argv[1], "r");
        result = fopen(argv[2], "w+");
        int n, v;
        char s[15];
        sscanf(argv[1], "%[^0-9]%d_%d", s, &n, &v);        
        nodecount = n * v;
    }
    else {
        yyin = fopen("IO/input/nva2_3.txt.in", "r");
        nodecount = 6;
    }
    
    yyparse();
    
//    for(int i = 0; i < nodecount; i++) {
//        char buffer[15];
//        sprintf(buffer, "reached(%d)", i);
//        char* tb = strdup(buffer);
//        int id = Vocabulary::instance().queryAtom(tb);
//        printf("%s\n", Vocabulary::instance().getAtom(id));
//        U.insert(id);
//        delete tb;
//    }
//    
    for(int i = 0; i < G_NLP.size(); i++) {
        if(G_NLP[i].type != FACT) break;
        char* tmp;
        char s[15];
        int left, right;
        int h = G_NLP[i].head;
        tmp = Vocabulary::instance().getAtom(h);
        sscanf(tmp, "%[^(](%d,%d)", s, &left, &right);
  
        if(strcmp(s, "arc") == 0) {
            if(left < nodecount/2 && right >= nodecount/2 || right < nodecount/2 && left >= nodecount/2) {
                char hc[15];
                sprintf(hc, "hc(%d,%d)", left, right);
                int id = Vocabulary::instance().queryAtom(hc);
                U.insert(id);
            }
        }
        
    }
    DependenceGraph dpg;
    dpg.application(result);
    fprintf(result, "#hide .");
    fclose(yyin);
    
    return 0;
}