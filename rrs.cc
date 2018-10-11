#include <utility>
#include <set>
#include <cstring>
#include "rrs.hh"

using namespace std;

enum OUTPUT_STYLE {
	OUTPUT_LIST,
	OUTPUT_DEPQBF,
	OUTPUT_COUNT
};

int main(int argc, char * argv[]) {
	OUTPUT_STYLE os = OUTPUT_LIST; 
	string formula_file = "";
	string query_file = "";
	bool count_trivial = false;
	for (int i = 1; i < argc; ++i) {
		string arg = argv[i];
		if (arg == "-h" || arg == "--help") {
			cout << "USAGE: rrs [-t|--count-trivial] [-c|--count-only] [-d|--depqbf-output] [-q|--query-file <query_file>] <formula>" << std::endl;
			return 0;
		} else if (arg == "-t" || arg == "--count-trivial") {
			count_trivial = true;
		} else if (arg == "-c" || arg == "--count-only") {
			os = OUTPUT_COUNT;
		} else if (os == OUTPUT_LIST && (arg == "-d" || arg == "--depqbf-output")) {
			os = OUTPUT_DEPQBF;
		} else if (arg == "-q" || arg == "--query-file") {
			if (i + 1 == argc) {
				std::cerr << "Missing query file name after " << arg << std::endl;
				return 3;
			}
			query_file = argv[++i];
		} else if (formula_file == "") {
			formula_file = argv[i];
		} else {
			cerr << "Unrecognized option: " << argv[i] << std::endl;
			return 1;
		}
	}
	if (formula_file == "") {
		cerr << "No filename specified, try 'rrs -h'." << std::endl;
		return 2;
	}

	ifstream formula_ifs(formula_file);
	Parser::Formula f(formula_ifs);
	formula_ifs.close();

	if (f.num_clauses > 2147483648) {
		cout << "The formula has too many clauses, unable to process. Only formulas with up to 2147483648 clauses are supported. Exiting.\n";
		return 4;
	}
	unsigned int nc = f.num_clauses;
	unsigned int num_lits = f.num_vars * 2;

	ifstream query_ifs(query_file);
	vector<pair<unsigned int, unsigned int>> queries;
	unsigned int on, of;
	while (query_ifs >> on) {
		query_ifs >> of;
		if (on > of) {
			swap(on, of);
		}
		if (of <= f.max_orig_var) {
			queries.push_back({f.var_conversion_map[on], f.var_conversion_map[of]});
		} else {
			cerr << "Invalid query, variable " << of << " does not exist\n";
			return 5;
		}
	}
	query_ifs.close();
	sort(queries.begin(), queries.end(), lexicographic);
	vector<bool> answers(queries.size());
	size_t next_query = 0;
	
	//initialize sets to track reachability
	vector<bool> reachable_true(num_lits);
	vector<bool> reachable_false(num_lits);

	//initialize occurence vectors
	vector<vector<unsigned int>> occ(num_lits);
	for (unsigned int i = 0; i < nc; i++) {
		for (unsigned int j = 0; j < f.clause_list[i].size; j++) {
			occ[LIT2IDX(f.clause_list[i].literals[j])].push_back(i);
		}
	}

	/*the stack used for the search for resolution paths
	 *instead of using a stack of pairs, we have a stack of uints only
	 *the meaning of each element is c + e*num_clauses, where e == 0
	 *if the clause is pushed the first time and e == 1 if it
	 *is the second time
	 *this means that the number of clauses is limited to 2^31=2147483648
	 */
	vector<unsigned int> stack;

	//initialize a structure to track whether a clause has been visited (non-zero value)
	//and if so, what was the first entry literal (value if non-zero)
	vector<int> visited(nc);

	//track how many times a clause has already been pushed on the stack
	//each clause is only allowed to be pushed twice--this is sufficient
	//to discover all resolution-path connections
	vector<unsigned char> entries(nc);

	//track whether a literal's occurences have already been explored
	//if yes, no need to check again
	vector<bool> explored(num_lits);

	//always check whether all possible connections from a literal have been found
	//already--if so, stop search immediately. Precompute the number of posible
	//endpoints here
	vector<unsigned int> max_deps(f.num_vars + 1);
	unsigned int num_vars_of_type[2] = {0, 0};
	unsigned int max_var_with_possible_deps = 0;
	for (unsigned int var = 1; var <= f.num_vars; var++) {
		unsigned int var_qtype = f.var_data[var].qtype;
		max_deps[var] = num_vars_of_type[1-var_qtype];
		num_vars_of_type[var_qtype]++;
	}
	/* print the number of trivial dependencies for verification
	 * unsigned long int triv_deps = num_vars_of_type[0] * num_vars_of_type[1];
	 * printf("%lu\n", triv_deps);
	 */
	for (unsigned int var = 1; var <= f.num_vars; var++) {
		unsigned int var_qtype = f.var_data[var].qtype;
		// multiply by 2, because that is the number of *literals* right of var
		max_deps[var] = 2*(num_vars_of_type[1-var_qtype] - max_deps[var]);
		if (max_deps[var] > 0)
			max_var_with_possible_deps = var;
	}

	//do the actual computation
	unsigned long int total = 0;
	for (unsigned int v = 1; v <= max_var_with_possible_deps; v++) {
		reachable_true.assign(num_lits, false);
		reachable_false.assign(num_lits, false);
		
		visited.assign(nc, 0);
		entries.assign(nc, 0);
		explored.assign(num_lits, false);
		get_deps(f, v, max_deps[v], occ, reachable_true, visited, entries, explored, stack);

		visited.assign(nc, 0);
		entries.assign(nc, 0);
		explored.assign(num_lits, false);
		get_deps(f, -v, max_deps[v], occ, reachable_false, visited, entries, explored, stack);
		if (os == OUTPUT_DEPQBF) {
			cout << f.var_data[v].orig_num << ":";
		}
		for (unsigned int x = (v-1)*2; x < num_lits; x += 2) {
			bool is_dependency = false;
			unsigned int xvar = IDX2VAR(x);
			if ((reachable_true[x] && reachable_false[x^1]) || (reachable_true[x^1] && reachable_false[x])) {
				is_dependency = true;
				if (os == OUTPUT_LIST) {
					cout << f.var_data[v].orig_num << " " << f.var_data[xvar].orig_num << endl;
				} else if (os == OUTPUT_DEPQBF) {
					cout << " " << f.var_data[xvar].orig_num;
				}
				++total;
			}
			pair<unsigned int, unsigned int> current_pair = {v, xvar};
			while (next_query < queries.size() && lexicographic(queries[next_query], current_pair)) {
				answers[next_query++] = false;
			}	
			if (next_query < queries.size() && queries[next_query] == current_pair) {
				answers[next_query++] = is_dependency;
			}
			reachable_true[x] = reachable_true[x^1] = reachable_false[x] = reachable_false[x^1] = false;
		}
		if (os == OUTPUT_DEPQBF) {
			cout << " 0" << endl;
		}
	}

	if (count_trivial) {
		cout << "Trivial deps: " << f.trivial_deps() << std::endl;
	}

	if (os == OUTPUT_COUNT) {
		cout << "RRS deps:     " << total << std::endl;
	}
	
	if (!queries.empty()) {
		cout << "Query answers:" << std::endl;
		for (size_t i = 0; i < queries.size(); i++) {
			if (f.var_data[queries[i].first].qtype == f.var_data[queries[i].second].qtype) {
				cout << "Q ";
			} else if (answers[i]) {
				cout << "Y ";
			} else {
				cout << "N ";
			}
			cout << f.var_data[queries[i].first].orig_num << " " << f.var_data[queries[i].second].orig_num << endl;
		}
	}
}

/*bool compare_lits_by_qdepth(int candidate, int anchor) {
	return f.var_data[abs(candidate)].qdepth < f.var_data[abs(anchor)].qdepth;
}*/

bool compare_lits_by_name(int candidate, int anchor) {
	return abs(candidate) < abs(anchor);
}

bool lexicographic (const pair<unsigned int, unsigned int>& a, const pair<unsigned int, unsigned int>& b) {
	return a.first < b.first || (a.first == b.first && a.second < b.second);
}

void get_deps(Parser::Formula& f, int lit, unsigned int max_connections, vector<vector<unsigned int>>& occ, vector<bool>& reachable, vector<int>& visited, vector<unsigned char>& entries, vector<bool>& explored, vector<unsigned int>& stack) {
	unsigned int nc = f.num_clauses;
	unsigned int litidx = LIT2IDX(lit);
	unsigned int var = abs(lit);
	bool litqtype = f.var_data[var].qtype;
	// unsigned int litqdepth = f->var_data[var].qdepth;
	unsigned int connections_found = 0;

	explored[litidx] = true;
	for (unsigned int c : occ[litidx]) {
		stack.push_back(c);
		visited[c] = lit;
		entries[c] = 1;
	}

	while (!stack.empty()) {
		unsigned int current_clause = stack.back();
		unsigned int which_entry = 1;
		if (current_clause >= nc) {
			which_entry = 2;
			current_clause -= nc;
		}
		stack.pop_back();
		int first_entry_literal = visited[current_clause];
		int first_entry_variable = abs(first_entry_literal);

		if (which_entry == 2) {
			if (f.var_data[first_entry_variable].qtype != litqtype) {
				int felidx = LIT2IDX(first_entry_literal);
				if (!reachable[felidx]) {
					connections_found++;
					reachable[felidx] = true;
					if (connections_found == max_connections) {
						stack.clear();
						return;
					}
				}
			}
			int negfel = -first_entry_literal;
			int negfelidx = LIT2IDX(negfel);
			if (f.var_data[first_entry_variable].qtype && !explored[negfelidx]) {
				explored[negfelidx] = true;
				for (unsigned int c : occ[negfelidx]) {
					if (entries[c] == 0) {
						stack.push_back(c);
						visited[c] = negfel;
						entries[c] = 1;
					}
					else if (entries[c] == 1 && visited[c] != negfel) {
						stack.push_back(c + nc);
						entries[c] = 2;
					}
				}
			}
			continue;
		}

		//note that in this case by definition which_entry == 1
		//and therefore the current entry literal is first_entry_literal

		Parser::Constraint& clause = f.clause_list[current_clause];
		int * begin = clause.literals;
		int * end = begin + clause.size;
		/* locate the quantifier level of lit within clause,
		 * in order to search only deeper literals
		 *
		 * however, not sure whether binary search is best
		 *
		 * !!!IMPORTANT CHANGE!!!
		 * actually, according to the definition in Friedrich's thesis,
		 * it is sufficient to search through variables to the right of lit
		 * in a total order on all variables, including a "tiebreak"
		 * within individual quantifier blocks. This means that permutations
		 * within blocks can change the relation...
		 */
		//int * pos = lower_bound(begin, end, lit, &compare_lits_by_qdepth);
		int * pos = lower_bound(begin, end, lit, &compare_lits_by_name);
		if (pos != end && *pos == lit) {
			++pos;
		}
		while (pos != end) {
			int poslit = *pos;
			int negposlit = -poslit;
			unsigned int posvar = abs(poslit);
			unsigned int negposidx = LIT2IDX(negposlit);
			if (poslit != first_entry_literal) {
				if (f.var_data[posvar].qtype && !explored[negposidx]) {
					explored[negposidx] = true;
					//variable is existential and is not the entry literal, extend search
					for (unsigned int c : occ[LIT2IDX(negposlit)]) {
						if (entries[c] == 0) {
							stack.push_back(c);
							visited[c] = negposlit;
							entries[c] = 1;
						}
						else if (entries[c] == 1 && visited[c] != negposlit) {
							stack.push_back(c + nc);
							entries[c] = 2;
						}
					}
				}
				if (litqtype != f.var_data[posvar].qtype) {
					int poslitidx = LIT2IDX(poslit);
					if (!reachable[poslitidx]) {
						connections_found++;
						reachable[poslitidx] = true;
						if (connections_found == max_connections) {
							stack.clear();
							return;
						}
					}
				}
			}
			pos++;
		}
	}
}
