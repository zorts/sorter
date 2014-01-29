#include "sorter.h"
#include "keyconvert.h"

#include <iostream>
#include <vector>
#include <stdint.h>

class PrintStringSortResults: public external_sort::Receiver {
public:
  void receive(const void* payload, unsigned int payloadLength)
  {
    using namespace std;
    string s((const char*) payload, payloadLength);
    cout << s << endl;
  }
};

int main(int argc, const char* const argv[])
{
  using namespace std;
  typedef pair<unsigned int, string> KeyValue;
  vector<KeyValue> source;
  source.push_back({3, "three 1"});
  source.push_back({2, "two"});
  source.push_back({1, "one"});
  source.push_back({0, "zero"});
  source.push_back({3, "three 2"});

  PrintStringSortResults receiver;
  external_sort::Sorter sorter;
  sorter
    .withReceiver(&receiver)
    .stable()
    .create();

  for (auto kv: source) 
  {
    char keyBuffer[4];
    external_sort::uint32ToKey(kv.first, keyBuffer);
    sorter.sort(keyBuffer, 4, kv.second.data(), kv.second.size());
  }
  sorter.finish();
  
  return 0;
}
