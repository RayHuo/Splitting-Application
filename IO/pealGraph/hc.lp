% Hamiltonian circuit problem by Ilkka Niemelä 
%
% requires the graph as facts vertex(.) and arc(.,.).
% Usage, e.g.,  
% UNIX>  cat hc.lp graphs/p20 | lparse -1 -d none | smodels

% Facts hc(X,Y) in the stable models provide a Hamiltonian circuit.


hc(V1,V2) :- arc(V1,V2), not otherroute(V1,V2).
otherroute(V1,V2) :- arc(V1,V2), arc(V1,V3), hc(V1,V3), V2 != V3.
otherroute(V1,V2) :- arc(V1,V2), arc(V3,V2), hc(V3,V2), V1 != V3.
reached(V2) :- arc(V1,V2), hc(V1,V2), reached(V1), not initialnode(V1). 
reached(V2) :- arc(V1,V2), hc(V1,V2), initialnode(V1).
initialnode(0).

:- vertex(V), not reached(V).