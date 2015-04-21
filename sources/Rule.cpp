#include "Rule.h"
#include "structs.h"
#include "Vocabulary.h"
#include <assert.h>
#include <cstdlib>
#include <string>
#include <cstring>
#include <algorithm>

Rule::Rule() {
    head = -1;
    body_length = 0;
    body_lits.clear();
    type = RULE;
}

Rule::Rule(_rule* r) : 
        head(r->head), type(r->type), body_length(r->length) {
    for(int i = 0; i < (r->length); i++) {
        body_lits.insert(r->body[i]);
    }
}
Rule::Rule(const Rule& _rhs) : 
        head(_rhs.head),
        type(_rhs.type),
        body_lits(_rhs.body_lits),
        body_length(_rhs.body_length) {
}
Rule::~Rule() {
    body_lits.clear();
}
Rule& Rule::operator = (const Rule& _rhs) {
    head = _rhs.head;
    type = _rhs.type;
    body_length = _rhs.body_length;
    body_lits = _rhs.body_lits;
    return *this;
}

bool Rule::operator == (const Rule& _rhs) {
    if(type != _rhs.type || head != _rhs.head || body_length != _rhs.body_length)
        return false;
    else {
        if(!(body_lits.size() == _rhs.body_lits.size() && 
                includes(_rhs.body_lits.begin(), _rhs.body_lits.end(), body_lits.begin(), body_lits.end())))
            return false;
    }
    return true;
}

void Rule::output(FILE* _out) const {
    if(type == FACT) {
        if(head > 0 && strlen(Vocabulary::instance().getAtom(head)) > 0)
            fprintf(_out, "%s.\n", Vocabulary::instance().getAtom(head));
    }
    fflush(_out);
    if(type != FACT) {
        if(head > 0)
             fprintf(_out, "%s", Vocabulary::instance().getAtom(head));
        fprintf(_out, " :- ");
        for(set<int>::iterator pit = body_lits.begin(); pit != 
                body_lits.end(); pit++) {
            if(*pit < 0) fprintf(_out, "not ");
            int id = (*pit < 0) ? (-1 * (*pit)) : *pit;
            fprintf(_out, "%s", Vocabulary::instance().getAtom(id));
            if(pit != (--body_lits.end())) {
                fprintf(_out, ",");
            }
        }
        fprintf(_out, ".\n");
    }
    
    
}