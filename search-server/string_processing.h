#pragma once
#include <iostream>
#include <set>
#include <map>
#include <algorithm>
#include <set>
#include <stdexcept>
#include "document.h"

using namespace std;

ostream& operator<<(ostream& out, const Document& document) ;

template <typename StringContainer>
set<string,less<>> MakeUniqueNonEmptyStrings( StringContainer& strings) {
    set<string,less<>> non_empty_strings;
    for (auto  str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(string(str));
        }
    }
    return non_empty_strings;
}

vector<string_view> SplitIntoWords(string_view text);
vector<string> SplitIntoWords(const string& text);
