#ifndef SDA_EXCEPTION_H
#define SDA_EXCEPTION_H

#include <string>

using namespace std;

enum Code {
	HTTP_NOT_OK_ERROR,
	HTTP_ERROR,
	GET_CONFIG_NO_VALUE_ERROR,
	GET_CONFIG_VALUE_TYPE_ERROR
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