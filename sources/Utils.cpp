 #include "Utils.h"
#include "Rule.h"
#include <assert.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <algorithm>

using namespace std;

/**
 * 使用联接词(非、析取、合取、蕴含)生成公式
 * @param _formulaType 联接词
 * @param _subformulaL 左公式
 * @param _subformulaR 右公式
 * @return 新公式
 */
_formula* Utils::compositeByConnective(FORMULA_TYPE _formulaType, 
                    _formula* _subformulaL, _formula* _subformulaR) {
    assert(_formulaType == NEGA || _formulaType == DISJ ||
		   _formulaType == CONJ || _formulaType == IMPL);

    _formula* fml = (_formula*)malloc(sizeof(_formula));
    assert(fml);

    fml->formula_type = _formulaType;
    fml->subformula_l = _subformulaL;
    fml->subformula_r = _subformulaR;
    return fml;
}
/**
 * 生成atom
 * @param _formulaType
 * @param _predicateId
 * @param _parameters
 * @return 
 */
_formula* Utils::compositeToAtom(int _atom_id) {
    _formula* fml = (_formula*)malloc(sizeof(_formula));
    assert(fml);

    fml->formula_type = ATOM;
    fml->predicate_id = _atom_id;
    return fml;
}

_formula* Utils::copyFormula(const _formula* _fml) {
    if (_fml == NULL) {
        return NULL;
    }
	
    _formula* newFormula = (_formula*)malloc(sizeof(_formula));
    assert (newFormula);

    memcpy(newFormula, _fml, sizeof(_formula));
    switch (_fml->formula_type)
    {
    case ATOM:
        newFormula->predicate_id = _fml->predicate_id;
        break;
    case CONJ:
    case DISJ:
    case IMPL:
        assert(_fml->subformula_r);
        newFormula->subformula_r = copyFormula( _fml->subformula_r);
    case NEGA:
    case UNIV:
    case EXIS:
        assert(_fml->subformula_l);
        newFormula->subformula_l = copyFormula(_fml->subformula_l);
        break;
    default:
        assert (0);
    }

    return newFormula;
}

void Utils::deleteFormula(_formula* _fml) {
    assert(_fml);

    switch (_fml->formula_type)
    {
    case ATOM:
        break;
    case CONJ:
    case DISJ:
    case IMPL:
        assert(_fml->subformula_r);
        deleteFormula(_fml->subformula_r);
    case NEGA:
    case UNIV:
    case EXIS:
        assert(_fml->subformula_l);
        deleteFormula(_fml->subformula_l);
        break;
    default:
        assert ( 0 );
    }

    free(_fml);
}

bool Utils::inSet(int tag, set<int> list) {
    if(list.find(tag) != list.end()) {
        return true;
    }
    
    return false;
}

bool Utils::crossSet(set<int> l1, set<int> l2) {
    for(set<int>::iterator it = l1.begin(); it != l1.end(); it++) {
        if(l2.find(*it) != l2.end()) return true;
    }
    
    return false;
}

void Utils::joinFormulas(vector<_formula*>& des_list, vector<_formula*> join_list) {
    for(int i = 0; i < join_list.size(); i++) {
        des_list.push_back(join_list.at(i));
    }
}

vector< set<int> > Utils::convertToSATInput(vector<_formula*> cnfDlp) {
    vector< set<int> > res;
    for(vector<_formula*>::iterator it = cnfDlp.begin(); it != cnfDlp.end(); it++) {
        set<int> lits;
        convertCNFformulaToLits(*it, lits);
        res.push_back(lits);
        
        deleteFormula(*it);
    }
    return res;
}

void Utils::convertCNFformulaToLits(_formula* rule, set<int>& lits) {
    if(rule->formula_type == ATOM) {
        lits.insert(rule->predicate_id);
    }
    else if(rule->formula_type == NEGA) {
        lits.insert(-1 * (rule->subformula_l->predicate_id));
    }
    else {
        convertCNFformulaToLits(rule->subformula_l, lits);
        convertCNFformulaToLits(rule->subformula_r, lits);
    }
}

//_formula* Utils::convertRuleBodyToFormula(const Rule& rule) {
//    _formula* fml = NULL;
//    
//    Rule _rule(rule);
//    
//    if(rule.type == FACT) return NULL;
//    
//    for(set<int>::iterator it = _rule.positive_literals.begin(); it != 
//            _rule.positive_literals.end(); it++) {
//        if(fml == NULL) fml = Utils::compositeToAtom(*it);
//        else {
//            fml = Utils::compositeByConnective(CONJ, fml, Utils::compositeToAtom(*it));
//        }
//    }
//    for(set<int>::iterator it = _rule.negative_literals.begin(); it !=
//            _rule.negative_literals.end(); it++) {
//        _formula* nega = Utils::compositeByConnective(NEGA, Utils::compositeToAtom(*it));
//        if(fml == NULL) fml = nega;
//        else {
//            fml = Utils::compositeByConnective(CONJ, fml, nega);
//        }
//    }
//    
//    return fml;
//}

vector< vector< vector<char*> > > Utils::readClaspAnswers(const char* AnswerSet_list) {
    FILE* asl = fopen(AnswerSet_list, "r");
    vector< vector< vector<char*> > > claspAnswers;
    int max_line = 1000;
    
    while(!feof(asl)) {
        char as[max_line];
        fgets(as, max_line, asl);     
        int asi = 0;
        while(as[asi] != '\n')
            asi++;
        as[asi] = '\0';
        
        claspAnswers.push_back(readClaspAnswer(as));        
    }
    fclose(asl);
    
    return claspAnswers;
}

vector< vector<char*> > Utils::readClaspAnswer(const char* answer) {
    vector< vector<char*> > model_answer;
    FILE* fas = fopen(answer, "r");
    if(fas == NULL) cout << "Open as failed.\n";
    
    char answer_match[] = "Answer";
    int index = 0;
    
    while(!feof(fas)) {
        char c = fgetc(fas);

        if(c == answer_match[index]) {
            index++;
        }
        else {
            index = 0;
        }
        
        if(answer_match[index] == '\0') {
            vector<char*> model;
            
            while(fgetc(fas) != '\n');
            
            while(true) {
                char m[MAX_ATOM_LENGTH];
                int mi = 0;
                char tmp = fgetc(fas);
                
                if(tmp == '\n') break;
                
                while(tmp != ' ' && tmp != '\n') {
                    m[mi] = tmp;
                    mi++;
                    tmp = fgetc(fas);
                }
                m[mi] = '\0';
                
                model.push_back(strdup(m));
                
                if(tmp == '\n') break;
            }
            index = 0;
            model_answer.push_back(model);
        }       
    }

    fclose(fas);  
    
    return model_answer;
}

void Utils::formulaOutput(FILE* out, const _formula* fml) {
    assert(fml);
    
    _formula* output = copyFormula(fml);
    
    switch(output->formula_type) {
        case ATOM:
            fprintf(out, "%s", Vocabulary::instance().getAtom(output->predicate_id));
            break;
        case CONJ:
            assert(output->subformula_l);
            fprintf(out, "(");
            formulaOutput(out, output->subformula_l);
            fprintf(out, " & ");
            assert(output->subformula_r);
            formulaOutput(out, output->subformula_r);
            fprintf(out, ")");
            break;
        case DISJ:
            assert(output->subformula_l);
            fprintf(out, "(");
            formulaOutput(out, output->subformula_l);
            fprintf(out, " | ");
            assert(output->subformula_r);
            formulaOutput(out, output->subformula_r);
            fprintf(out, ")");
            break;
        case IMPL:
            assert(output->subformula_l);
            fprintf(out, "(");
            formulaOutput(out, output->subformula_l);
            fprintf(out, " -> ");
            assert(output->subformula_r);
            formulaOutput(out, output->subformula_r);
            fprintf(out, ")");
            break;
        case NEGA:   
            fprintf(out, " not (");
            assert(output->subformula_l);
            formulaOutput(out, output->subformula_l);
            fprintf(out, ")");
            break;
        case UNIV:
        case EXIS:
            assert(output->subformula_l);
            formulaOutput(out, output->subformula_l);
            break;
        default:
            assert(0);
    }
    
    free(output);
}

bool Utils::charCmp(char* a, char* b) {
    if(strcmp(a, b) < 0) return true;
    else return false;
}

bool Utils::compareSingleModel(vector<char*>& claspModel, set<int>& satModel) {
    vector<char*> satCharModel;
    
    if(satModel.size() != claspModel.size()) return false;
    
    for(set<int>::iterator it = satModel.begin(); it != satModel.end(); it++) {
        satCharModel.push_back(Vocabulary::instance().getAtom(*it));
    }
    
    sort(claspModel.begin(), claspModel.end(), charCmp);
    sort(satCharModel.begin(), satCharModel.end(), charCmp);
    
    for(int i = 0; i < satModel.size(); i++) {
        if(strcmp(satCharModel.at(i), claspModel.at(i)) != 0)
            return false;
    }
    
    return true;
}

bool Utils::compareAnswerSet(vector< vector<char*> >& claspAnswer, vector< set<int> >& satAnswer) {
    if(claspAnswer.size() != satAnswer.size()) return false;
    
    for(vector< vector<char*> >::iterator c_it = claspAnswer.begin(); c_it != claspAnswer.end(); 
            c_it++) {
        bool exis = false;
        for(vector< set<int> >::iterator s_it = satAnswer.begin(); s_it != satAnswer.end();
                s_it++) {
            if(compareSingleModel(*c_it, *s_it)) {
                exis = true;
                break;
            }
        }
        if(!exis) return false;
    }
    
    return true;
}

bool Utils::setContains(const set<int>& a, const set<int>& b) {
    set<int>::iterator ait = a.begin();
    set<int>::iterator bit = b.begin();
    
    while(bit != b.end()) {
        if(*bit == *ait) {
            ait++;
            bit++;
        }
        else if(*bit > *ait) {
            ait++;
        }
        else {
            return false;
        }
    }
    
    return true;
}


//void Utils::createPealGraph(int PEALNUM, int PEALSIZE) {
//    FILE* fout; 
//    FILE* fcuts;
//    fout = fopen("IO/pealGraph/nv14a2.txt", "w");
//    if(!fout) {
//        printf("Fail to open output file.\n");
//        assert(0);
//    }
//
//    fcuts = fopen("IO/pealGraph/cuts.txt", "w");
//    if(!fcuts) {
//        printf("Fail to open cuts file.\n");
//        assert(0);
//    }
//
//    srand(time(0));
//    int total = PEALNUM*PEALSIZE;
//    bool visited[PEALNUM*PEALSIZE];
//    memset(visited, false, sizeof(visited));
//
//    // print all vertexs in file
//    for(int i = 0; i < total; i++) {
//        fprintf(fout, "vertex(%d).\n", i);      fflush(fout);
//    }
//    visited[0] = true;  // while visited[i] = true, vertex[i] can not be choose as a connect node
//
//    // print all arcs in the each peal Test
//    for(int t = 0; t < PEALNUM; t++)
//        for(int i = 0; i< PEALSIZE; i++)
//            for(int j = i+1; j< PEALSIZE; j++) {
//                fprintf(fout, "arc(%d, %d).\n", i+t*PEALSIZE, j+t*PEALSIZE);    fflush(fout);
//                fprintf(fout, "arc(%d, %d).\n", j+t*PEALSIZE, i+t*PEALSIZE);    fflush(fout);
//            }
//
//    // generate connect edge for every two nodes
//    set<int> cuts;              // hold all cut vertexs             
//    int st = -1, ed = -1;
//
//    // first node to second node
//    do {
//        st = rand() % PEALSIZE;
//    }while(visited[st]);
//    visited[st] = true;     cuts.insert(st);
//    do {
//        ed = rand() % PEALSIZE + PEALSIZE;
//    }while(visited[ed]);
//    visited[ed] = true;     cuts.insert(ed);
//    fprintf(fout, "arc(%d, %d).\n", st, ed);    fflush(fout);
//
//    // second node to first node
//    do {
//        st = rand() % PEALSIZE + PEALSIZE;
//    }while(visited[st]);
//    visited[st] = true;     cuts.insert(st);
//    do {
//        ed = rand() % PEALSIZE;
//    }while(visited[ed]);
//    visited[ed] = true;     cuts.insert(ed);
//    fprintf(fout, "arc(%d, %d).\n", st, ed);    fflush(fout);
//
//    // print out total line num to judge whether a correct output
//    // int lines = ((PEALSIZE-1)*PEALSIZE) * PEALNUM + PEALNUM + 2*PEALSIZE;
//    // fprintf(fout, "Total lines should be %d\n", lines);      fflush(fout);
//
//    // print out all cut vertexs with reached(i)
//    for(set<int>::iterator it = cuts.begin(); it != cuts.end(); it++) {
//        fprintf(fcuts, "reached(%d)\n", *it);   fflush(fout);
//    }
//
//    fclose(fcuts);
//    fclose(fout);
//
//
//    // then call gringo to ground the hc.lp with output file created above.
//    // 管道调用 RUN_CMD 计算并将其结果写进 OUTPUT_FILE
//    int BUFF_SIZE = 1024;
//    string OUTPUT_FILE = "IO/input/nv14a2.in";
//    string cmd = "gringo -t IO/pealGraph/hc.lp IO/pealGraph/nv14a2.txt > " + OUTPUT_FILE;
//    char buff[BUFF_SIZE];
//    FILE *pipe_file = popen(cmd.c_str(), "r");
//    FILE *output_file = fopen(OUTPUT_FILE.c_str(), "w");
//    while (fgets(buff, BUFF_SIZE, pipe_file)) {
//        fprintf(output_file, "%s", buff);
//    }
//    pclose(pipe_file);
//    fclose(output_file);
//}