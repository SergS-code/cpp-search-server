#include "search_server.h"
#include <thread>         // std::this_thread::sleep_for
#include <chrono>

void SearchServer:: AddDocument(int document_id,  string_view  document, DocumentStatus status, const vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (const auto  word : words) {
        word_to_document_freqs_[string(word)][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
        docs_duplecats[document_id].insert(string(word));
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.insert(document_id);
}

vector<Document> SearchServer:: FindTopDocuments( string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

vector<Document>  SearchServer:: FindTopDocuments(string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int  SearchServer:: GetDocumentCount() const {
    return documents_.size();
}


tuple<vector<string_view>, DocumentStatus> SearchServer:: MatchDocument(string_view raw_query, int document_id) const {

    if(document_ids_.count(document_id)==0){
        throw out_of_range("Invalid document_id");

    }


        const auto query = ParseQuery(raw_query);

        std::vector<std::string_view> matched_words1;
        if(any_of(query.minus_words.begin(),query.minus_words.end(),[this, document_id](std::string_view word_view){std::string word(word_view);
                  return word_to_document_freqs_.count(word)>0 && word_to_document_freqs_.at(word).count(document_id); })){
            return {matched_words1,documents_.at(document_id).status };

        }

    std::vector<std::string_view> matched_words;
    for (const auto word : query.plus_words) {
        if (word_to_document_freqs_.count(string(word)) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(string(word)).count(document_id)) {
            matched_words.push_back(string(word));
        }
    }

    sort(matched_words.begin(),matched_words.end());
    matched_words.resize(unique(matched_words.begin(),matched_words.end())-matched_words.begin());

//    for (const auto word : query.minus_words) {
//        if (word_to_document_freqs_.count(string(word)) == 0) {
//            continue;
//        }
//        if (word_to_document_freqs_.at(string(word)).count(document_id)) {
//            matched_words.clear();
//            break;
//        }
//    }

    return {matched_words, documents_.at(document_id).status};
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy&, std::string_view raw_query, int document_id) const{

 if(document_ids_.count(document_id)==0){
        throw out_of_range("Invalid document_id");

    }


        const auto query = ParseQuery(raw_query);

        std::vector<std::string_view> matched_words1;
        if(any_of(query.minus_words.begin(),query.minus_words.end(),[this, document_id](std::string_view word_view){std::string word(word_view);
                  return word_to_document_freqs_.count(word)>0 && word_to_document_freqs_.at(word).count(document_id); })){
            return {matched_words1,documents_.at(document_id).status };

        }

    std::vector<std::string_view> matched_words;
    for (const auto word : query.plus_words) {
        if (word_to_document_freqs_.count(string(word)) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(string(word)).count(document_id)) {
            matched_words.push_back(string(word));
        }
    }

    sort(matched_words.begin(),matched_words.end());
    matched_words.resize(unique(matched_words.begin(),matched_words.end())-matched_words.begin());

//    for (const auto word : query.minus_words) {
//        if (word_to_document_freqs_.count(string(word)) == 0) {
//            continue;
//        }
//        if (word_to_document_freqs_.at(string(word)).count(document_id)) {
//            matched_words.clear();
//            break;
//        }
//    }

    return {matched_words, documents_.at(document_id).status};
    }

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy&, std::string_view raw_query, int document_id) const{
 if(document_ids_.count(document_id)==0){
        throw out_of_range("Invalid document_id");

    }


        const auto query = ParseQuery(raw_query);

        std::vector<std::string_view> matched_words1;
        if(any_of(query.minus_words.begin(),query.minus_words.end(),[this, document_id](std::string_view word_view){std::string word(word_view);
                  return word_to_document_freqs_.count(word)>0 && word_to_document_freqs_.at(word).count(document_id); })){
            return {matched_words1,documents_.at(document_id).status };

        }

    std::vector<std::string_view> matched_words;
    for (const auto word : query.plus_words) {
        if (word_to_document_freqs_.count(string(word)) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(string(word)).count(document_id)) {
            matched_words.push_back(string(word));
        }
    }

    sort(matched_words.begin(),matched_words.end());
    matched_words.resize(unique(matched_words.begin(),matched_words.end())-matched_words.begin());

//    for (const auto word : query.minus_words) {
//        if (word_to_document_freqs_.count(string(word)) == 0) {
//            continue;
//        }
//        if (word_to_document_freqs_.at(string(word)).count(document_id)) {
//            matched_words.clear();
//            break;
//        }
//    }

    return {matched_words, documents_.at(document_id).status};
    }

const map<string_view, double> &SearchServer::GetWordFrequencies(int document_id) const
{
    static const std::map<std::string_view, double> none={};

    if (document_to_word_freqs_.count(document_id)!=0) {
        return document_to_word_freqs_.at(document_id);
    }
    else {
        return none;
    }

}

set<int>::const_iterator SearchServer::begin()const
{
    return document_ids_.begin();
}

set<int>::const_iterator SearchServer::end() const
{
    return document_ids_.end();

}

void SearchServer::RemoveDocument(int document_id)
{
    set<int>::iterator it = document_ids_.find(document_id);

    if(it == document_ids_.end()){
     return;
    }

    documents_.erase(document_id);

    for(auto & [word,doc_id]:document_to_word_freqs_[document_id]){

        word_to_document_freqs_[string(word)].erase(document_id);
    }

    document_to_word_freqs_.erase(document_id);
    docs_duplecats.erase(document_id);
    document_ids_.erase(it);
    

}
void SearchServer::RemoveDocument(const execution::parallel_policy&, int document_id) {
    if (document_to_word_freqs_.count(document_id) == 0) {
        return;
    }

    document_ids_.erase(document_id);
    documents_.erase(document_id);

    const auto& word_freqs = document_to_word_freqs_.at(document_id);
    for_each(execution::par, word_freqs.begin(), word_freqs.end(),
             [this, document_id](const auto& item) {
                 word_to_document_freqs_.at(string(item.first)).erase(document_id);
             });

    document_to_word_freqs_.erase(document_id);
}

void SearchServer::RemoveDocument(const execution::sequenced_policy&, int document_id) {
    if (document_to_word_freqs_.count(document_id) == 0) {
        return;
    }

    document_ids_.erase(document_id);
    documents_.erase(document_id);

    const auto& word_freqs = document_to_word_freqs_.at(document_id);
    for_each(execution::seq, word_freqs.begin(), word_freqs.end(),
             [this, document_id](const auto& item) {
                 word_to_document_freqs_.at(string(item.first)).erase(document_id);
             });

    document_to_word_freqs_.erase(document_id);
}

map<int,set<string>> SearchServer::GetDocsDuplicate()
{
    return this->docs_duplecats;
}

bool SearchServer:: IsStopWord(const string& word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer:: IsValidWord(const string& word) {

    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

vector<string_view> SearchServer:: SplitIntoWordsNoStop( string_view  text/*убрал ссылку и константу*/) const {
    vector<string_view> words;
    for (auto word : SplitIntoWords(text)) {/*убрал ссылку и константу 2 */
        if (!IsValidWord(string(word))) {
            throw invalid_argument("Word "s + string(word) + " is invalid"s);
        }
        if (!IsStopWord(string(word))) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer:: ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}



QueryWord  SearchServer:: ParseQueryWord(string_view text) const {
    if (text.empty()) {
        throw invalid_argument("Query word is empty"s);
    }
    string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(string(word))) {
        throw invalid_argument("Query word "s + string(text) + " is invalid");
    }

    return {word, is_minus, IsStopWord(string(word))};
}





Query  SearchServer:: ParseQuery(string_view text) const {
    Query result;

    for (auto word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            } else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    return result;
}

// Existence required
double SearchServer:: ComputeWordInverseDocumentFreq(const string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(string(word)).size());
}
