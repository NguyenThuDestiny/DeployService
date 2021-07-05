#ifndef SDA_SYSTEMINFO_H
#define SDA_SYSTEMINFO_H

#include <string>

using namespace std;

class SystemInfo
{
public:
	string mMacAddress;
	string mSerial;
	string mDeviceType;
	string mLocation;
	string mAddress;
	string mModel;

public:
	SystemInfo();
	SystemInfo(const string&, const string&, const string&, const string&, const string&, const string&);
};

#endif
