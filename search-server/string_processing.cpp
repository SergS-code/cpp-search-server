#include "string_processing.h"
using namespace std;


vector<string_view> SplitIntoWords( string_view text) {
    vector<string_view> words;
    string word;

    while (true) {
        size_t space_pos = text.find(' ');
        if(text.size()!=0){
            if(space_pos>0)
                words.push_back(text.substr(0, space_pos));
            if (space_pos == text.npos) {
                break;
            }
        }else{
            break;
        }

        text.remove_prefix(space_pos + 1);
    }
    return words;

}
vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}
