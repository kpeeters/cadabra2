
#include <iostream>
#include <string>
#include <regex>

int main(int argc, char **argv)
	{
	if(argc<3) return -1;

	std::regex match(argv[1]);
	std::smatch res;
	if(std::regex_match(std::string(argv[2]), res, match)) {
		for(unsigned int i=0; i<res.size(); ++i) {
			std::cout << i << ":\t |" << res[i] << "|" << std::endl;
			}
		}
	else {
		std::cout << "no match" << std::endl;
		}
	}
