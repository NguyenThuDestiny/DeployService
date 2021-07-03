#include "Config.h"
#include "lib/spdlog/include/spdlog/spdlog.h"
#include "SdaException.h"

nlohmann::json Config::json = nlohmann::json();

bool Config::Set(const string& key, const int& value) {
	try {
		json[key] = value;
		SPDLOG_INFO("Set {}={} in config", key, value);
		return true;
	}
	catch (const exception &e) {
		SPDLOG_ERROR("Can not set config {}={} {}", key, value, e.what());
	}
	return false;
}

bool Config::Set(const string& key, const long long& value) {
	try {
		json[key] = value;
		SPDLOG_INFO("Set {}={} in config", key, value);
		return true;
	}
	catch (const exception& e) {
		SPDLOG_ERROR("Can not set config {}={} {}", key, value, e.what());
	}
	return false;
}

bool Config::Set(const string& key, const string& value) {
	try {
		json[key] = value;
		SPDLOG_INFO("Set {}={} in config", key, value);
		return true;
	}
	catch (const exception& e) {
		SPDLOG_ERROR("Can not set config {}={} {}", key, value, e.what());
	}
	return false;
}

bool Config::Set(const string& key, const bool& value) {
	try {
		json[key] = value;
		SPDLOG_INFO("Set {}={} in config", key, value);
		return true;
	}
	catch (const exception& e) {
		SPDLOG_ERROR("Can not set config {}={} {}", key, value, e.what());
	}
	return false;
}

int Config::GetInt(const string& key) {
	auto value = json[key];
	if (value.type() == nlohmann::json::value_t::null) {
		SPDLOG_ERROR("No value in config of {}", key);
		throw SdaException(string("no value for key = " + key),
									GET_CONFIG_NO_VALUE_ERROR);
	}

	if (value.type() == nlohmann::json::value_t::number_integer)
		return value.get<int>();

	if (value.type() == nlohmann::json::value_t::number_unsigned)
		return value.get<int>();

	SPDLOG_ERROR("Value in config of {} is {} != int", key, value.type());
	throw SdaException(string("invalid int type for key=" + key),
								GET_CONFIG_VALUE_TYPE_ERROR);

}

long long Config::GetLongLong(const string& key) {
	auto value = json[key];
	if (value.type() == nlohmann::json::value_t::null) {
		SPDLOG_ERROR("No value in config of {}", key);
		throw SdaException(string("no value for key = " + key),
			GET_CONFIG_NO_VALUE_ERROR);
	}

	if (value.type() == nlohmann::json::value_t::number_integer)
		return value.get<long long>();

	if (value.type() == nlohmann::json::value_t::number_unsigned)
		return value.get<long long>();

	SPDLOG_ERROR("Value in config of {} is {} != int", key, value.type());
	throw SdaException(string("invalid long long type for key=" + key),
		GET_CONFIG_VALUE_TYPE_ERROR);

}

string Config::GetString(const string& key) {
	auto value = json[key];
	if (value.type() == nlohmann::json::value_t::null) {
		SPDLOG_ERROR("No value in config of {}", key);
		throw SdaException(string("no value for key = " + key),
			GET_CONFIG_NO_VALUE_ERROR);
	}

	if (value.type() == nlohmann::json::value_t::string)
		return value.get<string>();

	SPDLOG_ERROR("Value in config of {} is {} != int", key, value.type());
	throw SdaException(string("invalid string type for key=" + key),
		GET_CONFIG_VALUE_TYPE_ERROR);

}

bool Config::Getbool(const string& key) {
	auto value = json[key];
	if (value.type() == nlohmann::json::value_t::null) {
		SPDLOG_ERROR("No value in config of {}", key);
		throw SdaException(string("no value for key = " + key),
			GET_CONFIG_NO_VALUE_ERROR);
	}

	if (value.type() == nlohmann::json::value_t::boolean)
		return value.get<bool>();

	SPDLOG_ERROR("Value in config of {} is {} != int", key, value.type());
	throw SdaException(string("invalid bool type for key=" + key),
		GET_CONFIG_VALUE_TYPE_ERROR);

}