#include <locale>
#include <codecvt>
#include <string>

#ifdef _MSC_VER

struct utf_converter
{
public:
	std::string to_utf8(const std::u32string& utf32_string)
	{
		std::basic_string<__int32> inp(utf32_string.begin(), utf32_string.end());
		return conv.to_bytes(inp);
	}
	std::u32string to_utf32(const std::string& utf8_string)
	{
		auto int32str = conv.from_bytes(utf8_string);
		return std::u32string(int32str.begin(), int32str.end());
	}

private:
	std::wstring_convert<std::codecvt_utf8<__int32>, __int32> conv;
};

#else

struct utf_converter
{
public:
	std::string to_utf8(const std::u32string& utf32_string)
	{
		return conv.to_bytes(utf32_string);
	}
	std::u32string to_utf32(const std::string& utf8_string)
	{
		return conv.from_bytes(utf8_string);
	}

private:
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
};

#endif
