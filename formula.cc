#include "formula.hh"

namespace Parser {

bool QTYPE_FORALL = false;
bool QTYPE_EXISTS = true;

bool compare_literals(int x, int y) {
    x = x < 0 ? -x : x;
    y = y < 0 ? -y : y;
    return x < y;
}

Formula::Formula(istream& ifs) {
    /*
     * Parse a formula from a qdimacs file.
     */
    string token = "q";

    // Search until the tokens 'p' and 'cnf' are found.
    while (token != "p")
        ifs >> token;
    assert(token == "p");

    ifs >> token;
    assert(token == "cnf");
    
    // the declared number of clauses: when reading from the standard input, no more than this many will be read
    unsigned int m;

    ifs >> max_orig_var;
    ifs >> m;

    var_conversion_map.resize(max_orig_var + 1);
	var_data = {{0,0,0}};
    num_vars = 0;
    num_clauses = 0;
    bool current_qtype;
    unsigned int current_qdepth = 0;

    Variable current_var_data;
    
    bool empty_formula = true;

    // Read the prefix.
    while (ifs >> token) {
        if (token == "a") {
            current_qtype = QTYPE_FORALL;
        } else if (token == "e") {
            current_qtype = QTYPE_EXISTS;
        } else {
            /* The prefix has ended.
             * We have, however, already read the first literal of the first clause.
             * Therefore, when we jump out of the loop, we must take that literal into account.
             */
            empty_formula = false;
            break;
        }

        current_qdepth++;
        ifs >> current_var_data.orig_num;
        while (current_var_data.orig_num != 0) {
            num_vars++;
            var_conversion_map[current_var_data.orig_num] = num_vars;
            current_var_data.qdepth = current_qdepth;
            current_var_data.qtype = current_qtype;
            var_data.push_back(current_var_data);
            ifs >> current_var_data.orig_num;
        }
    }
        
    // Handle the case of an empty formula
    if (empty_formula) {
        return;
    }
    
    int literal = stoi(token);
    vector<int> temp_clause;
    Constraint current_clause;

    // Read the matrix.
    do {
        while(literal != 0) {
            // Convert the read literal into the corresponding literal according to the renaming of variables.
            temp_clause.push_back(literal > 0 ? var_conversion_map[literal] : -var_conversion_map[-literal]);
            ifs >> literal;
        }
        // apply universal reduction
        sort(temp_clause.begin(), temp_clause.end(), compare_literals);
        while (var_data[temp_clause.back() > 0 ? temp_clause.back() : -temp_clause.back()].qtype == QTYPE_FORALL) {
            temp_clause.pop_back();
        }
        // Copy clause to an array and append to clause_list.
        current_clause.size = temp_clause.size();
        current_clause.literals = new int[current_clause.size];
        size_t i = 0;
        for (vector<int>::iterator it = temp_clause.begin(); it != temp_clause.end(); ++it) {
            current_clause.literals[i] = *it;
            i++;
        }
        //sort(current_clause.literals, current_clause.literals + current_clause.size, compare_literals);
        clause_list.push_back(current_clause);
        num_clauses++;
        temp_clause.clear();
    } while((&ifs != &std::cin || num_clauses < m) && ifs >> literal);
}

void Formula::write(string filename) {
    ofstream ofs(filename, ofstream::out);
    ofs << "p cnf " << num_vars << " " << num_clauses << "\n";
    bool current_qtype = var_data[1].qtype;
    unsigned int i = 1;
    if (current_qtype == QTYPE_FORALL)
        ofs << "a";
    else
        ofs << "e";
    while(true) {
        while(i <= num_vars && var_data[i].qtype == current_qtype) {
            ofs << " " << i;
            i++;
        }
        if (i > num_vars) {
            ofs << " 0\n";
            break;
        }
        current_qtype = var_data[i].qtype;
        if (current_qtype == QTYPE_FORALL)
            ofs << " 0\na";
        else
            ofs << " 0\ne";
    }
    for (unsigned int i = 0; i < num_clauses; i++) {
        for (unsigned int j = 0; j < clause_list[i].size; j++) {
            ofs << clause_list[i].literals[j] << " ";
        }
        ofs << "0\n";
    }
}

unsigned int Formula::trivial_deps() {
	unsigned int num_vars_of_type[2] = {0,0};
	for (unsigned int v = 1; v <= num_vars; ++v) {
		num_vars_of_type[var_data[v].qtype]++;
	}
	return num_vars_of_type[0] * num_vars_of_type[1];
}

void print_clauses(Formula * f) {
    for (unsigned int i = 0; i < f->num_clauses; i++) {
        for (unsigned int j = 0; j < f->clause_list[i].size; j++) {
            printf("%d ", f->clause_list[i].literals[j]);
        }
        printf("\n");
    }
}
}
