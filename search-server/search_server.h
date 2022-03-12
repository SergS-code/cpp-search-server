#pragma once

#include <string>
#include <map>
#include <set>
#include <algorithm>
#include <cmath>
#include <iterator>
#include <execution>
#include "document.h"
#include "read_input_functions.h"
#include "string_processing.h"
#include <future>
#include <execution>
#include "concurrent_map.h"

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

struct QueryWord {
    string_view data;
    bool is_minus;
    bool is_stop;
};

struct Query {
    vector<string_view> plus_words;
    vector<string_view> minus_words;
};

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
    {
        if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
            throw invalid_argument("Some of stop words are invalid"s);
        }
    }

    explicit SearchServer( string_view stop_words_text) // убрал константу
        : SearchServer(SplitIntoWords(stop_words_text)) {

    }

    explicit SearchServer(const string& stop_words_text)
            : SearchServer(SplitIntoWords(stop_words_text)) {

        }

    void AddDocument(int document_id,  string_view document, DocumentStatus status, const vector<int>& ratings);/*убрал ссылку и константу*/

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(string_view raw_query, DocumentPredicate document_predicate) const {
         auto query = ParseQuery(raw_query);

        
         sort(query.minus_words.begin(),query.minus_words.end());
        unique(query.minus_words.begin(),query.minus_words.end());

        sort(query.plus_words.begin(),query.plus_words.end());
        unique(query.plus_words.begin(),query.plus_words.end());
        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
                return lhs.rating > rhs.rating;
            } else {
                return lhs.relevance > rhs.relevance;
            }
        });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }
    vector<Document> FindTopDocuments(string_view raw_query, DocumentStatus status) const ;


    vector<Document> FindTopDocuments(string_view raw_query) const;/*убрал ссылку и константу*/
    int GetDocumentCount() const;
    void RemoveDocument(const execution::parallel_policy&, int document_id);
    void RemoveDocument(const execution::sequenced_policy&, int document_id);
    tuple<vector<string_view>, DocumentStatus> MatchDocument(string_view raw_query, int document_id) const;/*убрал ссылку и константу*/
    tuple<vector<string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy&, std::string_view raw_query, int document_id) const;
    tuple<vector<string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy&, std::string_view raw_query, int document_id) const;
    const map<string_view, double>& GetWordFrequencies(int document_id) const;

    set<int> ::const_iterator begin()const;
    set<int>::const_iterator end()const;
    void RemoveDocument(int document_id);

    map<int,set<string>> GetDocsDuplicate() ;




private:


    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    const set<string> stop_words_;
    map<int,set<string>> docs_duplecats;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int,  map<string_view,double>> document_to_word_freqs_;
    map<int, DocumentData> documents_;
    set<int> document_ids_;

    bool IsStopWord(const string& word) const;
    static bool IsValidWord(const string& word);
    vector<string_view> SplitIntoWordsNoStop(string_view text) const;
    static int ComputeAverageRating(const vector<int>& ratings) ;


    QueryWord ParseQueryWord(string_view text) const;


    Query ParseQuery(string_view text) const ;
    double ComputeWordInverseDocumentFreq(const string_view word) const;


    template <typename ExecutionPolicy, typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& police, std::string_view raw_query,
        DocumentPredicate document_predicate) const {

            const auto query = ParseQuery(raw_query);
            auto matched_documents = FindAllDocuments(police, query, document_predicate);
            std::sort(police,matched_documents.begin(), matched_documents.end(),
                [](const Document& lhs, const Document& rhs) {
                    return lhs.relevance > rhs.relevance
                        || (std::abs(lhs.relevance - rhs.relevance) < 1e-6 && lhs.rating > rhs.rating);
                });
            if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
                matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
            }
            return matched_documents;

    }






    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (auto word : query.plus_words) {
            if (word_to_document_freqs_.count(string(word)) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(string(word))) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (auto word : query.minus_words) {/*убрал ссылку и константу*/
            if (word_to_document_freqs_.count(string(word)) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(string(word))) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};