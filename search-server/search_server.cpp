#include "search_server.h"


void SearchServer:: AddDocument(int document_id,  string_view  document, DocumentStatus status, const vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("Invalid document_id"s);
    }

    //const auto words = SplitIntoWordsNoStop(document);
    const auto [it, inserted] = documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status, std::string(document) });
    document_ids_.insert(document_id);
    const auto words = SplitIntoWordsNoStop(it->second.str);


    const double inv_word_count = 1.0 / words.size();
    for (const auto  word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
        docs_duplecats[document_id].insert(string(word));
    }

}

vector<Document> SearchServer:: FindTopDocuments(const std::execution::sequenced_policy&, string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(std::execution::seq,raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

vector<Document> SearchServer:: FindTopDocuments( string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(std::execution::seq,raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}
vector<Document> SearchServer:: FindTopDocuments(const std::execution::parallel_policy&, string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(std::execution::par,raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}
vector<Document>  SearchServer:: FindTopDocuments(string_view raw_query) const {
    return FindTopDocuments( std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}
vector<Document>  SearchServer:: FindTopDocuments(const std::execution::sequenced_policy&,string_view raw_query)const {
    return FindTopDocuments( std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}
vector<Document>  SearchServer:: FindTopDocuments(const std::execution::parallel_policy&,string_view raw_query) const {
    return FindTopDocuments( std::execution::par, raw_query, DocumentStatus::ACTUAL);
}

int  SearchServer:: GetDocumentCount() const {
    return documents_.size();
}


std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(std::string_view raw_query, int document_id) const {
    return MatchDocument(std::execution::seq, raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy&, std::string_view raw_query, int document_id) const {
    if (!document_ids_.count(document_id)) {
        throw std::out_of_range("incorrect document_id");
    }
    const Query query = ParseQuery(raw_query);

    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            return { vector<std::string_view>{}, documents_.at(document_id).status };
        }
    }
    std::vector<std::string_view> matched_words;

    for (const std::string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy&, std::string_view raw_query, int document_id) const {
    const auto query = ParseQuery(raw_query, true);

    const auto status = documents_.at(document_id).status;

    const auto whoareyou =
            [this, document_id](const std::string_view word) {
        const auto it = word_to_document_freqs_.find(word);
        return it != word_to_document_freqs_.end() && it->second.count(document_id);
    };

    if (std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(), whoareyou )) {
        return {vector<std::string_view> {}, status };
    }

    std::vector<std::string_view> matched_words(query.plus_words.size());
    auto words_end = std::copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(),
                                  matched_words.begin(), whoareyou
                                  );
    std::sort(matched_words.begin(), words_end);
    words_end = std::unique(matched_words.begin(), words_end);
    matched_words.erase(words_end, matched_words.end());

    return { matched_words, status };
}

Query SearchServer::ParseQuery(const std::string_view text, bool no_sort) const {
    Query result;
    for (const std::string_view word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            }
            else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    if (!no_sort) {
        for (auto* words : { &result.plus_words, &result.minus_words }) {
            std::sort(words->begin(), words->end());
            words->erase(unique(words->begin(), words->end()), words->end());
        }
    }
    return result;
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

        word_to_document_freqs_[word].erase(document_id);
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
        word_to_document_freqs_.at(item.first).erase(document_id);
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
        word_to_document_freqs_.at(item.first).erase(document_id);
    });

    document_to_word_freqs_.erase(document_id);
}

map<int,set<string>> SearchServer::GetDocsDuplicate()
{
    return this->docs_duplecats;
}

bool SearchServer:: IsStopWord(const string_view word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer:: IsValidWord(const string_view word) {

    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

vector<string_view> SearchServer:: SplitIntoWordsNoStop( string_view  text) const {
    vector<string_view> words;
    for (auto word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Word "s + string(word) + " is invalid"s);
        }
        if (!IsStopWord(word)) {
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
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw invalid_argument("Query word "s + string(text) + " is invalid");
    }

    return {word, is_minus, IsStopWord(word)};
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

    sort(result.plus_words.begin(),result.plus_words.end());
    // unique(result.plus_words.begin(),result.plus_words.end());
    //result.plus_words.erase(std::unique(result.plus_words.begin(), result.plus_words.end()),result.plus_words.end());


    auto last = std::unique(result.plus_words.begin(), result.plus_words.end());
    result.plus_words.resize(std::distance(result.plus_words.begin(), last));


    sort(result.minus_words.begin(),result.minus_words.end());
    auto last1 = std::unique(result.minus_words.begin(), result.minus_words.end());
    result.minus_words.resize(std::distance(result.minus_words.begin(), last1));

    return result;
}

// Existence required
double SearchServer:: ComputeWordInverseDocumentFreq( string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}












