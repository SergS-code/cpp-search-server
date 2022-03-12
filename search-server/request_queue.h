#pragma once
#include "search_server.h"
#include "document.h"
#include <set>
#include <deque>


using namespace std;

class RequestQueue {
public:
    explicit  RequestQueue(const SearchServer& search_server):ser(search_server)
    {

    }

    template <typename DocumentPredicate>
    vector<Document> AddFindRequest(const string& raw_query, DocumentPredicate document_predicate) {
        QueryResult rezult;
        rezult.rez=ser.FindTopDocuments(  raw_query,  document_predicate);
        if(requests_.size()==sec_in_day_)
            requests_.pop_front();
        requests_.push_back(rezult);

       return   rezult.rez;
    }

    vector<Document> AddFindRequest(const string& raw_query, DocumentStatus status);

    vector<Document> AddFindRequest(const string& raw_query);

    int GetNoResultRequests() const ;
private:
    struct QueryResult {
      vector<Document>rez;
      int num;
    };
    deque<QueryResult> requests_;
    const static int sec_in_day_ = 1440;
    const SearchServer& ser;


};
