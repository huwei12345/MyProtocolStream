#include "MyProtocolStream.h"
#include <iostream>
using namespace std;
using namespace net;

void print(string& s) {
    for (int i = 0; i < s.length(); i++) {
        unsigned char c = s[i];
        std::cout << std::hex << (unsigned int)c << " ";
    }
    std::cout << std::endl;
}

int main() {
    string request;
    MyProtocolStream reqStream(request);
    print(request);
    reqStream.loadInt32(1000);
    print(request);
    reqStream.loadString("hello");
    print(request);
    reqStream.flush();
    print(request);
    int x = 0;
    reqStream.getInt32(x);
    printf("x = %d\n", x);
    print(request);
    string s;
    reqStream.getString(s);
    print(s);
    print(request);
    reqStream.clear();
    print(request);
    reqStream.setPos(0);
    printf("`````````````````````````````````````````\n");
    
    print(request);
    reqStream.loadInt32(3);
    print(request);
    reqStream.loadString("hello");
    print(request);
    reqStream.flush();
    print(request);
    reqStream.getInt32(x);
    printf("x = %d\n", x);
    print(request);
    
    reqStream.getString(s);
    print(s);
    print(request);
    reqStream.clear();
    print(request);
}
