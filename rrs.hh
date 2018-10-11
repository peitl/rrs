#ifndef RRS_H
#define RRS_H

#include "formula.hh"

bool compare_lits (int, int);
bool lexicographic (const pair<unsigned int, unsigned int>&, const pair<unsigned int, unsigned int>&);
void get_deps (Parser::Formula& f, int, unsigned int, vector<vector<unsigned int>>&, vector<bool>&, vector<int>& visited, vector<unsigned char>& entries, vector<bool>& explored, vector<unsigned int>& stack);

inline unsigned int LIT2IDX(int lit) {
	if (lit < 0)
		return (-2*lit) - 1;
	return 2*(lit-1);
}

inline int IDX2LIT(unsigned int idx) {
	if (idx % 2 == 0)
		return idx / 2 + 1;
	return (idx + 1) / (-2);
}

inline unsigned int IDX2VAR(unsigned int idx) {
	return idx / 2 + 1;
}

#endif
