#ifndef FORMULA_H
#define FORMULA_H

#include <vector>
#include <iostream>
#include <string>
#include <fstream>
#include <istream>
#include <memory>
#include <algorithm>

// uncomment to disable assert()
#define NDEBUG
#include <cassert>

using namespace std;

namespace Parser {

//extern bool QTYPE_FORALL;
//extern bool QTYPE_EXISTS;;

typedef struct {
	unsigned int size;
	int * literals;
} Constraint;

typedef struct {
	unsigned int orig_num;
	unsigned int qdepth;
	bool qtype;
} Variable;

class Formula {
	public:
		unsigned int num_clauses;
		unsigned int num_vars;
		unsigned int max_orig_var;
		vector<Constraint> clause_list;
		vector<Variable> var_data;
		vector<unsigned int> var_conversion_map;

		Formula(istream& ifs = cin);
		void write(string filename);
		unsigned int trivial_deps();
};

//Formula * read_formula(istream& ifs = cin);
void print_clauses(Formula * f);

}

#endif
