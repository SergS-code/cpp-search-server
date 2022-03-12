#include <algorithm>
#include <cstdlib>
#include <future>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <mutex>




template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
       
    };

    explicit ConcurrentMap(size_t bucket_count):
    bucket_count_(bucket_count),
    submaps(bucket_count),
    mutexs(std::vector<std::mutex>(bucket_count)){};
    

    Access operator[](const Key& key){
    uint64_t index = static_cast<uint64_t>(key) % bucket_count_;
		return Access{  std::lock_guard(mutexs[index]) ,submaps[index][key]};
    };

    std::map<Key, Value> BuildOrdinaryMap() {
		std::map<Key, Value> result;
		
		for (size_t i = 0; i < submaps.size(); ++i) {
			std::lock_guard guard(mutexs[i]);
			std::for_each(submaps[i].begin(),
				submaps[i].end(), [&](auto& item)
			{
				result.insert(move(item));
			});
		}
		return result;
	};

private:
uint64_t bucket_count_;
std::vector<std::map<Key, Value>> submaps;
std::vector<std::mutex> mutexs;


};
