#ifndef DEPENDENCEGRAPH_H
#define	DEPENDENCEGRAPH_H

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <stack>
#include "Rule.h"
#include "Utils.h"

extern vector<Rule> G_NLP;

struct Loop {
    set<int> loopNodes;
    set<int> ESRules;   // 存放外部支持rule在G_NLP中的下标号，从0开始
    vector< set<int> > loopFormulas;
    
    Loop() {
        loopNodes.clear();
        loopFormulas.clear();
        ESRules.clear();
    }
    Loop(set<int> s) {
        loopNodes = s;
        loopFormulas.clear();
        ESRules.clear();
    }
    Loop(const Loop& l) {
        loopNodes = l.loopNodes;
        ESRules = l.ESRules;
        loopFormulas = l.loopFormulas;
    }
    
    ~Loop() {
        loopNodes.clear();
        loopFormulas.clear();
        ESRules.clear();
    }
    
    bool operator==(const Loop& l) {
        if(this->loopNodes.size() != l.loopNodes.size()) return false;
        
        for(set<int>::iterator it = this->loopNodes.begin(), lit = l.loopNodes.begin(); 
                it != this->loopNodes.end(); lit++, it++) {
            if(*it != *lit) return false;
        }
        
        return true;
    }
    
    int calHash() {
        int hash = 0;
       for(set<int>::iterator it = loopNodes.begin(); it != loopNodes.end(); it++) {
           hash += (*it) * (*it);
       } 
        
        return hash;
    }
    
    void print(FILE* out) {
        fprintf(out, "Loop: ");
        for(set<int>::iterator it = this->loopNodes.begin(); it != this->loopNodes.end(); it++) {
            fprintf(out, "%s ", Vocabulary::instance().getAtom(*it));
        }
        //fprintf(out, "\n");
        fprintf(out, "ES: ");
        for(set<int>::iterator it = this->ESRules.begin(); it != this->ESRules.end(); it++) {
           //G_NLP[*it].output(stdout);
            fprintf(out, "%d ", *it);
        }
        fprintf(out, "\n");
    }
};

class DependenceGraph {
public:
    DependenceGraph();
    DependenceGraph(const DependenceGraph& orig);
    ~DependenceGraph();
//    void test();                //nonsense, just for test
    
    int computeLoopFormulas(Loop& loop);
    void operateGraph();
    vector<Loop> getLoopWithESRuleSize(int k);
    vector<int> getESRSizes();
    
    //vector<Loop> loops;
    vector<Loop> SCCs;
    
    vector<Loop> findSCC();
    void findSCCInSCC(vector<Loop>& loops);
    vector<Loop> findCompMaximal(set<int> comp, int t);
    Loop findLoopMaximal(Loop scc);
    
    void findProperLoops(vector<Loop>& ploops, set<int>& s);
    void findElementaryLoops(vector<Loop>& ploops, set<int>& s);
    void findLoops(vector<Loop>& ploops, set<int>& s);
    
    vector<Loop> findAllProperLoops();
    vector<Loop> findAllElementaryLoops();
    vector<Loop> findAllLoops();
    
    void findProperSection(vector<Loop>& ploops, set<int>& s, bool self);
    void findProperSection(vector<Loop>& ploops, vector<Loop>& cloops, set<int>& cns, set<int>& s, set<int>& in, bool self);
    
    void findElSection(vector<Loop>& ploops, set<int>& s);
    int sectionHash(set<int>& s1, set<int>& s2, int m);
    int Hash(set<int>& s1);
    Loop findProperLoop(Loop scc);
    Loop searchProperLoop(Loop scc);
    Loop searchProperLoop(Loop scc, set<int> in);
    Loop searchElementary(Loop scc);
    Loop findElememtary(Loop scc);
    void findESRules(Loop& loop);
    
    int setRelation(const set<int>& es1, const set<int>& es2, set<int>& m);
    int setRelation(const set<int>& es1, const set<int>& es2);
    bool setCross(const set<int>& es1, const set<int>& es2);
    vector<Loop> findL1L2Conbimation();
    vector<Loop> findL1L2Conbimation2();
    vector<Loop> findProperLoopsWithCut();
    void findProperLoopsCuts(vector<Loop>& ploops, vector<Loop>& cloops, set<int>& s, set<int>& cutNodes, set<int>& in);
    vector<Loop> findProperLoopsWithCut2();
    
    bool isLoop(set<int> lns);
    int properTimes;
   
    
    /*
     * 计算splitting的过程函数
     */
    void b_UP();                                        // 计算出b_U(P)，P为G_NLP，U使用一个全局变量U。
    void in_UP();                                       // 计算出in_U(P)
    void out_UP();                                      // 计算出out_U(P)
    void EC_UP();                                       // 计算出EC_U(P)
    set< set<int> > solutionXs();                       // 计算出b_U(P)和EC_U(P)并集的所有answer set
    vector<Rule> ECC_UP(set<int> X);                    // 计算出ECC_U(P, X)
    void calE();                                        // 计算出half loop E，理论上存在多个                                 
    void R_EX(Loop& E, set<int> X);                     // 计算出R^-(E, X)
    vector<Loop> HL_UPX(set<int> X);                    // 计算出HL_U(P, X)
    vector<Rule> t_UPX(set<int> X);                     // 计算出t_U(P, X)
    void e_U(vector<Rule>& tuPX, set<int> X);           // 计算出e_U(P, X)
    set< set<int> > solutionYs(vector<Rule> pY, set<int> X);  // 计算出e_U(P, X)和ECC_U(P, X)并集的所有answer set
    set< set<int> > callClaspD(string fileName);        // 调用claspD来计算answer set，输入为一个文件
    bool judge(FILE* out, set<int> X, set<int> Y);      // 对一对solution<X,Y>进行判断是否对应的S是否是程序P的一个answer set
    void splitting(FILE* out, string argv1);            // 整个splitting的运行过程
    

    // Find U
    set<int> calU();                                    // the calU use dpdGraph to calculate the minCut set U
    void calbuU();                                      // 计算Atoms(b_U(P)) - U 的原子个数

    void testFunction(FILE* out, string argv1);                       // 用于对上述函数的单元测试
    void setMPealSize(int s);                                // 设置 m_pealSize的值。
    
    // Application : Program Simplification
    vector<Loop> CS_UP();                               // 计算得到 CS_U(P)
    vector<Rule> ctUP();                                        // 计算得到 ct_U(P)
    void trpPU(vector<Rule>& ctUP);                     // 计算得到 trp(ct_U(P),U)
    void application(FILE* out);                                 // 
    /*
     * END
     */
    
 private:
    vector<Rule> nlp;
    map<int, set<int> > dpdGraph;       // 原程序的有向正依赖图，key为head(r)，value为body^+(r)
    map<int, set<int> > dpdRules;       
    map<int, set<int> > udpdGraph;      // 原程序的无向正依赖图，key为Atoms(P)中的每一个，value为其连向的atoms
    
    map<int, vector<Loop> > loopWithESSize;
    
    int nodeNumber;
    int maxNode;
    
    void tarjan(int u, vector<Loop>& loops);

    //SCC
    bool *visit;
    bool *involved;
    int *DFN;
    int *Low;
    int Index;
    stack<int> vs;
    map<int, bool> loopHash;
    map<int, bool> secHash;
    map<int, bool> esHash;
    
    
    /*
     * 计算splitting的过程需要用到的变量
     * 以下变量都是直接根据输入程序P和指定的分割集
     */
    set<int> bUP;           // b_U(P)，保存的是G_NLP中的rule下标
    set<int> inUP;          // in_U(P)，保存的是G_NLP中的rule下标
    set<int> outUP;         // out_U(P)，保存的是G_NLP中的rule下标
    vector<Rule> ECUP;          // EC_U(P)
    vector<Loop> Es;            // all half loops E in P w.r.t. U
    int apsize;                 // 保存刚开始的Vocabulary的atom_lists的size，后面可以判断，一旦大于这个数值的都不是Atoms(P)中的元素。
    set< set<int> > PAnswerSets; // 输入程序P的answer set集合
    int m_pealSize;
    
    double XTime;
    double YTime;
    double XIOTime;
    double YIOTime;
    double ClaspDTime;  // callClaspD函数中提取答案的时间。
    /*
     * End
     */
};

#endif	/* DEPENDENCEGRAPH_H */
