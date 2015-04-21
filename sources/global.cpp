#include "structs.h"
#include <cstdio>
#include <vector>
#include "Rule.h"
using namespace std;

vector<Rule> G_NLP;
set<int> U;
int *atomState;
int *ruleState;