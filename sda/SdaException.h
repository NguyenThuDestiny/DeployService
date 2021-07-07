#ifndef SDA_EXCEPTION_H
#define SDA_EXCEPTION_H

#include <string>

using namespace std;

enum Code {
	HTTP_NOT_OK_ERROR,
	HTTP_ERROR,
	HTTP_UNAUTHORIZED,
	GET_CONFIG_NO_VALUE_ERROR,
	GET_CONFIG_VALUE_TYPE_ERROR,
	SET_CONFIG_ERROR,
	PARSE_POM_ERROR,
	NO_REPOSITORIES_ERROR,
	NO_REPOSITORY_URL_ERROR,
	INVALID_TYPE_ERROR,
	ALREADY_REGISTER_DEVICE_ERROR,
	NO_CONFIG_ERROR,
	NO_CONFIG_FILE_ERROR,
	PARSE_CONFIG_FILE_ERROR,
	NO_MAC_ERROR,
	NO_SERIAL_ERROR,
	NOT_GET_TOKEN_ERROR
};

class SdaException : public exception {
private:
	Code code;
	string msg;

public:
	SdaException(const string& message, const Code& c) {
		msg = message;
		code = c;
	}

	Code get_code() const {
		return code;
	}

	const char* what() const throw() {
		return msg.c_str();
	}
};

#endif