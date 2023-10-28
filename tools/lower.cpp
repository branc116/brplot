#include <iostream>

int main(void) {
  std::string s;
  std::cin >> s;
  for (char& c : s) if (c >= 'A' && c <= 'Z') c +=  'a' - 'A';
  std::cout << s << " "; 
  return 0;
}
