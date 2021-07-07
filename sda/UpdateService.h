#ifndef SDA_UPDATESERVICE_H
#define SDA_UPDATESERVICE_H

#include <string>
#include <functional>
#include "CommunicationManager.h"
#include "LocalVersionManager.h"
#include <thread>
#include <chrono>

using namespace std;

class UpdateService
{
private:
	CommunicationManager* mCommunicationManager = NULL;
	LocalVersionManager mLocalVersionManager;

	SystemInfo mSystemInfo;
	function<void(UpdateService*, PomData)> mCallback;

	vector<PomData> mPoms;

	bool LoadConfig();
	bool SaveConfig();

	//thread
	static void CheckStatusTask(UpdateService* aClass);
	bool active = false;
	void AddPomData(const PomData& pom_data);

public:
	~UpdateService();

	bool InitializeComponent(const SystemInfo& sys_omfp, function<void(UpdateService*, PomData)> callback);
	bool Start();
	bool Stop();
	bool UpdateLocalVersionApp(const InstallationResult& result);

	thread* mUpdateThread = NULL;
};

#endif