#include <iostream>
#include <string>
#include <stack>
#include <cctype>
#include <vector>
#include <unordered_map>

using namespace std;

// operator precedence
int precedence(char op) {
    if (op == '*' || op == '/') return 2;
    if (op == '+' || op == '-') return 1;
    return 0;
}

// safe arithmetic (protects against div by zero)
int applyOp(int a, int b, char op) {
    switch(op){
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return b == 0 ? 0 : a / b; 
    }
    return 0;
}

// check if string is a valid number
bool isNumber(const string& s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (!isdigit(c)) return false;
    }
    return true;
}

// extract numbers, vars, and operators from the raw string
vector<string> tokenize(const string& expr) {
    vector<string> tokens;
    for (size_t i = 0; i < expr.size(); ++i) {
        if (isspace(expr[i])) continue;
        if (isalnum(expr[i]) || expr[i] == '_') {
            string token;
            while (i < expr.size() && (isalnum(expr[i]) || expr[i] == '_')) {
                token += expr[i++];
            }
            i--; 
            tokens.push_back(token);
        } else {
            tokens.push_back(string(1, expr[i]));
        }
    }
    return tokens;
}

// core shunting-yard evaluation for a single operator
void processOp(stack<string>& opd1, stack<string>& opd2, stack<char>& opr,
               vector<string>& out1, vector<string>& out2,
               int& tcount, int& pcount) {
    
    char op = opr.top(); opr.pop();
    
    // pop operands for unoptimized output
    string b1 = opd1.top(); opd1.pop();
    string a1 = opd1.top(); opd1.pop();
    
    // pop operands for optimized output
    string b2 = opd2.top(); opd2.pop();
    string a2 = opd2.top(); opd2.pop();

    // 1. generate standard 3-address code for output 1
    string temp1 = "t" + to_string(tcount++);
    out1.push_back(temp1 + "=" + a1 + op + b1);
    opd1.push(temp1);

    // 2. constant propagation for output 2
    bool a_is_const = isNumber(a2);
    bool b_is_const = isNumber(b2);

    if (a_is_const && b_is_const) {
        // fully evaluate if both are numbers and push result back
        int res = applyOp(stoi(a2), stoi(b2), op);
        opd2.push(to_string(res)); 
    } else {
        // otherwise, generate an optimized temp var ('p')
        string temp2 = "p" + to_string(pcount++);
        out2.push_back(temp2 + "=" + a2 + op + b2);
        opd2.push(temp2);
    }
}

int main() {
    string line;
    vector<string> out1;
    vector<string> out2;
    unordered_map<string, int> env; // tracks constants across lines
    
    int tcount = 1;
    int pcount = 1;

    // process statements line by line
    while (getline(cin, line)) {
        if (line.empty()) continue;

        // check if it's a print statement
        if (line.rfind("print", 0) == 0) {
            string varname = line.substr(5);
            varname.erase(0, varname.find_first_not_of(" \t"));
            varname.erase(varname.find_last_not_of(" \t") + 1);
            
            out1.push_back("print " + varname);
            out2.push_back("print " + varname);
            continue;
        }

        // otherwise, it's an assignment: var = expr
        size_t eqPos = line.find('=');
        if (eqPos == string::npos) continue;

        string varname = line.substr(0, eqPos);
        varname.erase(0, varname.find_first_not_of(" \t"));
        varname.erase(varname.find_last_not_of(" \t") + 1);

        string expr = line.substr(eqPos + 1);
        vector<string> tokens = tokenize(expr);

        stack<string> opd1, opd2;
        stack<char> opr;

        // shunting-yard parsing
        for (const string& token : tokens) {
            if (isalnum(token[0])) {
                opd1.push(token); // output 1 gets the raw token
                
                // output 2 injects known constants
                if (!isNumber(token) && env.count(token)) {
                    opd2.push(to_string(env[token]));
                } else {
                    opd2.push(token);
                }
            } else if (token == "(") {
                opr.push('(');
            } else if (token == ")") {
                while (!opr.empty() && opr.top() != '(') {
                    processOp(opd1, opd2, opr, out1, out2, tcount, pcount);
                }
                if (!opr.empty()) opr.pop(); 
            } else { // operator
                char op = token[0];
                while (!opr.empty() && precedence(opr.top()) >= precedence(op)) {
                    processOp(opd1, opd2, opr, out1, out2, tcount, pcount);
                }
                opr.push(op);
            }
        }

        // process remaining operators
        while (!opr.empty()) {
            processOp(opd1, opd2, opr, out1, out2, tcount, pcount);
        }

        // finalize output 1 (standard 3AC)
        if (!opd1.empty()) {
            string final1 = opd1.top(); opd1.pop();
            out1.push_back(varname + "=" + final1);
        }

        // finalize output 2 (optimized)
        if (!opd2.empty()) {
            string final2 = opd2.top(); opd2.pop();
            
            // squash redundant assignments (e.g., turn `p2=p1-1` \n `x=p2` into `x=p1-1`)
            if (!isNumber(final2) && !out2.empty() && out2.back().substr(0, final2.size() + 1) == final2 + "=") {
                string rhs = out2.back().substr(final2.size() + 1);
                out2.pop_back();
                out2.push_back(varname + "=" + rhs);
            } else {
                out2.push_back(varname + "=" + final2);
            }

            // track constants for future lines
            if (isNumber(final2)) {
                env[varname] = stoi(final2);
            } else {
                env.erase(varname); // delete if reassigned to a non-constant
            }
        }
    }

    cout << "Output I:\n";
    for (const string& s : out1) {
        cout << s << "\n";
    }

    cout << "Output II:\n";
    for (const string& s : out2) {
        cout << s << "\n";
    }

    return 0;
}