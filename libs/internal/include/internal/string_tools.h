#include <algorithm> 
#include <cctype>
#include <locale>
#include <string>
#include <vector>

inline std::string ltrim(std::string s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !std::isspace(ch);
	}));
	return s;
}

inline std::string rtrim(std::string s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !std::isspace(ch);
	}).base(), s.end());
	return s;
}

inline std::string trim(std::string s)
{
	return ltrim(rtrim(s));
}
inline std::vector<std::string> string_to_vec(const std::string& s)
{
	std::vector<std::string> v;
	auto pos = s.begin();
	while (pos != s.end()) {
		auto it = std::find(pos, s.end(), '\n');
		if (it != s.end())
			++it;
		v.push_back(std::string(pos, it));
		pos = it;
	}
	return v;
}
