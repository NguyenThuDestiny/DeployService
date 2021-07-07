#include "SystemInfo.h"

SystemInfo::SystemInfo() {};

SystemInfo::SystemInfo(const string& mac_dd,
	const string& serial,
	const string& model,
	const string& device_type,
	const string& location,
	const string& address)
{
	mAddress = mac_dd;
	mSerial = serial;
	mModel = model;
	mDeviceType = device_type;
	mLocation = location;
	mAddress = address;
}