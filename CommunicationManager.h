#ifndef SDA_COMMUNICATIONMANAGER_H
#define SDA_COMMUNICATIONMANAGER_H

#include <string>
#include "SystemInfo.h"
#include <httplib.h>
#include "Utils.h"

using namespace std;

class CommunicationManager
{
private:
	string GetFromServer(const string& url,
		const HttpAuthorization& auth);
	string PostToServer(const string& url,
		const string& content,
		const HttpAuthorization& token);

	string mDeviceId;
	string mToken;

public:
	CommunicationManager();
	int mCheckStatisPeriod = 300;
	bool RegisterAgent(const SystemInfo& sys_info);
	vector<UpdateInfo> CheckAppStatus(const SystemInfo& sys_info,
		const vector<AppInfo>& apps);
	PomData DownloadPom(const string& token, const string& pom_url);
	vector<string> DownloadFiles(const string& token, const PomData& pom_data,
		const vector<Dependency>& downloads);
	bool ConfirmUpdateStatus(const vector<InstallationResult>& result);
	bool GetToken();
};

#endif
