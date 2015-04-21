#include "DependenceGraph.h"
#include <iostream>
#include <cstdio>
#include "Utils.h"
#include <cstring>
#include <assert.h>
#include <vector>
#include "Rule.h"
#include <map>
#include "structs.h"
#include "Vocabulary.h"
#include <string.h>
#include <set>
#include <algorithm>
#include <functional>
#include <sstream>
#include <ctime>
#include <bits/stdc++.h>

using namespace std;

#define OUTPUT_FILE "IO/output.txt"
#define BUFF_SIZE 1024

extern vector<Rule> G_NLP;
extern set<int> U;
extern int *atomState;
extern int *ruleState;
extern FILE* count_out;

DependenceGraph::DependenceGraph() {
    set<int> nodeSet;
    
    for(int i = 0; i < G_NLP.size(); i++) {
        Rule r = G_NLP[i];
        if(r.type == RULE) {
            nodeSet.insert(r.head);
            dpdRules[r.head].insert(i);
            for(set<int>::iterator p_it = r.body_lits.begin(); p_it != r.body_lits.end(); p_it++) {
                if(*p_it > 0) {
                    // 构造原程序的有向正依赖图
                    dpdGraph[r.head].insert(*p_it);
                    nodeSet.insert(*p_it);
                    // 构造原程序的无向正依赖图
                    udpdGraph[r.head].insert(*p_it);
                    udpdGraph[*p_it].insert(r.head);
                }
            }
        }
    }
    
    nodeNumber = nodeSet.size();
    maxNode = *(--nodeSet.end());
    visit = new bool[maxNode + 1];
    memset(visit, false, sizeof(bool) * (maxNode + 1));
    DFN = new int[maxNode + 1];
    memset(DFN, 0, sizeof(int) * (maxNode + 1));
    Low = new int[maxNode + 1];
    memset(Low, 0, sizeof(int) * (maxNode + 1));
    involved = new bool[maxNode + 1];
    memset(involved, true, sizeof(bool) * (maxNode + 1));
    
    apsize = Vocabulary::instance().apSize();
    m_pealSize = 0;
}

DependenceGraph::~DependenceGraph() {
    dpdGraph.clear();
    dpdRules.clear();
    delete[] visit;
    delete[] DFN;
    delete[] Low;
}

vector<Loop> DependenceGraph::findSCC() {
    vector<Loop> loops;
    memset(visit, false, sizeof(bool) * (maxNode + 1));

    for(map<int, set<int> >::iterator it = dpdGraph.begin(); it != dpdGraph.end(); it++) {
        if(!visit[it->first] && involved[it->first]) {
            Index = 0;
            tarjan(it->first, loops);
        }
    }
    
    return loops;
}

void DependenceGraph::findSCCInSCC(vector<Loop>& loops) {
    memset(visit, false, sizeof(bool) * (maxNode + 1));    
    
    for(map<int, set<int> >::iterator it = dpdGraph.begin(); it != dpdGraph.end(); it++) {
        if(!visit[it->first] && involved[it->first]) {
            Index = 0;
            tarjan(it->first, loops);
        }
    }  
//    Index = 0;
//    tarjan(u, loops);
}

void DependenceGraph::tarjan(int u, vector<Loop>& loops) {
    DFN[u] = Low[u] = ++Index;
    vs.push(u);
    visit[u] = true;
    for(set<int>::iterator it = dpdGraph[u].begin(); it != dpdGraph[u].end(); it++) {
        if(!involved[*it]) continue;
        
        if(!visit[*it]) {
            tarjan(*it, loops);
            if(Low[u] > Low[*it]) Low[u] = Low[*it];
        }
        else {
            if(Low[u] > DFN[*it]) Low[u] = DFN[*it];
        }
    }
    if(Low[u] == DFN[u]) {
        if(vs.top() != u) {
            Loop l;
            while(vs.top() != u) {                  
                l.loopNodes.insert(vs.top());
                vs.pop();
            }
            l.loopNodes.insert(u);
            findESRules(l);
            vs.pop();
            loops.push_back(l);
        }
        else {
            vs.pop();
        }
    }
}

void DependenceGraph::findESRules(Loop& loop) {
    for(set<int>::iterator it = loop.loopNodes.begin(); it != loop.loopNodes.end();
            it++) {
        map<int, set<int> >::iterator dit = dpdRules.find(*it);
        if(dit != dpdRules.end()) {
            for(set<int>::iterator rit = dit->second.begin(); rit != dit->second.end();
                    rit++) {
                bool in = false;
                for(set<int>::iterator ait = G_NLP[*rit].body_lits.begin(); ait != G_NLP[*rit].body_lits.end(); ait++) {
                    if(*ait > 0) {
                        if(loop.loopNodes.find(*ait) != loop.loopNodes.end()) {
                            in = true;
                            break;
                        }
                    }
                }
                if(!in) {
                    loop.ESRules.insert(*rit);
                }
            }
        }
    }
}

int DependenceGraph::computeLoopFormulas(Loop& loop) {
    int newVar = 0;
    set<int> lf;
        
    for(set<int>::iterator hit = loop.loopNodes.begin(); hit != loop.loopNodes.end();
            hit++) {
        lf.insert(-1 * (*hit));
    }
    
    for(set<int>::iterator rit = loop.ESRules.begin(); rit != loop.ESRules.end();
            rit++) {
        Rule rule = G_NLP[*rit];
        
        if(rule.body_length <= 1) {
            lf.insert(*(rule.body_lits.begin()));          
        }
        else {
            int id = ruleState[*rit];
            if(id == 0) {
                char newAtom[MAX_ATOM_LENGTH];
                sprintf(newAtom, "Rule_%d", *rit);
                id = Vocabulary::instance().addAtom(strdup(newAtom));
                ruleState[*rit] = id;
                
                newVar++;
                set<int> rightEqual;                                          
                rightEqual.insert(-1 * id);

                for(set<int>::iterator eit = G_NLP[*rit].body_lits.begin(); 
                        eit != G_NLP[*rit].body_lits.end(); eit++) {
                    set<int> leftEqual;
                    leftEqual.insert(id);
                    leftEqual.insert(-1 * (*eit));
                    loop.loopFormulas.push_back(leftEqual);
                    rightEqual.insert(*eit);
                }
                
                loop.loopFormulas.push_back(rightEqual);
            }
            lf.insert(id);
        }
    }

    loop.loopFormulas.push_back(lf); 
    
    return newVar;
} 

vector<Loop> DependenceGraph::findCompMaximal(set<int> comp, int t) {
    vector<Loop> maximals;
    
    memset(involved, false, sizeof(bool) * (maxNode + 1));
    for(set<int>::iterator it = comp.begin(); it != comp.end(); it++) {
        involved[*it] = true;
    }
    vector<Loop> sccs = findSCC();
    
    if(t == 0) {
        return sccs;
    }
    else {
        for(vector<Loop>::iterator it = sccs.begin(); it != sccs.end(); it++) {
          //  printf("\n\nBLOCK\n");
          //  findESRules(*it); 
          //  it->print();
          //  Loop l = findLoopMaximal(*it);
          //  findESRules(l);
          //  maximals.push_back(l);
          //  l.print();
            if(t == 1) maximals.push_back(findProperLoop(*it));
            if(t == 2) maximals.push_back(findElememtary(*it));
        }

        return maximals;
    }
   // return sccs;
}

Loop DependenceGraph::searchElementary(Loop scc) {
    vector<Loop> sccs;
    
    memset(involved, false, sizeof(bool) * (maxNode + 1));
    for(set<int>::iterator lit = scc.loopNodes.begin(); lit != scc.loopNodes.end(); lit++) {
        involved[*lit] = true;      
    }
    int r = G_NLP[*(++scc.ESRules.begin())].head;
    for(set<int>::iterator it = scc.ESRules.begin(); it != scc.ESRules.end(); it++) {
        involved[r] = true;
        involved[G_NLP[*it].head] = false;
        findSCCInSCC(sccs);
        r = G_NLP[*it].head;   
        
        for(int i = 0; i < sccs.size(); i++) {
            set<int> m;
            Loop c = sccs[i];
            setRelation(c.ESRules, scc.ESRules, m);
            if(m.size() == 0) return c;
            else {
                memset(involved, false, sizeof(bool) * (maxNode + 1));
                //int u = -1;
                for(set<int>::iterator lit = c.loopNodes.begin(); lit != c.loopNodes.end(); lit++) {
                   // if(u == -1) u = (m.find(*lit) == m.end()) ? *lit : -1; 
                    involved[*lit] = true;      
                } 
                for(set<int>::iterator mit = m.begin(); mit != m.end(); mit++) {
                    involved[G_NLP[*mit].head] = false;      
                }

                findSCCInSCC(sccs);
            }
        }
    }
    
    return scc;
}

Loop DependenceGraph::searchProperLoop(Loop scc) {
    if(scc.ESRules.size() == 0) return scc;
    
    memset(involved, true, sizeof(bool) * (maxNode + 1));
    
    vector<Loop> sccs;
    findSCCInSCC(sccs); 
    
    for(int i = 0; i < sccs.size(); i++) {
        set<int> m;                       
        Loop c = sccs[i];
        int rel = setRelation(c.ESRules, scc.ESRules, m);
        
        if(rel == 1) return c;
        if(rel == 0) {
            memset(involved, false, sizeof(bool) * (maxNode + 1));
            for(set<int>::iterator lit = c.loopNodes.begin(); 
                    lit != c.loopNodes.end(); lit++) {
                involved[*lit] = true;      
            }
            int r = G_NLP[*(++c.ESRules.begin())].head;
            for(set<int>::iterator it = c.ESRules.begin(); it != c.ESRules.end(); it++) {
                involved[r] = true;
                involved[G_NLP[*it].head] = false;
                findSCCInSCC(sccs);
                r = G_NLP[*it].head;                             
            }
        }
        if(rel == -1) {
            memset(involved, false, sizeof(bool) * (maxNode + 1));
            int u = -1;
            for(set<int>::iterator lit = c.loopNodes.begin(); lit != c.loopNodes.end(); lit++) {
                if(u == -1) u = (m.find(*lit) == m.end()) ? *lit : -1; 
                involved[*lit] = true;      
            } 
            for(set<int>::iterator mit = m.begin(); mit != m.end(); mit++) {
                involved[G_NLP[*mit].head] = false;      
            }

            findSCCInSCC(sccs);
        }
    }
    return scc;
}

Loop DependenceGraph::searchProperLoop(Loop scc, set<int> in) {
    if(scc.ESRules.size() == 0) return scc;

    memset(involved, false, sizeof(bool) * (maxNode + 1));
    for(set<int>::iterator sit = in.begin(); sit != in.end(); sit++) involved[*sit] = true;
    
    vector<Loop> sccs;
    findSCCInSCC(sccs); 
    
    for(int i = 0; i < sccs.size(); i++) {
        set<int> m;                       
        Loop c = sccs[i];
        int rel = setRelation(c.ESRules, scc.ESRules, m);
        
        if(rel == 1) return c;
        if(rel == 0) {
            memset(involved, false, sizeof(bool) * (maxNode + 1));
            for(set<int>::iterator lit = c.loopNodes.begin(); 
                    lit != c.loopNodes.end(); lit++) {
                involved[*lit] = true;      
            }
            int r = G_NLP[*(++c.ESRules.begin())].head;
            for(set<int>::iterator it = c.ESRules.begin(); it != c.ESRules.end(); it++) {
                involved[r] = true;
                involved[G_NLP[*it].head] = false;
                findSCCInSCC(sccs);
                r = G_NLP[*it].head;                             
            }
        }
        if(rel == -1) {
            memset(involved, false, sizeof(bool) * (maxNode + 1));
            int u = -1;
            for(set<int>::iterator lit = c.loopNodes.begin(); lit != c.loopNodes.end(); lit++) {
                if(u == -1) u = (m.find(*lit) == m.end()) ? *lit : -1; 
                involved[*lit] = true;      
            } 
            for(set<int>::iterator mit = m.begin(); mit != m.end(); mit++) {
                involved[G_NLP[*mit].head] = false;      
            }

            findSCCInSCC(sccs);
        }
    }
    return scc;
}

vector<Loop> DependenceGraph::findAllProperLoops() {
    secHash.clear();
    loopHash.clear();
    vector<Loop> ploops;
    set<int> s;
    properTimes = 0;
    findProperLoops(ploops, s);
    
    return ploops;
}

vector<Loop> DependenceGraph::findAllElementaryLoops() {
    secHash.clear();
    loopHash.clear();
    vector<Loop> eloops;
    set<int> s;
    properTimes = 0;
    findElementaryLoops(eloops, s);
    
    return eloops;
}

void DependenceGraph::findElementaryLoops(vector<Loop>& eloops, set<int>& s) {
    if(s.size() == 0) {
        memset(involved, true, sizeof(bool) * (maxNode + 1));  
    }
    else {
        memset(involved, false, sizeof(bool) * (maxNode + 1));
        for(set<int>::iterator sit = s.begin(); sit != s.end(); sit++) involved[*sit] = true;
    }
    
    vector<Loop> sccs = findSCC();
    
    for(int i = 0; i < sccs.size(); i++) {
        Loop tc = sccs[i];
        int hash = Hash(tc.loopNodes);
        if(!loopHash[hash]) {
            loopHash[hash] = true;  
            Loop c = searchElementary(tc);
            if(c == tc) eloops.push_back(c);
            properTimes++;
            findElSection(eloops, tc.loopNodes);
        }
    }
}

void DependenceGraph::findProperLoops(vector<Loop>& ploops, set<int>& s) {
    if(s.size() == 0) {
        memset(involved, true, sizeof(bool) * (maxNode + 1));  
    }
    else {
        memset(involved, false, sizeof(bool) * (maxNode + 1));
        for(set<int>::iterator sit = s.begin(); sit != s.end(); sit++) involved[*sit] = true;
    }
    
    vector<Loop> sccs = findSCC();
    
    for(int i = 0; i < sccs.size(); i++) {
        Loop tc = sccs[i];
        int hash = Hash(tc.loopNodes);
        if(!loopHash[hash]) {
            loopHash[hash] = true;    
             Loop c = searchProperLoop(tc);
             properTimes++;
             if(c == tc) {
                 ploops.push_back(c);                
                 for(set<int>::iterator it = tc.ESRules.begin(); it != tc.ESRules.end(); it++) {
                     set<int> s = tc.loopNodes;
                     s.erase(G_NLP[*it].head);
                     findProperSection(ploops, s, true);
                 }
             }
             else {
                 findProperSection(ploops, tc.loopNodes, false);
             }
        }
    }
}

void DependenceGraph::findElSection(vector<Loop>& ploops, set<int>& s) {
    if(s.size() <= 2) return;
    
    for(set<int>::iterator it = s.begin(); it != s.end(); it++) {
        set<int> ps;
        int hash = sectionHash(s, ps, *it);
        
        if(!secHash[hash]) {
            secHash[hash] = true;
            findElementaryLoops(ploops, ps);
        }
    }
}

void DependenceGraph::findProperSection(vector<Loop>& ploops, vector<Loop>& cloops, set<int>& cns, set<int>& s, set<int>& in, bool self) {
    if(self) {
        int hash = Hash(s);
        if(!secHash[hash]) {
            secHash[hash] = true;
            findProperLoopsCuts(ploops, cloops, s, cns, in);
        }
    }
    
    if(s.size() <= 2) return;
    
    for(set<int>::iterator it = s.begin(); it != s.end(); it++) {
        set<int> ps;
        int hash = sectionHash(s, ps, *it);
        
        if(!secHash[hash]) {
            secHash[hash] = true;
            findProperLoopsCuts(ploops, cloops, ps, cns, in);
        }
    }
}

void DependenceGraph::findProperSection(vector<Loop>& ploops, set<int>& s, bool self) {
    if(self) {
        int hash = Hash(s);
        if(!secHash[hash]) {
            secHash[hash] = true;
            findProperLoops(ploops, s);
        }
    }
    
    if(s.size() <= 2) return;
    
    for(set<int>::iterator it = s.begin(); it != s.end(); it++) {
        set<int> ps;
        int hash = sectionHash(s, ps, *it);
        
        if(!secHash[hash]) {
            secHash[hash] = true;
            findProperLoops(ploops, ps);
        }
    }
}

int DependenceGraph::Hash(set<int>& s1) {
    int hash = 0;
    
    for(set<int>::iterator it = s1.begin(); it != s1.end(); it++) {
        hash += (*it) * (*it) * (*it);
    }
    
    return hash;
}

int DependenceGraph::sectionHash(set<int>& s1, set<int>& s2, int m) {
    int hash = 0;
    
    for(set<int>::iterator it = s1.begin(); it != s1.end(); it++) {
        if(*it != m) {
            s2.insert(*it);
            hash += (*it) * (*it) * (*it);
        }
    }
    
    return hash;
}
//For logic program that there is no loop without external support 
Loop DependenceGraph::findElememtary(Loop scc) {
   if(scc.ESRules.size() == 0) return scc;
   memset(involved, false, sizeof(bool) * (maxNode + 1));
   for(set<int>::iterator it = scc.loopNodes.begin(); it != scc.loopNodes.end(); it++) {      
       involved[*it] = true;
   }
   int r = -1;
   for(set<int>::iterator nit = scc.loopNodes.begin(); nit != scc.loopNodes.end(); nit++) {
       if(r != -1) involved[r] = true;
       involved[*nit] = false;
       r = *nit;
       vector<Loop> sccs = findSCC();
       
       for(int i = 0; i < sccs.size(); i++) {
           set<int> moreoverRules;
           for(set<int>::iterator eit = sccs[i].ESRules.begin(); eit != sccs[i].ESRules.end(); eit++) {
               if(scc.ESRules.find(*eit) == scc.ESRules.end()) {
                   moreoverRules.insert(*eit);
               }
           }
           if(moreoverRules.size() == 0) return findElememtary(sccs[i]);
           else {
               memset(involved, false, sizeof(bool) * (maxNode + 1));
               for(set<int>::iterator cit = sccs[i].loopNodes.begin(); cit != sccs[i].loopNodes.end(); cit++) {
                   involved[*cit] = true;
               }
               for(set<int>::iterator mit = moreoverRules.begin(); mit != moreoverRules.end(); mit++) {
                   involved[G_NLP[*mit].head] = false;
               }
               vector<Loop> tscc = findSCC();
               for(int ts = 0; ts < tscc.size(); ts++) sccs.push_back(tscc[ts]); 
           }
       }
   }
   
   return scc;
}

//For logic program that there is no loop without external support 
Loop DependenceGraph::findProperLoop(Loop scc) {      
    if(scc.ESRules.size() == 1) return scc;   
    
    memset(involved, true, sizeof(bool) * (maxNode + 1));
    vector<Loop> sccs;
    findSCCInSCC(sccs);
    
    for(int i = 0; i < sccs.size(); i++) {
        set<int> m;                       
        Loop c = sccs[i];
        int rel = setRelation(c.ESRules, scc.ESRules, m);
        
        if(rel == 1) return findProperLoop(c);
        if(rel == 0) {
            memset(involved, false, sizeof(bool) * (maxNode + 1));
            for(set<int>::iterator lit = c.loopNodes.begin(); 
                    lit != c.loopNodes.end(); lit++) {
                involved[*lit] = true;      
            }
            int r = G_NLP[*(++c.ESRules.begin())].head;
            for(set<int>::iterator it = c.ESRules.begin(); it != c.ESRules.end(); it++) {
                involved[r] = true;
                involved[G_NLP[*it].head] = false;
                findSCCInSCC(sccs);
                r = G_NLP[*it].head;                             
            }
        }
        if(rel == -1) {
            memset(involved, false, sizeof(bool) * (maxNode + 1));
            //int u = -1;
            for(set<int>::iterator lit = c.loopNodes.begin(); 
                    lit != c.loopNodes.end(); lit++) {
              //  if(u == -1) u = (m.find(*lit) == m.end()) ? *lit : -1; 
                involved[*lit] = true;      
            } 
            for(set<int>::iterator mit = m.begin(); 
                    mit != m.end(); mit++) {
                involved[G_NLP[*mit].head] = false;      
            }

            findSCCInSCC(sccs);
        }
    }
   
    return scc;    
}

vector<Loop> DependenceGraph::getLoopWithESRuleSize(int k) {
    return this->loopWithESSize[k];
}

vector<int> DependenceGraph::getESRSizes() {
    vector<int> result;
    
    for(map<int, vector<Loop> >::iterator it = loopWithESSize.begin();
            it != loopWithESSize.end(); it++) {
        result.push_back(it->first);
    }
    
    sort(result.begin(), result.end());    //default comp is operator < 
    
    return result;
}

int DependenceGraph::setRelation(const set<int>& es1, const set<int>& es2) {
    for(set<int>::iterator it1 = es1.begin(), it2 = es2.begin(); it1 != es1.end();) {
        if(it2 == es2.end() || *it2 > *it1) {
            return -1;
        }
        else if(*it2 == *it1) {
            it2++;
            it1++;
        }
        else if(*it2 < *it1) {
            it2++;
        }
    }

    if(es1.size() == es2.size()) return 0;
    else return 1;
}

int DependenceGraph::setRelation(const set<int>& es1, const set<int>& es2, set<int>& m) {
    for(set<int>::iterator it1 = es1.begin(), it2 = es2.begin(); it1 != es1.end();) {
        if(it2 == es2.end() || *it2 > *it1) {
            m.insert(*it1);
            it1++;
        }
        else if(*it2 == *it1) {
            it2++;
            it1++;
        }
        else if(*it2 < *it1) {
            it2++;
        }
    }

    if(es1.size() == es2.size() && m.size() == 0) return 0;
    else if(m.size() == 0) return 1;
    else return -1;
}

//special case for pearl graph using minimal cut
bool cmp(const Loop& l1, const Loop& l2) {
    return l1.loopNodes.size() < l2.loopNodes.size();
}

void DependenceGraph::findProperLoopsCuts(vector<Loop>& ploops, vector<Loop>& cloops, set<int>& s, set<int>& cns, set<int>& in) {   
    if(s.size() == 0) {
        memset(involved, true, sizeof(bool) * (maxNode + 1));  
    }
    else {
        memset(involved, false, sizeof(bool) * (maxNode + 1));
        for(set<int>::iterator sit = s.begin(); sit != s.end(); sit++) involved[*sit] = true;
    }
    
    vector<Loop> sccs = findSCC();
    
    for(int i = 0; i < sccs.size(); i++) {
        Loop tc = sccs[i];
        int hash = Hash(tc.loopNodes);
        if(!loopHash[hash]) {
            loopHash[hash] = true;          
//             printf("loop: ");
//             tc.print(stdout);
             Loop c = searchProperLoop(tc);
             Loop ic = searchProperLoop(tc, in);
             properTimes++;
//             printf("proper: ");
//             c.print(stdout);
//             fflush(stdout);   
             if(ic == tc) {
                bool ok = false;
                for(set<int>::iterator iit = c.ESRules.begin(); iit != c.ESRules.end(); iit++) {
                    if(cns.find(G_NLP[*iit].head) != cns.end()) {
                        ok = true;
                        break;
                    }
                }
                if(ok) cloops.push_back(c); 
             }
             if(c == tc) {                
                 ploops.push_back(c);
                 esHash[Hash(c.ESRules)] = true;
                 for(set<int>::iterator it = tc.ESRules.begin(); it != tc.ESRules.end(); it++) {
                     set<int> s = tc.loopNodes;
                     s.erase(G_NLP[*it].head);
                     findProperSection(ploops, cloops, cns, s, in, true);
                 }
             }
             else {
                 findProperSection(ploops, cloops, cns, tc.loopNodes, in, false);
             }
        }
    }  
}

vector<Loop> DependenceGraph::findProperLoopsWithCut2() {
   // int n = 2;
    vector<Loop> ploops;
    secHash.clear();
    loopHash.clear();
    properTimes = 0;
    
    memset(involved, true, sizeof(bool) * (maxNode + 1));
    vector<Loop> sccs = findSCC();      //找到整个大连通图
    int SIZE = sccs.front().loopNodes.size() / 2;         //判断每个连通图的点数。
    
    int size = Vocabulary::instance().apSize();
    set<int> L1_nodes, L2_nodes; 
    int num;
    for(int i = 1; i <= size; i++) {
        if(strncmp(Vocabulary::instance().getAtom(i), "reached(cutNode", 15) == 0) {
            int judge = Vocabulary::instance().getAtom(i)[16] - '0';
            if(judge >= 0 && judge <= 9)
               num = (Vocabulary::instance().getAtom(i)[15] - '0')*10 + judge;
            else
               num = (Vocabulary::instance().getAtom(i)[15] - '0'); 
            if(num <= SIZE) {
               if(num % 2 != 0)         
                   L1_nodes.insert(i);
            }
            else {      
               if((num - SIZE) % 2 != 0)
                   L2_nodes.insert(i);       
            }
        }
    }
    
    memset(involved, true, sizeof(bool) * (maxNode + 1));
    for(set<int>::iterator it = L2_nodes.begin(); it != L2_nodes.end(); it++)
        involved[*it] = false;
    vector<Loop> sccs1 = findSCC();
    sort(sccs1.begin(), sccs1.end(), cmp);
    
    memset(involved, true, sizeof(bool) * (maxNode + 1));
    for(set<int>::iterator it = L1_nodes.begin(); it != L1_nodes.end(); it++)
        involved[*it] = false;
    vector<Loop> sccs2 = findSCC();
    sort(sccs2.begin(), sccs2.end(), cmp);
    
    vector<Loop> ll1;
    findProperLoopsCuts(ploops, ll1, sccs1.back().loopNodes, L1_nodes, sccs1.back().loopNodes);
    vector<Loop> ll2;
    findProperLoopsCuts(ploops, ll2, sccs2.back().loopNodes, L2_nodes, sccs2.back().loopNodes);
    
    for(int i = 0; i < ll1.size(); i++) {
        for(int j = 0; j < ll2.size(); j++) {
           set<int> cl1 = ll1[i].loopNodes;
           set<int> cl2 = ll2[j].loopNodes;
           
           for(set<int>::iterator it = cl2.begin(); it != cl2.end(); it++) {
               cl1.insert(*it);
           }
           
           if(isLoop(cl1)) {
               Loop ploop(cl1);
               findESRules(ploop);
               //Loop cp = searchProperLoop(ploop);
               
               int hash = Hash(ploop.ESRules);
               if(!esHash[hash]) {
                   esHash[hash] = true;
                   ploops.push_back(ploop);
               }
           }
        }
    }
    
    return ploops;
}

bool DependenceGraph::isLoop(set<int> lns) {
    memset(involved, false, sizeof(bool) * (maxNode + 1));
    for(set<int>::iterator it = lns.begin(); it != lns.end(); it++)
        involved[*it] = true;   
    
    vector<Loop> sccs = findSCC();
    if(sccs.size() == 0) return false;
    else {
        return sccs.front().loopNodes.size() == lns.size();
    }
}

bool DependenceGraph::setCross(const set<int>& es1, const set<int>& es2) {
    for(set<int>::iterator it = es1.begin(); it != es1.end(); it++) {
        if(es2.find(*it) != es2.end()) return true;
    }
    
    return false;
}



/*
 * Additional functions for Splitting
 */
/*
 * 在一开始构造原图时，先构造一个无向图，然后使用该无向图进行查找割集 
 */
set<int> DependenceGraph::calU() {
    set<int> ret;
    int attack = -1, endure = -1;
    bool isExist = false;
    for (map< int, set<int> >::iterator it = udpdGraph.begin();
            it != udpdGraph.end(); ++ it) {
        map<int, bool> isVisited;
        queue<int> que;
        attack = it->first;
        que.push(attack);
        isVisited[attack] = true;
        endure = -1;
        while (! que.empty()) {
            int cur = que.front();
            que.pop();
            set<int> &adj = udpdGraph[cur];
            for (set<int>::iterator it = adj.begin(); it != adj.end(); ++ it) {
                int next = *it;
                if (! isVisited[next]) {
                    isVisited[next] = true;
                    que.push(next);
                }
            }
            endure = cur;
        }
        if (udpdGraph[attack].find(endure) == udpdGraph[attack].end()) {
            isExist = true;
            break;
        }
    }
    if (! isExist) {
        return ret;
    }

    set<int> attackSet, attackOneSet;//攻的集和、一次被攻推到的集合
    set<int> endureSet, endureOneSet;//受的集合、一次被受推到的集合
    set<int> bothOneSet, unknownSet;//攻受都可以一次推到的集合、未确定的集合
    map<int, int> weight;

    for (map< int, set<int> >::iterator it = udpdGraph.begin();
            it != udpdGraph.end(); ++ it) {
        unknownSet.insert(it->first);
    }
    unknownSet.erase(attack);
    attackSet.insert(attack);
    for (set<int>::iterator it = udpdGraph[attack].begin();
            it != udpdGraph[attack].end(); ++ it) {
        unknownSet.erase(*it);
        attackOneSet.insert(*it);
        weight.insert(make_pair(*it, 1));
    }
    unknownSet.erase(endure);
    endureSet.insert(endure);
    for (set<int>::iterator it = udpdGraph[endure].begin();
            it != udpdGraph[endure].end(); ++ it) {
        if (attackOneSet.find(*it) != attackOneSet.end()) {
            attackOneSet.erase(*it);
            bothOneSet.insert(*it);
        }
        else {
            unknownSet.erase(*it);
            endureOneSet.insert(*it);
            weight.insert(make_pair(*it, 1));
        }
    }

    while (! attackOneSet.empty() || ! endureOneSet.empty()) {
        if (! attackOneSet.empty()) {
            int cur = *(attackOneSet.begin());
            int maxWeight = weight[cur];
            for (set<int>::iterator it = attackOneSet.begin();
                    it != attackOneSet.end(); ++ it) {
                if (weight[*it] > maxWeight) {
                    maxWeight = weight[*it];
                    cur = *it;
                }
            }

            attackOneSet.erase(cur);
            attackSet.insert(cur);
            set<int> &adj = udpdGraph[cur];
            for (set<int>::iterator it = adj.begin();
                    it != adj.end(); ++ it) {
                weight[*it] ++;
                if (unknownSet.find(*it) != unknownSet.end()) {
                    unknownSet.erase(*it);
                    attackOneSet.insert(*it);
                    weight.insert(make_pair(*it, 1));
                }
                else if (endureOneSet.find(*it) != endureOneSet.end()) {
                    endureOneSet.erase(*it);
                    bothOneSet.insert(*it);
                }
            }
        }
        if (! endureOneSet.empty()) {
            int cur = *(endureOneSet.begin());
            int maxWeight = weight[cur];
            for (set<int>::iterator it = endureOneSet.begin();
                    it != endureOneSet.end(); ++ it) {
                if (weight[*it] > maxWeight) {
                    maxWeight = weight[*it];
                    cur = *it;
                }
            }

            endureOneSet.erase(cur);
            endureSet.insert(cur);
            set<int> &adj = udpdGraph[cur];
            for (set<int>::iterator it = adj.begin();
                    it != adj.end(); ++ it) {
                weight[*it] ++;
                if (unknownSet.find(*it) != unknownSet.end()) {
                    unknownSet.erase(*it);
                    endureOneSet.insert(*it);
                }
                else if (attackOneSet.find(*it) != attackOneSet.end()) {
                    attackOneSet.erase(*it);
                    bothOneSet.insert(*it);
                }
            }
        }
    }

    for (map< int, set<int> >::iterator it = udpdGraph.begin();
            it != udpdGraph.end(); ++ it) {
        if (attackSet.find(it->first) == attackSet.end() &&
                endureSet.find(it->first) == endureSet.end()) {
            ret.insert(it->first);
        }
    }
//    printf("\nRet size : %d\n", ret.size());
    return ret;
}


// 计算出b_U(P)，P为G_NLP，U使用一个全局变量U。
void DependenceGraph::b_UP() {    
    bUP.clear();
    int i = 0;
    for(vector<Rule>::iterator it = G_NLP.begin(); it != G_NLP.end(); it++, i++) {
        if(U.find(it->head) != U.end()) {
//            bUP.push_back(*it);
            bUP.insert(i);
        }
    }
}               

// 计算出in_U(P)
void DependenceGraph::in_UP() {
    inUP.clear();
    int num = 0;
    for(vector<Rule>::iterator it = G_NLP.begin(); it != G_NLP.end(); it++, num++) {
        if(it->type == RULE) {
            if(U.find(it->head) != U.end()) {
                // 先从Rule中的body_list得到正原子集，然后使用“两次”set_difference来判断其与U是否真子集关系
                set<int> bodyPlus;  // 体部正原子集合
                for(set<int>::iterator sit = (it->body_lits).begin(); sit != (it->body_lits).end(); sit++) {
                    if((*sit) >= 0)
                        bodyPlus.insert(*sit);
                }
                if(bodyPlus.size() == 0) {
                    continue;       // 如果body^+(r)为空集，而U必不是空集，所以就有body^+(r)是U的真子集，所以跳过这条公式。
                }

                // 如果仅属于U而不属于bodyPlus的元素集合不为空，且，仅属于bodyPlus而不属于U的元素集合为空，那么bodyPlus为U的真子集
                // set_difference(sa.begin(),sa.end(),sb.begin(),sb.end(),inserter(sc,sc.begin()));
//                set<int> tmp1, tmp2;
//                set_difference(U.begin(), U.end(), bodyPlus.begin(), bodyPlus.end(), inserter(tmp1, tmp1.begin()));
//                set_difference(bodyPlus.begin(), bodyPlus.end(), U.begin(), U.end(), inserter(tmp2, tmp2.begin()));
//                
//                if(!(tmp1.size() > 0 && tmp2.size() == 0)) {
//                    inUP.push_back(*it);
//                }
                
                // 事实上，个人认为：A不是B的真子集 可以等价于 A \ B 不等于空集
                
                // 判断两个数组（vector，set，常规数据均可）直接使用algorithm中的includes函数即可
                // includes函数判断前者是否包含后者，注意真子集不包含两者完全相等的情况，所以还要判断size。
                // 这里的效率其实是可以保证的，一旦计算出bodyPlus的size比U的小，则里边表达式会直接返回false，然后取非后得到true
                if(!(bodyPlus.size() < U.size() && includes(U.begin(), U.end(), bodyPlus.begin(), bodyPlus.end())))
//                    inUP.push_back(*it);   
                    inUP.insert(num);
            }
        }
    }
}   

// 计算出out_U(P)
void DependenceGraph::out_UP() {
    outUP.clear();
    int num = 0;
    for(vector<Rule>::iterator it = G_NLP.begin(); it != G_NLP.end(); it++, num++) {
        if(it->type == RULE) {
            set<int> bodyPlus;      // 体部正原子集合
            for(set<int>::iterator sit = (it->body_lits).begin(); sit != (it->body_lits).end(); sit++) {
                if((*sit) >= 0) {
                    bodyPlus.insert(*sit);
                }
            }
            if(bodyPlus.size() == 0) {
                continue;       // body^+(r)为空集的话，其与U的交集必为空集，故不符合条件，跳过该公式。
            }
            
            set<int> intersection;  // 体部正原子集合与U的交集。 
            set_intersection(bodyPlus.begin(), bodyPlus.end(), U.begin(), U.end(), inserter(intersection, intersection.begin()));
            if(intersection.size() > 0) {
                if(U.find(it->head) == U.end()) {
//                    outUP.push_back((*it));     // head(r)在NLP中只有一个元素，所以如果该元素不在U中，则必有head(r)不是U的真子集。
                    outUP.insert(num);
                }
            }
        }
    }   
}   

// 计算出EC_U(P)
void DependenceGraph::EC_UP() {
    ECUP.clear();
    set<int> AtomBUP;   // Atoms(b_U(P))
    for(set<int>::iterator it = bUP.begin(); it != bUP.end(); it++) {
        Rule R = G_NLP[*it];
        AtomBUP.insert(R.head);
        for(set<int>::iterator bit = (R.body_lits).begin(); bit != (R.body_lits).end(); bit++) {
            int element = (*bit) >= 0 ? (*bit) : -(*bit);
            AtomBUP.insert(element);
        }
    }
    
    set<int> diff;      // Atoms(b_U(P)) \ U
    set_difference(AtomBUP.begin(), AtomBUP.end(), U.begin(), U.end(), inserter(diff, diff.begin()));
    
    // 输出 Atoms(b_U(P)) \ U 的数量到 count_out.txt
    fprintf(count_out, "Atoms(b_U(P)) - U size = %d\n", diff.size());
    
    // 由定义可知diff中的元素必为正值
    for(set<int>::iterator it = diff.begin(); it != diff.end(); it++) {
        // 构造 a :- not a_.
        Rule R1;
        R1.head = (*it);
        string s(Vocabulary::instance().getAtom(*it));
        s = "_" + s;
        char *tmp = new char[s.size() + 1];
        strcpy(tmp, s.c_str());         // 这里进行一次深拷贝是为了避免Vocabulary中的atom_list被指针修改了。
        int id = Vocabulary::instance().addAtom(tmp);    // 注意Vocabulary中的getAtom的实现。
        R1.body_lits.insert(-id);        R1.body_length = 1;
        
        // 构造 a_ :- not a.
        Rule R2;
        R2.head = id;
        R2.body_lits.insert(-(*it));    R2.body_length = 1;
        
        ECUP.push_back(R1);     ECUP.push_back(R2);
    }
}                   

// 计算出b_U(P)和EC_U(P)并集的所有answer set
set< set<int> > DependenceGraph::solutionXs() {   
    vector<Rule> pX;    // b_U(P)和EC_U(P)的并集
    for(set<int>::iterator it = bUP.begin(); it != bUP.end(); it++)
        pX.push_back(G_NLP[*it]);
    pX.insert(pX.end(), ECUP.begin(), ECUP.end());  
    
    long start = clock();
    FILE *result;
    result = fopen("IO/pX.txt", "w");
    for(vector<Rule>::iterator pit = pX.begin(); pit != pX.end(); pit++) {
        (*pit).output(result);    fflush(result);
    }
    
    // 再把 Atoms(b_U(P)) - U 的 原子全部作为事实加入到 pX.txt 中，以保证每个 X 都能命中。
    set<int> AtomBUP;   // Atoms(b_U(P))
    for(set<int>::iterator it = bUP.begin(); it != bUP.end(); it++) {
        Rule R = G_NLP[*it];
        AtomBUP.insert(R.head);
        for(set<int>::iterator bit = (R.body_lits).begin(); bit != (R.body_lits).end(); bit++) {
            int element = (*bit) >= 0 ? (*bit) : -(*bit);
            AtomBUP.insert(element);
        }
    }
    
    set<int> diff;      // Atoms(b_U(P)) \ U
    set_difference(AtomBUP.begin(), AtomBUP.end(), U.begin(), U.end(), inserter(diff, diff.begin()));
    for(set<int>::iterator sit = diff.begin(); sit != diff.end(); sit++) {
        fprintf(result, "%s.\n", Vocabulary::instance().getAtom(*sit));
    }
    
    // 再加入左边的所有 :- not reached(a)。
    for(int i = 0; i < m_pealSize; i++)
        fprintf(result, ":- not reached(%d).\n", i);
    
    
    fflush(result);
    fclose(result);
    long end = clock();
    XIOTime = (double)(end - start) / CLOCKS_PER_SEC;
    return callClaspD("IO/pX.txt");
//    set< set<int> > result_;
//    return result_;
}   

/* 
 * 计算出ECC_U(P, X)
 * 由两部分组成：
 * 基于集合：（X与Atoms(P)的交集） \ U 和 Atoms(b_U(P)) \ U 及 X
 */
vector<Rule> DependenceGraph::ECC_UP(set<int> X) {    
    vector<Rule> result;
    
    // （X与Atoms(P)的交集） \ U 部分
    set<int> intersectXP;       // （X与Atoms(P)的交集）
    for(set<int>::iterator xit = X.begin(); xit != X.end(); xit++) {
        if((*xit) <= apsize) {
            intersectXP.insert(*xit);
        }
    }
    set<int> diff_XPU;      // （X与Atoms(P)的交集） \ U
    set_difference(intersectXP.begin(), intersectXP.end(), U.begin(), U.end(), inserter(diff_XPU, diff_XPU.begin()));
    for(set<int>::iterator it = diff_XPU.begin(); it != diff_XPU.end(); it++) {
        Rule R;         R.type = CONSTRANT;
        R.body_lits.insert(-(*it));     R.body_length = 1;
        result.push_back(R);
    }
    
    // Atoms(b_U(P)) \ U 及 X
    set<int> AtomBUP;   // Atoms(b_U(P))
//    for(vector<Rule>::iterator it = bUP.begin(); it != bUP.end(); it++) {
    for(set<int>::iterator it = bUP.begin(); it != bUP.end(); it++) {
        Rule R = G_NLP[*it];
        AtomBUP.insert(R.head);
        for(set<int>::iterator bit = (R.body_lits).begin(); bit != (R.body_lits).end(); bit++) {
            int element = (*bit) >= 0 ? (*bit) : -(*bit);
            AtomBUP.insert(element);
        }
    }
    
    set<int> diff_bPU;      // Atoms(b_U(P)) \ U
    set_difference(AtomBUP.begin(), AtomBUP.end(), U.begin(), U.end(), inserter(diff_bPU, diff_bPU.begin()));
    for(set<int>::iterator it = diff_bPU.begin(); it != diff_bPU.end(); it++) {
        string s(Vocabulary::instance().getAtom(*it));
        s = "_" + s;
        char *tmp = new char[s.size() + 1];
        strcpy(tmp, s.c_str());         // 这里进行一次深拷贝是为了避免Vocabulary中的atom_list被指针修改了。
        int it_id = Vocabulary::instance().queryAtom(tmp);      delete[] tmp;
        if(X.find(it_id) != X.end()) {
            Rule R;     R.type = CONSTRANT;
            R.body_lits.insert(*it);       R.body_length = 1;
            result.push_back(R);
        }
    }
    
    return result;
}     

// 计算出half loop E，先计算出所有环，然后再计算出所有的E    
void DependenceGraph::calE() { 
    Es.clear();
    
    vector<Loop> allLoops;
    for(set<int>::iterator it = U.begin(); it != U.end(); it++) {
        Loop a;
        a.loopNodes.insert(*it);
        allLoops.push_back(a);
    }
    Loop a(U);
    allLoops.push_back(a);
    
    for(vector<Loop>::iterator it = allLoops.begin(); it != allLoops.end(); it++) {
        
        if(!(includes(U.begin(), U.end(), (it->loopNodes).begin(), (it->loopNodes).end()))) {
            set<int> ENodes;
            set_intersection((it->loopNodes).begin(), (it->loopNodes).end(), U.begin(), U.end(), inserter(ENodes, ENodes.begin()));
            if(ENodes.size() == 0) continue;
            Loop E;
            E.loopNodes = ENodes;
            printf("ENode size: %d\n", ENodes.size());
            Es.push_back(E);
            
        }
    }
}

/* 
 * 计算出R^-(E, X)，定义为：E在程序P中的外部支持，且外部支持中的rule要有body(r)满足X，
 * 其中body(r)满足X定义为：body+(r)都属于X，且body-(r)不存在原子属于X。
 */
void DependenceGraph::R_EX(Loop& E, set<int> X) {
    // 先找出该环基于整个程序P的外部支持
    findESRules(E);     // findESRules函数的参数也是取引用的。
//    printf("\nE external support : \n");
//    for(set<int>::iterator it = E.ESRules.begin(); it != E.ESRules.end(); it++) {
//        G_NLP[*it].output(stdout);
//    }
//    printf("\n");
     
    // 删去body(r)不满足X的外部支持，注意删除过程中设计的迭代器失效问题
    for(set<int>::iterator it = E.ESRules.begin(); it != E.ESRules.end();) {
        Rule R = G_NLP[*it];
        set<int> bodyPlus, bodyMinu;    // body+(r)和body-(r)
        for(set<int>::iterator bit = R.body_lits.begin(); bit != R.body_lits.end(); bit++) {
            if((*bit) < 0)
                bodyMinu.insert(-(*bit));
            else
                bodyPlus.insert(*bit);
        }
        
        // includes判断前者是否包含后者，返回bool。
        bool plus = includes(X.begin(), X.end(), bodyPlus.begin(), bodyPlus.end());
        bool minu = true;
        for(set<int>::iterator bit = bodyMinu.begin(); bit != bodyMinu.end(); bit++) {
            if(X.find((*bit)) != X.end()) { // 如果找到一个，就证明是不符合
                minu = false;
                break;
            }
        }
        
        if(!(plus && minu))
            E.ESRules.erase(it++);   // 注意迭代器失效问题
        else
            it++;
    }
}   

// 计算出HL_U(P, X)，其本质是：一系列的half loop E
vector<Loop> DependenceGraph::HL_UPX(set<int> X) {
    vector<Loop> HLUPX;
    
    for(vector<Loop>::iterator it = Es.begin(); it != Es.end(); it++) {
        // 判断E是否为X的真子集
        if(((it->loopNodes).size() < X.size()) && 
                includes(X.begin(), X.end(), (it->loopNodes).begin(), (it->loopNodes).end())) {
            // 判断R-(E,X)与in_U(P)的差集是否为空，其实就是两者是否相等
            R_EX((*it), X);
            if(inUP.size() == (it->ESRules).size() &&
                includes(inUP.begin(), inUP.end(), (it->ESRules).begin(), (it->ESRules).end())) {
                HLUPX.push_back(*it);
            }
        }
    }
    
    return HLUPX; 
}   

// 计算出t_U(P, X)
vector<Rule> DependenceGraph::t_UPX(set<int> X) {
    vector<Rule> result;
    
    // part 1
    set<int> union_;
    set_union(bUP.begin(), bUP.end(), outUP.begin(), outUP.end(), inserter(union_, union_.begin()));
    // P \ (bUP + outUP) 的rule集合
    for(int i = 0; i < G_NLP.size(); i++)
        if(union_.find(i) == union_.end())
            result.push_back(G_NLP[i]);
    
    vector<Loop> HLUPX = HL_UPX(X);
//    printf("\nHL_U(P,X) size = %d\n", HLUPX.size());
    
    // part 2，当HL_U(P,X)为空时，第二部分也为空.
    if(HLUPX.size() > 0) {
//        // not sure !!!
//        for(set<int>::iterator it = inUP.begin(); it != inUP.end(); it++)
//            result.push_back(G_NLP[*it]);
//    }
//    else {
        for(int i = 1; i <= HLUPX.size(); i++) {
            // 第二部分中的xE中的E其实可能是一个不止一个元素的环，所以直接命名也没有问题
            for(set<int>::iterator iit = inUP.begin(); iit != inUP.end(); iit++) {
                stringstream ss;    // 数字可能远超9,所以用stringstream好一些
                string tmp = "";
                string str("x");
                ss << i;
                tmp = ss.str();
//                tmp += 'a' + i - 1;
                str = "a" + tmp + str;  // 这里加个“a”是为了防止 clasp 读入失败
                char *input = new char[str.size() + 1];
                strcpy(input, (char*)(str.c_str())); input[str.size()] = '\0';
                int id = Vocabulary::instance().queryAtom(input);
                if(id == -1)
                   id = Vocabulary::instance().addAtom(input);  
                Rule R_;        R_.head = id;
                R_.body_lits = G_NLP[*iit].body_lits; // 当前对应的in_U(P)中的rule
                R_.body_length = R_.body_lits.size();
                
                result.push_back(R_);
            }
        }
    }
    
    // part 3，当HL_U(P,X)为空时，直接插入out_U(P)，这个是确定的.
    if(HLUPX.size() == 0) {
        for(set<int>::iterator it = outUP.begin(); it != outUP.end(); it++)
            result.push_back(G_NLP[*it]);
    }
    else {
        for(set<int>::iterator iit = outUP.begin(); iit != outUP.end(); iit++) {
            Rule R_;
            R_.head = G_NLP[*iit].head;
            R_.body_lits = G_NLP[*iit].body_lits;
            for(int i = 1; i <= HLUPX.size(); i++) {
                set<int> bodyPlus;
                for(set<int>::iterator bit = G_NLP[*iit].body_lits.begin(); 
                        bit != G_NLP[*iit].body_lits.end(); bit++) {
                    if((*bit) >= 0)
                        bodyPlus.insert(*bit);
                }
                set<int> intersect;
                set_intersection(bodyPlus.begin(), bodyPlus.end(), HLUPX.at(i-1).loopNodes.begin(),
                        HLUPX.at(i-1).loopNodes.end(), inserter(intersect, intersect.begin()));
                if(intersect.size() > 0) {
                    stringstream ss;    
                    string tmp = "";   
                    string str("x");
                    ss << i;
                    tmp = ss.str();
//                    tmp += 'a' + i - 1;
                    str = "a" + tmp + str; 
                    char *input = new char[str.size() + 1];
                    strcpy(input, (char*)(str.c_str())); input[str.size()] = '\0';
                    int id = Vocabulary::instance().queryAtom(input);
                    if(id == -1)
                       id = Vocabulary::instance().addAtom(input); 
                    R_.body_lits.insert(id);
                }
            } 
            R_.body_length = R_.body_lits.size();
            result.push_back(R_);
        }
    }
    
    return result;
}                  

// 计算出e_U(P, X)
void DependenceGraph::e_U(vector<Rule>& tuPX, set<int> X) {
    // part 1
    for(vector<Rule>::iterator tit = tuPX.begin(); tit != tuPX.end(); ) {
        set<int> bodyPlus, bodyMinu;
        for(set<int>::iterator it = (tit->body_lits).begin(); it != (tit->body_lits).end(); it++) {
            if((*it) >= 0)
                bodyPlus.insert(*it);
            else
                bodyMinu.insert(-(*it));
        }
        set<int> PlusIntersect, MinuIntersect, XIntersect;
        set_intersection(U.begin(), U.end(), bodyPlus.begin(), bodyPlus.end(), inserter(PlusIntersect, PlusIntersect.begin()));
        set_intersection(U.begin(), U.end(), bodyMinu.begin(), bodyMinu.end(), inserter(MinuIntersect, MinuIntersect.begin()));
        set_intersection(X.begin(), X.end(), MinuIntersect.begin(), MinuIntersect.end(), inserter(XIntersect, XIntersect.begin()));
        if(!(PlusIntersect.size() < X.size() && includes( X.begin(), X.end(), PlusIntersect.begin(), PlusIntersect.end())) 
             || XIntersect.size() > 0) {
            tit = tuPX.erase(tit);
        }
        else
            tit++;
    }
    
    // part 2
    // 先得到一份U中所有元素及其相反数的集合(tmpU)，而后面判断时，可以直接使用body(r) \ U。
    set<int> tmpU;
    for(set<int>::iterator uit = U.begin(); uit != U.end(); uit++) {
        tmpU.insert(*uit);
        tmpU.insert(-(*uit));
    }
    for(vector<Rule>::iterator tit = tuPX.begin(); tit != tuPX.end(); tit++) {
        set<int> diff;
        set_difference((tit->body_lits).begin(), (tit->body_lits).end(), tmpU.begin(), tmpU.end(), inserter(diff, diff.begin()));
        (tit->body_lits).clear();
        if(diff.size() == 0)
            tit->type = FACT;
        else
            tit->body_lits = diff;
        tit->body_length = (tit->body_lits).size();
    }
}                    

// 计算出e_U(P, X)和ECC_U(P, X)并集的所有answer set
set< set<int> > DependenceGraph::solutionYs(vector<Rule> pY,set<int> X) {
    long start = clock();
    FILE* result;
    result = fopen("IO/pY.txt", "w");
    for(vector<Rule>::iterator pit = pY.begin(); pit != pY.end(); pit++) {
        (*pit).output(result);  fflush(result);
    }
    fflush(result);
    fclose(result);
    long end = clock();
    YIOTime = (double)(end - start) / CLOCKS_PER_SEC;
    return callClaspD("IO/pY.txt");
}   

      
/*
 * 调用claspD来求程序的answer set，输入的程序为solutionXs及solutionYs中产生的程序文件。
 */
set< set<int> > DependenceGraph::callClaspD(string fileName) {
    // 管道调用 RUN_CMD 计算并将其结果写进 OUTPUT_FILE
    string cmd = "gringo " + fileName + " | clasp 100 > " + OUTPUT_FILE;
    char buff[BUFF_SIZE];
    FILE *pipe_file = popen(cmd.c_str(), "r");
    FILE *output_file = fopen(OUTPUT_FILE, "w");
    while (fgets(buff, BUFF_SIZE, pipe_file)) {
        fprintf(output_file, "%s", buff);
    }
    pclose(pipe_file);
    fclose(output_file);
    
    long start = clock();
    // 提取answer set
    set< set<string> > ret;     // 结果所在，还要转成int
    string line, str;
    ifstream answer_in(OUTPUT_FILE);
    bool flag = false;
    while (getline(answer_in, line)) {
        if (line.substr(0, 6) == "Answer") {
            flag = true;
            continue;
        }
        if (flag) {
            flag = false;
            stringstream ss(line);
            set<string> ans;
            while (ss >> str) {
                ans.insert(str);
            }
            ret.insert(ans);
        }
    }
    answer_in.close();
    
//    for(set< set<string> >::iterator sit = ret.begin(); sit != ret.end(); sit++) {
//        for(set<string>::iterator rit = (*sit).begin(); rit != (*sit).end(); rit++)
//            printf("%s ", (*rit).c_str());
//        printf("\n");
//        fflush(stdout);
//    }
    
    
    // 把ret中的string转化为对应的atom ID。
    set< set<int> > result;     //printf("\nAnswer sets : \n");
    for(set< set<string> >::iterator rit = ret.begin(); rit != ret.end(); rit++) {
        set<int> tmp;   
        for(set<string>::iterator sit = (*rit).begin(); sit != (*rit).end(); sit++) {
//            printf("%s\n", (*sit).c_str());     fflush(stdout);
            int id = Vocabulary::instance().queryAtom((char*)((*sit).c_str()));
            if(id != -1)
                tmp.insert(id);
            else
                printf("\ncallClaspD Error!\n");
        }
        //printf("\n");
        result.insert(tmp);
    }
    long end = clock();
    ClaspDTime = (double)(end - start) / CLOCKS_PER_SEC;
    
    return result;
}

bool DependenceGraph::judge(FILE* out, set<int> X, set<int> Y) {
    set<int> S;
    set_union(X.begin(), X.end(), Y.begin(), Y.end(), inserter(S, S.begin()));
    
    for(set<int>::iterator it = S.begin(); it != S.end();) {
        if((*it) > apsize)
            S.erase(it++);
        else
            it++;
    }
    fprintf(out, "\nS : ");
    for(set<int>::iterator it = S.begin(); it != S.end(); it++) {
        fprintf(out, "%s ", Vocabulary::instance().getAtom(*it));
    }
    fprintf(out, "\n");
    
    bool flag = false;
    for(set< set<int> >::iterator pit = PAnswerSets.begin(); pit != PAnswerSets.end(); pit++) {
        if(S.size() == (*pit).size() && includes(S.begin(), S.end(), (*pit).begin(), (*pit).end())) {
            flag = true;
            break;
        }
    }
    
    if(!flag)
        fprintf(out, "\nS is an answer set of input program! : False\n");
    else {
        fprintf(out, "\nS is an answer set of input program! : True\n");
        exit(0);
    }
    
    return flag;
}


// 整个splitting的过沉
void DependenceGraph::splitting(FILE* out, string argv1) {
    XTime = 0;          YTime = 0;
    XIOTime = 0;        YIOTime = 0;
    ClaspDTime = 0;
    
    PAnswerSets = callClaspD(argv1);
    fprintf(out, "Input program P's answer sets : \n");
    for(set< set<int> >::iterator pit = PAnswerSets.begin(); pit != PAnswerSets.end(); pit++) {
        for(set<int>::iterator it = (*pit).begin(); it != (*pit).end(); it++) {
            fprintf(out, " %s", Vocabulary::instance().getAtom(*it));
        }
        fprintf(out, "\n");
    }
    fprintf(out, "\n");
    
    fprintf(out, "U size : %d\n", U.size());
//    for(set<int>::iterator it = U.begin(); it != U.end(); it++)
//        fprintf(out, "%s ", Vocabulary::instance().getAtom(*it));
//    fprintf(out, "\n");
    
    long start = clock();
    // calculate b_U(P)
    b_UP();   //  printf("\nb_U(P) done!\n");
//    fprintf(out, "\nb_U(P) : \n");
//    for(set<int>::iterator it = bUP.begin(); it != bUP.end(); it++) {
//        G_NLP[*it].output(out);
//    }
//    fprintf(out, "\n");
    
    // Atoms(b_U(P)) - U 的数目

    // calculate EC_U(P)
    EC_UP();   // printf("\nEC_U(P) done!\n");
//    fprintf(out, "EC_U(P) : \n");
//    for(vector<Rule>::iterator it = ECUP.begin(); it != ECUP.end(); it++) {
//        (*it).output(out);
//    }
//    fprintf(out, "\n");
    
    // calculate answer set X
    long middle = clock();
    XTime = (double)(middle - start) / CLOCKS_PER_SEC;
    set< set<int> > Xs = solutionXs(); // printf("\nXs done!\n");
    long end = clock();
    XTime += ( (double)(end - middle) / CLOCKS_PER_SEC - XIOTime - ClaspDTime);
    XTime /= Xs.size();     // Average
    fprintf(out, "\nXTime = %lf\n", XTime);
    
    
    fprintf(out, "Answer set Xs num : %d\n", Xs.size()); fflush(out);
//    for(set< set<int> >::iterator it = Xs.begin(); it != Xs.end(); it++) {
//        for(set<int>::iterator sit = (*it).begin(); sit != (*it).end(); sit++) {
//            fprintf(out, " %s", Vocabulary::instance().getAtom(*sit));  fflush(out);
//        }
//        fprintf(out, "\n");
//    }
//    fprintf(out, "\n"); fflush(out);
    
    
    // calculate in_U(P)
    in_UP();   // printf("\nin_U(P) done!\n");
//    fprintf(out, "in_U(P) : \n");
//    for(set<int>::iterator it = inUP.begin(); it != inUP.end(); it++) {
//        G_NLP[*it].output(out);
//    }
//    fprintf(out, "\n");
    
    // calculate out_U(P)
    out_UP(); //  printf("\nout_U(P) done!\n");
//    fprintf(out, "out_U(P) : \n");
//    for(set<int>::iterator it = outUP.begin(); it != outUP.end(); it++) {
//        G_NLP[*it].output(out);
//    }
//    fprintf(out, "\n");
    
    // calculate all half loops E
    calE();         fprintf(out, "\nE done, Es size = %d\n", Es.size());
    // answer set Y 
    for(set< set<int> >::iterator xit = Xs.begin(); xit != Xs.end(); xit++) {
        
        set<int> Xtest = (*xit);
        fprintf(out, "----------------------------\nChoose X : \n");    //  printf("\nChoose X : \n");
        for(set<int>::iterator it = Xtest.begin(); it != Xtest.end(); it++)
            fprintf(out, "%s ", Vocabulary::instance().getAtom(*it));
        fprintf(out, "\n");
        
        long start1 = clock();
        // calculate ECC_U(P,X)
        vector<Rule> eccupx = ECC_UP(Xtest);   // printf("\nECC_U(P, X) done\n");
//        fprintf(out, "\nECC_U(P,X) :\n");
//        for(vector<Rule>::iterator eit = eccupx.begin(); eit != eccupx.end(); eit++) {
//            (*eit).output(out);
//        }
//        fprintf(out, "\n"); 
        
        
//        fprintf(out, "Half loops Es : \n");
//        for(vector<Loop>::iterator it = Es.begin(); it != Es.end(); it++) {
//            fprintf(out, "Loop : ");
//            for(set<int>::iterator sit = (it->loopNodes).begin(); sit != (it->loopNodes).end(); sit++) {
//                fprintf(out, "%s ", Vocabulary::instance().getAtom(*sit));
//            }
//            fprintf(out, "\n");
//        }
//        fprintf(out, "\n"); 
        
        if(Es.size() > 0) {
            // calculate R-(E,X)
            R_EX(Es.front(), Xtest);      //  printf("\nR_(E,X) done\n");
        
//            fprintf(out, "R-(E,X) : \n");
//            for(set<int>::iterator it = Es.front().ESRules.begin(); it != Es.front().ESRules.end(); it++) {
//                G_NLP[*it].output(out);
//            }
//            fprintf(out, "\n");
        }
//        printf("1 : \n"); Vocabulary::instance().dumpVocabulary(stdout);
        
        // calculate HL_U(P,X) will inside t_UPX(Xtest)
//        vector<Loop> hlupx = HL_UPX(Xtest);   //  printf("\nHL_U(P,X) done\n");
//        fprintf(out, "HL_U(P,X) :\n");
//        for(vector<Loop>::iterator it = hlupx.begin(); it != hlupx.end(); it++) {
//            fprintf(out, "Loop : ");
//            for(set<int>::iterator sit = (it->loopNodes).begin(); sit != (it->loopNodes).end(); sit++)
//                fprintf(out, "%s ", Vocabulary::instance().getAtom(*sit));
//            fprintf(out, "\n");
//        }
//        fprintf(out, "\n");         // tested OK for HL_U(P,X) is empty situation.
//        
//        printf("2 : \n"); Vocabulary::instance().dumpVocabulary(stdout);
        
//        fflush(out);
        // calculate t_U(P,X)
        vector<Rule> tupx = t_UPX(Xtest);    //   printf("\nt_U(P,X) done\n");
//        printf("3 : \n"); Vocabulary::instance().dumpVocabulary(stdout);
//        fprintf(out, "t_U(P,X) : \n");  fflush(out);
//        for(vector<Rule>::iterator it = tupx.begin(); it != tupx.end(); it++) {
//            (*it).output(out);
//        }
//        fprintf(out, "\n");         // tested OK for HL_U(P,X) is empty situation.

//        fflush(out);
        // calculate e_U(P,X)
        e_U(tupx, Xtest);
//        fprintf(out, "e_U(t_U(P,X), X): \n");
//        for(vector<Rule>::iterator it = tupx.begin(); it != tupx.end(); it++) {
//            (*it).output(out);
//        }
//        fprintf(out, "\n");         // tested OK for HL_U(P,X) is empty situation.


        // calculate answer sets Y
        vector<Rule> pY = eccupx;    // e_U(P, X)和ECC_U(P, X)的并集
        pY.insert(pY.begin(), tupx.begin(), tupx.end());
        long start2 = clock();
        YTime = (double)(start2 - start1) / CLOCKS_PER_SEC;
        set< set<int> > Ys = solutionYs(pY, Xtest);    // printf("Ys size %d\n", Ys.size());
        long start3 = clock();
        YTime += ((double)(start3 - start2) / CLOCKS_PER_SEC - YIOTime - ClaspDTime);
        YTime /= Ys.size();
        fprintf(out, "XTime + YTime = %lf\n", XTime + YTime);
        fprintf(out, "Answer set Ys num : %d\n", Ys.size()); //fflush(out);
//        for(set< set<int> >::iterator it = Ys.begin(); it != Ys.end(); it++) {
//            for(set<int>::iterator sit = (*it).begin(); sit != (*it).end(); sit++) {
//                fprintf(out, " %s", Vocabulary::instance().getAtom(*sit));  fflush(out);
//            }
//            fprintf(out, "\n");
//        }
//        fprintf(out, "\n");

//        fprintf(out, "\nInput program P's answer sets : \n");
//        for(set< set<int> >::iterator pit = PAnswerSets.begin(); pit != PAnswerSets.end(); pit++) {
//            for(set<int>::iterator it = (*pit).begin(); it != (*pit).end(); it++) {
//                fprintf(out, " %s", Vocabulary::instance().getAtom(*it));
//            }
//            fprintf(out, "\n");
//        }
//        fprintf(out, "\n");

        fprintf(out, "\nSolution judge \n");
        for(set< set<int> >::iterator sit = Ys.begin(); sit != Ys.end(); sit++) {
            bool judgeResult = judge(out, Xtest, (*sit));
//            break;      // 为了方便统计，先只对每组 Ys 都只计算第一个。
//            if(judgeResult) {
//                long end = clock();
//                YTime = (double)(end - start) / CLOCKS_PER_SEC;
//                fprintf(out, "XTime + YTime = %lf\n", XTime + YTime -YIOTime - ClaspDTime);
//            }
        }
        fprintf(out, "\n");
        //fflush(out);
    }
}

void DependenceGraph::calbuU() {
    b_UP();
    in_UP();
    out_UP();
    EC_UP();
}


// 纯粹用于测试各个函数的功能的
void DependenceGraph::testFunction(FILE* out, string argv1) {
    PAnswerSets = callClaspD(argv1);
    
    fprintf(out, "Input program P's answer sets : \n");
    for(set< set<int> >::iterator pit = PAnswerSets.begin(); pit != PAnswerSets.end(); pit++) {
        for(set<int>::iterator it = (*pit).begin(); it != (*pit).end(); it++) {
            fprintf(out, " %s", Vocabulary::instance().getAtom(*it));
        }
        fprintf(out, "\n");
    }
    fprintf(out, "\n");
    
    fprintf(out, "U size : %d\n", U.size());
    for(set<int>::iterator it = U.begin(); it != U.end(); it++)
        fprintf(out, "%s ", Vocabulary::instance().getAtom(*it));
    fprintf(out, "\n");
    
    // calculate b_U(P)
    b_UP();   //  printf("\nb_U(P) done!\n");
    fprintf(out, "\nb_U(P) : \n");
    for(set<int>::iterator it = bUP.begin(); it != bUP.end(); it++) {
        G_NLP[*it].output(out);
    }
    fprintf(out, "\n");
    
    // Atoms(b_U(P)) - U 的数目
    
    
    // calculate in_U(P)
    in_UP();   // printf("\nin_U(P) done!\n");
    fprintf(out, "in_U(P) : \n");
    for(set<int>::iterator it = inUP.begin(); it != inUP.end(); it++) {
        G_NLP[*it].output(out);
    }
    fprintf(out, "\n");
    
    // calculate out_U(P)
    out_UP(); //  printf("\nout_U(P) done!\n");
    fprintf(out, "out_U(P) : \n");
    for(set<int>::iterator it = outUP.begin(); it != outUP.end(); it++) {
        G_NLP[*it].output(out);
    }
    fprintf(out, "\n");
    
    // calculate EC_U(P)
    EC_UP();   // printf("\nEC_U(P) done!\n");
    fprintf(out, "EC_U(P) : \n");
    for(vector<Rule>::iterator it = ECUP.begin(); it != ECUP.end(); it++) {
        (*it).output(out);
    }
    fprintf(out, "\n");
    
    // calculate answer set X
    set< set<int> > Xs = solutionXs(); // printf("\nXs done!\n");
    fprintf(out, "Answer set Xs num : %d\n", Xs.size()); fflush(out);
    for(set< set<int> >::iterator it = Xs.begin(); it != Xs.end(); it++) {
        for(set<int>::iterator sit = (*it).begin(); sit != (*it).end(); sit++) {
            fprintf(out, " %s", Vocabulary::instance().getAtom(*sit));  fflush(out);
        }
        fprintf(out, "\n");
    }
    fprintf(out, "\n"); fflush(out);
    
    // calculate all half loops E
    calE();         
    fprintf(out, "Half loops Es : \n");
    for(vector<Loop>::iterator it = Es.begin(); it != Es.end(); it++) {
        fprintf(out, "Loop : ");
        for(set<int>::iterator sit = (it->loopNodes).begin(); sit != (it->loopNodes).end(); sit++) {
            fprintf(out, "%s ", Vocabulary::instance().getAtom(*sit));
        }
        fprintf(out, "\n");
    }
    fprintf(out, "\n"); 
    
    // answer set Y 
    for(set< set<int> >::iterator xit = Xs.begin(); xit != Xs.end(); xit++) {
        set<int> Xtest = (*xit);
        fprintf(out, "----------------------------\nChoose X : ");    //  printf("\nChoose X : \n");
        for(set<int>::iterator it = Xtest.begin(); it != Xtest.end(); it++)
            fprintf(out, "%s ", Vocabulary::instance().getAtom(*it));
        fprintf(out, "\n");
        fflush(out);
        
        // calculate ECC_U(P,X)
        vector<Rule> eccupx = ECC_UP(Xtest);   // printf("\nECC_U(P, X) done\n");
        fprintf(out, "\nECC_U(P,X) :\n");
        for(vector<Rule>::iterator eit = eccupx.begin(); eit != eccupx.end(); eit++) {
            (*eit).output(out);
        }
        fprintf(out, "\n"); 
        fflush(out);
        
        if(Es.size() > 0) {
            // calculate R-(E,X)
            R_EX(Es.front(), Xtest);      //  printf("\nR_(E,X) done\n");        
            fprintf(out, "R-(E,X) : \n");
            for(set<int>::iterator it = Es.front().ESRules.begin(); it != Es.front().ESRules.end(); it++) {
                G_NLP[*it].output(out);
            }
            fprintf(out, "\n");
        }
//        printf("1 : \n"); Vocabulary::instance().dumpVocabulary(stdout);
        
        // calculate HL_U(P,X) will inside t_UPX(Xtest)
        vector<Loop> hlupx = HL_UPX(Xtest);   //  printf("\nHL_U(P,X) done\n");
        fprintf(out, "HL_U(P,X) :\n");
        for(vector<Loop>::iterator it = hlupx.begin(); it != hlupx.end(); it++) {
            fprintf(out, "Loop : ");
            for(set<int>::iterator sit = (it->loopNodes).begin(); sit != (it->loopNodes).end(); sit++)
                fprintf(out, "%s ", Vocabulary::instance().getAtom(*sit));
            fprintf(out, "\n");
        }
        fprintf(out, "\n");         // tested OK for HL_U(P,X) is empty situation.
        
//        printf("2 : \n"); Vocabulary::instance().dumpVocabulary(stdout);
        
        fflush(out);
        // calculate t_U(P,X)
        vector<Rule> tupx = t_UPX(Xtest);    //   printf("\nt_U(P,X) done\n");
//        printf("3 : \n"); Vocabulary::instance().dumpVocabulary(stdout);
        fprintf(out, "t_U(P,X) : \n");  fflush(out);
        for(vector<Rule>::iterator it = tupx.begin(); it != tupx.end(); it++) {
            (*it).output(out);
        }
        fprintf(out, "\n");         // tested OK for HL_U(P,X) is empty situation.

        fflush(out);
        // calculate e_U(P,X)
        e_U(tupx, Xtest);
        fprintf(out, "e_U(t_U(P,X), X): \n");
        for(vector<Rule>::iterator it = tupx.begin(); it != tupx.end(); it++) {
            (*it).output(out);
        }
        fprintf(out, "\n");         // tested OK for HL_U(P,X) is empty situation.


        // calculate answer sets Y
        vector<Rule> pY = eccupx;    // e_U(P, X)和ECC_U(P, X)的并集
        pY.insert(pY.begin(), tupx.begin(), tupx.end());
        set< set<int> > Ys = solutionYs(pY, Xtest);    // printf("Ys size %d\n", Ys.size());
        fprintf(out, "Answer set Ys num : %d\n", Ys.size()); fflush(out);
        for(set< set<int> >::iterator it = Ys.begin(); it != Ys.end(); it++) {
            for(set<int>::iterator sit = (*it).begin(); sit != (*it).end(); sit++) {
                fprintf(out, " %s", Vocabulary::instance().getAtom(*sit));  fflush(out);
            }
            fprintf(out, "\n");
        }
        fprintf(out, "\n");



        fprintf(out, "\nSolution judge \n"); fflush(out);
        for(set< set<int> >::iterator sit = Ys.begin(); sit != Ys.end(); sit++) {
            judge(out, Xtest, (*sit));
        }
        fprintf(out, "\n");
        fflush(out);
    }
}

void DependenceGraph::setMPealSize(int s) {
    m_pealSize = s;
}



vector<Loop> DependenceGraph::CS_UP() {
    vector<Loop> CSUP;
    
    for(vector<Loop>::iterator it = Es.begin(); it != Es.end(); it++) {
        findESRules(*it);       // 会直接修改it本身的外部支持属
        if(includes(inUP.begin(), inUP.end(), (it->ESRules).begin(), (it->ESRules).end()))
            CSUP.push_back(*it);
    }
    
    return CSUP;
}


vector<Rule> DependenceGraph::ctUP() {
    vector<Rule> result;
    
//    printf("ctUp:\n");
    // Part 1
    set<int> union_;
    set_union(bUP.begin(), bUP.end(), outUP.begin(), outUP.end(), inserter(union_, union_.begin()));
    // P \ (bUP + outUP) 的rule集合
    for(int i = 0; i < G_NLP.size(); i++)
        if(union_.find(i) == union_.end())
            result.push_back(G_NLP[i]);
//     printf("phase 1\n");   
//    for(int i = 0; i < result.size();i++) result[i].output(stdout);

    // Part 2
    vector<Loop> csup = CS_UP();        // 这里已经改变了各个 E 的外部支持属性了
    int mark = 1;
    for(vector<Loop>::iterator it = csup.begin(); it != csup.end(); it++, mark++) {
        for(set<int>::iterator iit = (it->ESRules).begin(); iit != (it->ESRules).end(); iit++) {
            stringstream ss;    // 数字可能远超9,所以用stringstream好一些
            string tmp = "";
            string str("x");
            ss << mark;
            tmp = ss.str();
            str = "a" + tmp + str;  // 这里加个“a”是为了防止 clasp 读入失败
            char *input = new char[str.size() + 1];
            strcpy(input, (char*)(str.c_str())); input[str.size()] = '\0';
            int id = Vocabulary::instance().queryAtom(input);
            if(id == -1)
               id = Vocabulary::instance().addAtom(input);  
            Rule R_;        R_.head = id;
            R_.body_lits = G_NLP[*iit].body_lits; // 当前对应的in_U(P)中的rule
            R_.body_length = R_.body_lits.size();

            result.push_back(R_);
        }
    }
//     printf("phase 2\n");
//    for(int i = 0; i < result.size();i++) result[i].output(stdout);
//      
//    
    // part 3，当HL_U(P,X)为空时，直接插入out_U(P)，这个是确定的.
    if(csup.size() == 0) {
        for(set<int>::iterator it = outUP.begin(); it != outUP.end(); it++)
            result.push_back(G_NLP[*it]);
    }
    else {
        for(set<int>::iterator iit = outUP.begin(); iit != outUP.end(); iit++) {
            Rule R_;
            R_.head = G_NLP[*iit].head;
            R_.body_lits = G_NLP[*iit].body_lits;
            for(int i = 1; i <= csup.size(); i++) {
                set<int> bodyPlus;
                for(set<int>::iterator bit = G_NLP[*iit].body_lits.begin(); 
                        bit != G_NLP[*iit].body_lits.end(); bit++) {
                    if((*bit) >= 0)
                        bodyPlus.insert(*bit);
                }
                set<int> intersect;
                set_intersection(bodyPlus.begin(), bodyPlus.end(), csup.at(i-1).loopNodes.begin(),
                        csup.at(i-1).loopNodes.end(), inserter(intersect, intersect.begin()));
                if(intersect.size() > 0) {
                    stringstream ss;    
                    string tmp = "";   
                    string str("x");
                    ss << i;
                    tmp = ss.str();
//                    tmp += 'a' + i - 1;
                    str = "a" + tmp + str; 
                    char *input = new char[str.size() + 1];
                    strcpy(input, (char*)(str.c_str())); input[str.size()] = '\0';
                    int id = Vocabulary::instance().queryAtom(input);
                    if(id == -1)
                       id = Vocabulary::instance().addAtom(input); 
                    R_.body_lits.insert(id);
                }
            } 
            R_.body_length = R_.body_lits.size();
            result.push_back(R_);
        }
    }
     
//     printf("phase 3\n");
//    for(int i = 0; i < result.size();i++) result[i].output(stdout);
//    
    // Part 4
    int mark2 = 1;
    for(vector<Loop>::iterator it = csup.begin(); it != csup.end(); it++, mark2++) {
        stringstream ss;    
        string tmp = "";   
        string str("x");
        ss << mark2;
        tmp = ss.str();
        str = "a" + tmp + str; 
        char *input = new char[str.size() + 1];
        strcpy(input, (char*)(str.c_str())); input[str.size()] = '\0';
        int id = Vocabulary::instance().queryAtom(input);
        if(id == -1)
            id = Vocabulary::instance().addAtom(input);
        Rule R;
        R.type = CONSTRANT;
        R.body_lits.insert(-id);
        R.body_length = 1;
        result.push_back(R);
    }
//    printf("phase 4\n");
//    for(int i = 0; i < result.size();i++) result[i].output(stdout);
//    
    return result;
}


void DependenceGraph::trpPU(vector<Rule>& ctUP) {
    // Part 1
    for(vector<Rule>::iterator rit = ctUP.begin(); rit != ctUP.end(); rit++) {
        set<int> bodyMinu;
        for(set<int>::iterator sit = (rit->body_lits).begin(); sit != (rit->body_lits).end(); sit++)
            if((*sit) < 0)
                bodyMinu.insert(-(*sit));
        set<int> intersect;
        set_intersection(U.begin(), U.end(), bodyMinu.begin(), bodyMinu.end(), inserter(intersect, intersect.begin()));
        bool flag = false;
        // deleting rule that has an atom p in head(r) with p in L, too
        if(U.find(rit->head) != U.end()) {
            rit = ctUP.erase(rit);
            flag = true;
        }
        // deleting rule that has an atom p in body^-(r) with p in L, too
        if(intersect.size() > 0) {
            rit = ctUP.erase(rit);
            flag = true;
        }
        
        if(flag) rit++;
    }
    
    // Part 2
    for(vector<Rule>::iterator rit = ctUP.begin(); rit != ctUP.end(); rit++) {
        set<int> diff;
        set_difference((rit->body_lits).begin(), (rit->body_lits).end(), U.begin(), U.end(), inserter(diff, diff.begin()));
        rit->body_lits.clear();
        rit->body_lits = diff;
        if(rit->body_lits.size() == 0) rit->type = FACT;
    }
}


void DependenceGraph::application(FILE* out) {
    b_UP();
//    
//    printf("bUP \n");
//    for(set<int>::iterator it = bUP.begin(); it != bUP.end(); it++) {
//        G_NLP[*it].output(stdout);
//    }
//    printf("\n");
    in_UP();
//    printf("inUP \n");
//    for(set<int>::iterator it = inUP.begin(); it != inUP.end(); it++) {
//        G_NLP[*it].output(stdout);
//    }

    out_UP();
//    printf("outUP \n");
//    for(set<int>::iterator it = outUP.begin(); it != outUP.end(); it++) {
//        G_NLP[*it].output(stdout);
//    }
//    printf("\n");
    calE();
//    printf("ES: %d\n", Es.size());
//    
//    for(vector<Loop>::iterator it = Es.begin(); it != Es.end(); it++) {
//        (*it).print(stdout);
//    }
//    printf("\n");
    vector<Rule> ctup = ctUP();
    
   
    trpPU(ctup);
    
    for(int i = 0; i < ctup.size(); i++) {
        ctup[i].output(out);
    }
}