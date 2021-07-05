#ifndef SDA_LOCALVERSIONMANAGER_H
#define SDA_LOCALVERSIONMANAGER_H

#include <string>
#include "Utils.h"

using namespace std;

class LocalVersionManager
{
public:
	bool SaveLocalVersionApp(const PomData& pom_data);
	PomData GetLocalPom(const string& groupId);
	vector<Dependency> CompareWithLocalPom(const PomData& pom_data);
	vector<AppInfo> GetLocalAppInfor();
};

#endif