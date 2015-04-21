%{
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include "Vocabulary.h"
#include "Rule.h"

extern "C" {
	void yyerror(const char *s);
	extern int yylex(void);
}
 
extern vector<Rule> G_NLP;

int id;

void yyerror(const char* s)
{
	printf("Parser error: %s\n", s);
}

%}

%union {
    char* s;
    int i;
    struct __literals* l;
    struct __rule* r;
}

%token <s> S_ATOM
%token <s> S_NEGA
%token <s> S_IMPL
%token <s> LPAREN
%token <s> RPAREN
%token <s> COMMA
%token <s> PERIOD

%type <s> term terms atom
%type <i> literal
%type <l> literals
%type <r> rule

%left S_IMPL

%%
nlp 
    : rules {
        printf("nlp\n");
    }
    |
;

rules
    : rules rule {
        Rule rule($2);
        G_NLP.push_back(rule);
    }
    | rule {
        Rule rule($1);
        G_NLP.push_back(rule);
    }
;

rule 
    : literal PERIOD{
        $$ = (_rule*)malloc(sizeof(_rule));
        $$->head = $1;
        $$->length = 0;
        $$->type = FACT;
    }
    | literal S_IMPL literals PERIOD{
        $$ = (_rule*)malloc(sizeof(_rule));
        $$->head = $1;
        $$->type = RULE;
        for(int i = 0; i < ($3->length); i++) {
            $$->body[i] = $3->atoms[i];
        }
        $$->length = $3->length;
    }
    | S_IMPL literals PERIOD{
        $$ = (_rule*)malloc(sizeof(_rule));
        $$->head = -1;
        $$->type = CONSTRANT;
        for(int i = 0; i < ($2->length); i++) {
            $$->body[i] = $2->atoms[i];
        }
        $$->length = $2->length;
    }
;

literals
    : literals COMMA literal {
        $1->atoms[$1->length] = $3;
        $1->length++;
    }
    | literal {
        $$ = (__literals*)malloc(sizeof(_literals));
        memset($$->atoms, 0, sizeof(int) * MAX_ATOM_LENGTH);
        
        $$->atoms[0] = $1;
        $$->length = 1;
    }
;

literal
    : S_NEGA atom {
        int id = Vocabulary::instance().queryAtom($2);
        if(id < 0)
            id = Vocabulary::instance().addAtom($2);

        $$ = -1 * id;
    }
    | atom {
        int id = Vocabulary::instance().queryAtom($1);
        if(id < 0)
            id = Vocabulary::instance().addAtom($1);

        $$ = id;
    }
;

atom
    : S_ATOM LPAREN terms RPAREN {
        char str_buff[512];
        
        sprintf(str_buff, "%s(%s)", $1, $3);
        $$ = strdup(str_buff);
    } 
    | S_ATOM {
        $$ = strdup($1);
    }
;

terms
    : terms COMMA term {
        char str_buff[512];
        
        sprintf(str_buff, "%s,%s", $1, $3);
        $$ = strdup(str_buff);
    }
    | term {
        $$ = strdup($1);
    }
;

term
    : S_ATOM {
        $$ = strdup($1);
    }
;
%%
