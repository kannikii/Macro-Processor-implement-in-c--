#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <cctype>
using namespace std;

/* ---------- 문자열 처리 ---------- */
static inline string trim(const string& s){
    size_t i = 0, j = s.size();
    while (i < j && isspace((unsigned char)s[i])) i++;
    while (j > i && isspace((unsigned char)s[j - 1])) j--;
    return s.substr(i, j - i);
}
static inline string upperStr(string s){
    for (char& c : s) c = toupper((unsigned char)c);
    return s;
}
static inline string rep(string s, const string& f, const string& t){
    if (f.empty()) return s;
    size_t p = 0;
    while ((p = s.find(f, p)) != string::npos){
        s.replace(p, f.size(), t);
        p += t.size();
    }
    return s;
}

/* ---------- 1줄 파싱 ---------- */
struct PL { bool lab=false; string label, op, opd; };
static PL parse(const string& l){
    PL p; string t = trim(l);
    if (t.empty()) return p;
    p.lab = !isspace((unsigned char)l[0]);
    stringstream ss(t);
    if (p.lab) ss >> p.label >> p.op; else ss >> p.op;
    string r; getline(ss, r); p.opd = trim(r);

    if (p.lab){
        bool noOp = p.op.empty();
        bool badOp = (!p.op.empty() && !isalpha((unsigned char)p.op[0]) && p.op[0] != '+' && p.op[0] != '-');
        if (noOp || badOp){
            string rest = p.op;
            if (!p.opd.empty()){
                if (!rest.empty()) rest += " ";
                rest += p.opd;
            }
            p.op = p.label;
            p.label.clear();
            p.lab = false;
            p.opd = trim(rest);
        }
    }
    return p;
}

/* ---------- 인자 파싱 ---------- */
static vector<string> args(const string& s){
    vector<string> v;
    string cur;
    int par = 0;
    bool qt = false;
    char qc = 0;

    auto push = [&](){ v.push_back(trim(cur)); cur.clear(); };

    for (size_t i = 0; i < s.size(); i++){
        char c = s[i];

        if (c=='\'' || c=='"'){
            if (!qt){ qt = true; qc = c; }
            else if (qc == c) qt = false;
            cur.push_back(c);
            continue;
        }

        if (!qt){
            if (c == '(') { par++; cur.push_back(c); continue; }
            if (c == ')') { par--; cur.push_back(c); continue; }
            if (c == ',' && par == 0){ push(); continue; }
        }
        cur.push_back(c);
    }

    push();
    return v;
}

/* ---------- Alpha label generator ($AA, $AB, ...) ---------- */
static string alphaLabel(int n){
    n = n % (26 * 26);
    char a = 'A' + (n / 26);
    char b = 'A' + (n % 26);
    string s; s.push_back(a); s.push_back(b);
    return s;
}

/* ---------- 매크로 테이블 구조 ---------- */
struct NE {
    int st = -1, en = -1;
    vector<string> prm;
    unordered_map<string,int> idx;
    unordered_map<string,string> defv;
};

vector<string> DEFTAB;
unordered_map<string, NE> NAMTAB;
vector<string> INTER;

/* ---------- 출력 포맷 ---------- */
static string fmt(const string& L, const string& O, const string& P){
    string s;
    if (!L.empty()){
        s += L;
        if (L.size() < 9) s.append(9 - L.size(), ' ');
        else s.push_back(' ');
    }
    else s.append(9, ' ');
    s += O; if (!O.empty() && O.size() < 9) s.append(9 - O.size(), ' ');
    if (!P.empty()) s += P;
    return s;
}

/* ---------- 매개변수 접합 처리 (->) ---------- */
static string concatOp(string s){
    size_t p = 0;
    while ((p = s.find("->", p)) != string::npos){
        if (p > 0 && s[p - 1] == ' '){ s.erase(p - 1, 1); p--; }
        s.erase(p, 2);
        if (p < s.size() && s[p] == ' ') s.erase(p, 1);
    }
    return s;
}

/* ---------- 고유 라벨 생성 ($ → $AA) ---------- */
static string locLabel(string s, const string& u){
    for (size_t i = 0; i < s.size(); i++){
        if (s[i] == '$'){
            size_t j = i + 1;
            if (j < s.size() && (isalnum((unsigned char)s[j]) || s[j]=='_')){
                s.insert(j, u);
                i += u.size();
            }
        }
    }
    return s;
}

static string replaceArgs(const string& src, const vector<string>& A){
    string out;
    for (size_t i = 0; i < src.size(); ){
        if (src[i]=='#'){
            size_t j = i+1;
            int num = 0;
            bool has = false;
            while (j < src.size() && isdigit((unsigned char)src[j])){
                has = true;
                num = num * 10 + (src[j]-'0');
                j++;
            }
            if (has && j < src.size() && src[j]=='@' &&
                num > 0 && num < (int)A.size()){
                out += A[num];
                i = j + 1;
                continue;
            }
        }
        out.push_back(src[i++]);
    }
    return out;
}

/* ---------- 조건부 처리 ---------- */
static string unq(string x){
    x = trim(x);
    if (x.size()>=2 &&
       ((x.front()=='\'' && x.back()=='\'') ||
        (x.front()=='"' && x.back()=='"')))
        return x.substr(1, x.size()-2);
    return x;
}
static string strip(string x){
    x = trim(x);
    if (!x.empty() && x.front()=='(' && x.back()==')')
        return strip(x.substr(1, x.size()-2));
    return x;
}
static bool condEval(string cond, const NE& ne, const vector<string>& A){
    cond = replaceArgs(cond, A);
    for (size_t i = 0; i < ne.prm.size(); i++)
        cond = rep(cond, ne.prm[i], A[i+1]);
    cond = strip(cond);

    size_t p;
    string L,R,O;

    if ((p = cond.find(" EQ ")) != string::npos){
        O="EQ"; L=cond.substr(0,p); R=cond.substr(p+4);
    }
    else if ((p = cond.find(" NE ")) != string::npos){
        O="NE"; L=cond.substr(0,p); R=cond.substr(p+4);
    }
    else if (cond.rfind("EQ ", 0) == 0){
        O="EQ"; L=""; R=cond.substr(3);
    }
    else if (cond.rfind("NE ", 0) == 0){
        O="NE"; L=""; R=cond.substr(3);
    }
    else return false;

    L = trim(L); R = trim(R);
    string Lv = unq(L), Rv = unq(R);

    auto blank = [&](string v){
        return (v=="" || v=="' '" || v=="\" \"" || v=="''");
    };

    if (blank(L)) Lv = "";
    if (blank(R)) Rv = "";

    bool eq = (Lv == Rv);
    return (O == "EQ") ? eq : !eq;
}

/* ---------- Pass2 ---------- */
static int SYS = 0;

static void expand(const string& line, vector<string>& out, int d);

static void expandM(const string& name, const vector<string>& ac,
                    vector<string>& out, int d)
{
    auto it = NAMTAB.find(name);
    if (it == NAMTAB.end()) return;
    const NE& ne = it->second;
    if (d > 1000) return;

    string UL = alphaLabel(SYS++);

    unordered_map<string,string> K;
    vector<string> P;

    for (auto a : ac){
        a = trim(a);
        size_t e = a.find('=');
        if (e != string::npos){
            string k = trim(a.substr(0,e));
            string v = trim(a.substr(e+1));
            if (!k.empty() && k[0] != '&') k = "&"+k;
            k = upperStr(k);
            K[k] = v;
        } else P.push_back(a);
    }

    vector<string> A(ne.prm.size()+1);
    for (size_t i = 0; i < ne.prm.size(); i++){
        string pn = ne.prm[i], v;
        string key = upperStr(pn);
        if (K.count(key)) v = K[key];
        else if (i < P.size()) v = P[i];
        else if (ne.defv.count(pn)) v = ne.defv.at(pn);
        A[i+1] = v;
    }

    vector<bool> STK;

    auto active = [&](){
        for (bool b : STK) if (!b) return false;
        return true;
    };

    for (int i = ne.st; i <= ne.en; i++){
        string raw = DEFTAB[i];
        PL p = parse(raw);

        if (p.op == "IF"){
            STK.push_back(condEval(p.opd, ne, A));
            continue;
        }
        if (p.op == "ELSE"){
            if (!STK.empty()) STK.back() = !STK.back();
            continue;
        }
        if (p.op == "ENDIF"){
            if (!STK.empty()) STK.pop_back();
            continue;
        }
        if (!active()) continue;

        string s = raw;
        s = locLabel(s, UL);
        s = concatOp(s);

        s = replaceArgs(s, A);

        for (size_t k = 0; k < ne.prm.size(); k++)
            s = rep(s, ne.prm[k], A[k+1]);

        expand(s, out, d+1);
    }
}

static void expand(const string& line, vector<string>& out, int d){
    PL p = parse(line);
    if (p.op.empty()){ out.push_back(line); return; }
    if (NAMTAB.count(p.op)){
        expandM(p.op, args(p.opd), out, d);
    } else {
        out.push_back(fmt(p.lab?p.label:"", p.op, p.opd));
    }
}

/* ---------- main (Pass1 + Pass2) ---------- */
int main(){
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    vector<string> L;
    string line;
    while (getline(cin, line)) L.push_back(line);

    bool inM = false;
    string name;
    NE cur;

    for (auto& l : L){
        PL p = parse(l);

        if (!inM){
            if (p.op == "MACRO"){
                inM = true; name = p.label;
                cur = NE(); cur.st = DEFTAB.size();
                vector<string> ps = args(p.opd);

                for (size_t i=0;i<ps.size();i++){
                    string v = ps[i], d="";
                    size_t e = v.find('=');
                    if (e!=string::npos){ d = trim(v.substr(e+1)); v = trim(v.substr(0,e)); }
                    if (!v.empty() && v[0] != '&') v = "&"+v;
                    cur.prm.push_back(v);
                    cur.idx[v] = i+1;
                    if (!d.empty()) cur.defv[v] = d;
                }
            }
            else INTER.push_back(l);
        }
        else {
            if (p.op == "MEND"){
                inM = false;
                cur.en = DEFTAB.size()-1;
                NAMTAB[name] = cur;
            }
            else {
                string b = l;
                for (size_t i=0;i<cur.prm.size();i++)
                    b = rep(b, cur.prm[i], "#"+to_string(i+1)+"@");
                DEFTAB.push_back(b);
            }
        }
    }

    vector<string> out;
    for (auto& l : INTER) expand(l, out, 0);
    for (auto& x : out) cout << x << "\n";

    return 0;
}
