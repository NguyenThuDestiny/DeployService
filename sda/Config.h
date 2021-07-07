#ifndef SDA_CONFIG_H
#define SDA_CONFIG_H

#include <string>
#include "lib/json/single_include/nlohmann/json.hpp"

using namespace std;

class Config
{
public:
	static bool Set(const string& key, const int &value);
	static bool Set(const string& key, const long long& value);
	static bool Set(const string& key, const string& value);
	static bool Set(const string& key, const bool& value);

	static int GetInt(const string& key);
	static string GetString(const string& key);
	static bool Getbool(const string& key);
	static long long GetLongLong(const string& key);

	static nlohmann::json json;
};

#endif

