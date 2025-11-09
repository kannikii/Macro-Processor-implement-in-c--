#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cctype>
using namespace std;

/* ---------- 문자열 유틸 ---------- */

string trimLeft(const string &s) {
    size_t i = 0;
    while (i < s.size() && isspace((unsigned char)s[i])) i++;
    return s.substr(i);
}

string trimRight(const string &s) {
    if (s.empty()) return s;
    size_t i = s.size();
    while (i > 0 && isspace((unsigned char)s[i - 1])) i--;
    return s.substr(0, i);
}

string trim(const string &s) {
    return trimRight(trimLeft(s));
}

string replaceAll(string s, const string &from, const string &to) {
    if (from.empty()) return s;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

/* ---------- 데이터 구조: NAMTAB / DEFTAB ---------- */

struct NameEntry {
    int defStart = -1;
    int defEnd   = -1;
    vector<string> params;                  // "&INDEV", "&BUFADR", ...
    unordered_map<string,int> paramIndex;   // "&INDEV" -> 1, "&BUFADR" -> 2
};

vector<string> DEFTAB;                      // 매크로 본문 라인 (파라미터는 #1,#2,...로 치환)
unordered_map<string, NameEntry> NAMTAB;    // 매크로 이름 -> 정의 정보

vector<string> INTERMED;                    // Pass1 결과: 매크로 정의 제거, 기타 라인 저장

/* ---------- 보조: 피연산자(인자) 파싱 ---------- */

vector<string> parseArgs(const string &operandField) {
    vector<string> args;
    string s = operandField;
    string token;
    stringstream ss(s);
    // 콤마 단위로 잘라서 trim
    while (getline(ss, token, ',')) {
        string t = trim(token);
        if (!t.empty())
            args.push_back(t);
    }
    return args;
}

/* ---------- 1줄 어셈블리 파싱 (label / opcode / operands) ---------- */

struct ParsedLine {
    bool hasLabel = false;
    string label;
    string opcode;
    string operands;
};

ParsedLine parseAssemblyLine(const string &line) {
    ParsedLine pl;
    string trimmedLeft = trimLeft(line);
    if (trimmedLeft.empty()) return pl; // empty line

    bool hasLabel = !(line.empty()) && !isspace((unsigned char)line[0]);
    pl.hasLabel = hasLabel;

    stringstream ss(trimmedLeft);
    if (hasLabel) {
        ss >> pl.label;      // 첫 토큰: label
        ss >> pl.opcode;     // 둘째 토큰: opcode
    } else {
        ss >> pl.opcode;     // 첫 토큰: opcode
    }
    string rest;
    getline(ss, rest);
    pl.operands = trim(rest);
    return pl;
}

/* ---------- Pass2에서 사용할 확장 함수 선언 ---------- */

void expandLine(const string &line, vector<string> &out, int depth);

/* ---------- 매크로 호출 확장 (Pass2) ---------- */

void expandMacro(const string &name,
                 const vector<string> &actuals,
                 vector<string> &out,
                 int depth)
{
    auto it = NAMTAB.find(name);
    if (it == NAMTAB.end()) return;
    const NameEntry &ne = it->second;

    // 깊이 제한(재귀 폭주 방지)
    if (depth > 1000) {
        // 그냥 원본 매크로 호출 형태로 출력해버리자.
        string line = name;
        if (!actuals.empty()) {
            line += " ";
            for (size_t i = 0; i < actuals.size(); ++i) {
                if (i > 0) line += ", ";
                line += actuals[i];
            }
        }
        out.push_back(line);
        return;
    }

    // ARGTAB: 1-based 인덱스 (#1, #2, ...)
    vector<string> ARGTAB(ne.params.size() + 1);
    for (size_t i = 0; i < ne.params.size(); ++i) {
        string actual = (i < actuals.size()) ? actuals[i] : "";
        ARGTAB[i + 1] = actual;
    }

    // DEFTAB[defStart..defEnd] 순회하며 #i 치환
    for (int idx = ne.defStart; idx <= ne.defEnd; ++idx) {
        string tmpl = DEFTAB[idx];
        string expanded = tmpl;

        // #10 이 #1 보다 먼저 치환되도록 뒤에서부터
        for (int p = (int)ne.params.size(); p >= 1; --p) {
            string key = "#" + to_string(p);
            expanded = replaceAll(expanded, key, ARGTAB[p]);
        }

        // 조건부 매크로, 로컬 라벨 등은 여기서 추가 구현 가능
        // (현재는 단순 치환 + 재귀 확장만 수행)

        // 치환된 한 줄을 다시 expandLine에 넘겨서
        // 내부에 다른 매크로 호출이 있는 경우도 처리
        expandLine(expanded, out, depth + 1);
    }
}

/* ---------- 한 줄 확장 (Pass2) ---------- */

void expandLine(const string &line, vector<string> &out, int depth) {
    string trimmed = trimLeft(line);
    if (trimmed.empty()) {
        out.push_back(line);
        return;
    }

    // 주석 라인(.으로 시작) 등은 그대로
    if (!trimmed.empty() && trimmed[0] == '.') {
        out.push_back(line);
        return;
    }

    ParsedLine pl = parseAssemblyLine(line);
    if (pl.opcode.empty()) {
        out.push_back(line);
        return;
    }

    auto it = NAMTAB.find(pl.opcode);
    if (it == NAMTAB.end()) {
        // 매크로 이름이 아니면 그냥 출력
        out.push_back(line);
        return;
    }

    // 매크로 호출이면 인자 파싱 후 확장
    vector<string> actuals = parseArgs(pl.operands);
    expandMacro(pl.opcode, actuals, out, depth);
}

/* ---------- main: Pass1 + Pass2 ---------- */

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    vector<string> allLines;
    string line;
    while (getline(cin, line)) {
        allLines.push_back(line);
    }

    /* ===== Pass 1: 매크로 정의 수집 ===== */

    bool inMacro = false;
    string curMacroName;
    NameEntry curEntry;

    for (size_t i = 0; i < allLines.size(); ++i) {
        string originalLine = allLines[i];
        string trimmed = trimLeft(originalLine);

        // 빈 줄
        if (trimmed.empty()) {
            if (!inMacro) INTERMED.push_back(originalLine);
            else {
                // 매크로 본문 안의 빈 줄도 DEFTAB에 그대로 저장
                DEFTAB.push_back(originalLine);
            }
            continue;
        }

        ParsedLine pl = parseAssemblyLine(originalLine);

        if (!inMacro) {
            // 매크로 정의 시작인지 검사
            if (!pl.opcode.empty() && pl.opcode == "MACRO") {
                inMacro = true;
                curMacroName = pl.label;  // label 필드에 매크로 이름이 들어온다고 가정
                if (curMacroName.empty()) {
                    cerr << "Warning: MACRO line without name at line " << (i+1) << "\n";
                    curMacroName = "_NONAME_"; // 임시
                }

                curEntry = NameEntry();
                curEntry.defStart = (int)DEFTAB.size();

                // 파라미터 목록 파싱 (&INDEV, &BUFADR, ...)
                vector<string> params = parseArgs(pl.operands);
                for (size_t pi = 0; pi < params.size(); ++pi) {
                    string p = params[pi];
                    if (!p.empty() && p[0] != '&') {
                        p = "&" + p; // 보정
                    }
                    curEntry.params.push_back(p);
                    curEntry.paramIndex[p] = (int)pi + 1; // 1-based
                }

                // MACRO 라인 자체는 INTERMED에 넣지 않음
            } else {
                // 일반 라인 → 임시 출력 버퍼
                INTERMED.push_back(originalLine);
            }
        } else {
            // 매크로 본문 안
            // MEND 체크
            if (pl.opcode == "MEND") {
                inMacro = false;
                curEntry.defEnd = (int)DEFTAB.size() - 1;
                NAMTAB[curMacroName] = curEntry;
                curMacroName.clear();
                // MEND 라인은 DEFTAB/INTERMED 어디에도 넣지 않음
            } else {
                // 본문 라인: 파라미터 이름(&INDEV 등)을 #1,#2,...로 치환해서 DEFTAB에 저장
                string bodyLine = originalLine;
                for (size_t pi = 0; pi < curEntry.params.size(); ++pi) {
                    string pname = curEntry.params[pi];      // "&INDEV"
                    string key = "#" + to_string(pi + 1);    // "#1"
                    bodyLine = replaceAll(bodyLine, pname, key);
                }
                DEFTAB.push_back(bodyLine);
            }
        }
    }

    if (inMacro) {
        cerr << "Warning: MACRO without closing MEND at end of file.\n";
    }

    /* ===== Pass 2: 매크로 확장 수행 ===== */

    vector<string> finalOut;
    for (const string &l : INTERMED) {
        expandLine(l, finalOut, /*depth=*/0);
    }

    // 결과 출력
    for (const string &l : finalOut) {
        cout << l << "\n";
    }

    return 0;
}
