#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries( const SearchServer& search_server, const std::vector<std::string>& queries){
    std:: vector<vector<Document>> result(queries.size());
    transform(execution::par, make_move_iterator(queries.begin()), make_move_iterator(queries.end()), result.begin(),
                  [&search_server](const string &query) { return search_server.FindTopDocuments(query); });
    return result;
} 

std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries){
    
    std::list<Document> result;
	
    std::vector<std::vector<Document>> rez=ProcessQueries(search_server,queries);
    
    for (const auto a : rez){
        for (const auto b : a){
            result.push_back(b);
        }
    }
	return result;
    
    
}