#include "request_queue.h"


vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
    QueryResult rezult;
    rezult.rez=ser.FindTopDocuments( raw_query,  status);
    if(requests_.size()==sec_in_day_)
        requests_.pop_front();
    requests_.push_back(rezult);

    return   rezult.rez;
}

vector<Document> RequestQueue:: AddFindRequest(const string& raw_query) {
    QueryResult rezult;
    rezult.rez=ser.FindTopDocuments(raw_query);
    if(requests_.size()==sec_in_day_)
        requests_.pop_front();
    requests_.push_back(rezult);

    return   rezult.rez;
}

RequestQueue:: RequestQueue(const SearchServer& search_server):ser(search_server){

}

int RequestQueue:: GetNoResultRequests() const {
    int sum=0;
    for(auto a:requests_)
    {
        if(a.rez.size()==0)
            sum++;
    }
    return sum;

}
