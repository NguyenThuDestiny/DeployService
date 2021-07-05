#include "UpdateService.h"
#include "SdaException.h"
#include "Config.h"
#include <iostream>
#include <spdlog/spdlog.h>
#include <cstdlib>
#include <ctime>
#include <filesystem.hpp>
#include <future>

UpdateService::~UpdateService()
{
	auto logger = spdlog::get("multi_sink");
	if (NULL != logger)
	{
		logger->flush();
	}
	SaveConfig();

	if (active == true)
		active = false;

	if (mCommunicationManager != NULL)
		delete mCommunicationManager;

	if (mUpdateThread != NULL)
		delete mUpdateThread;
}

bool UpdateService::InitializeComponent(const SystemInfo& sys_info, function<void(UpdateService*, PomData)> callback)
{
	mSystemInfo = sys_info;
	mCallback = callback;

	if (false == LoadConfig())
	{
		SPDLOG_INFO("Load config fail");
		throw SdaException("Can not parse config file", PARSE_CONFIG_FILE_ERROR);
	}

	if (NULL != mCommunicationManager)
	{
		SPDLOG_INFO("Update service already initialize");
		return false;;
	}

	mCommunicationManager = new CommunicationManager();

	//default check update period
	try
	{
		mCommunicationManager->mCheckStatisPeriod = Config::GetInt("update_period");
		SPDLOG_INFO("get config.period = {}", mCommunicationManager->mCheckStatisPeriod);
	}
	catch (const std::exception&)
	{
		mCommunicationManager->mCheckStatisPeriod = Utils::Random(250, 350);
		Config::Set("update_period", mCommunicationManager->mCheckStatisPeriod);
		SPDLOG_INFO("random period = {}", mCommunicationManager->mCheckStatisPeriod);
	}

	return true;
}

void UpdateService::CheckStatusTask(UpdateService* us)
{
	SPDLOG_INFO("Thread Started");
	while (us->active)
	{
		SPDLOG_INFO("Thread Execute");

		try
		{
			// last get token
			auto last_get_token = Config::GetLongLong("last_get_token");
			auto now = Utils::GetTimestamp();
			auto periodGetToken = Config::GetInt("get_token_period");
			if (last_get_token + periodGetToken < now)
			{
				SPDLOG_INFO("Get new token after {}", periodGetToken);
				SPDLOG_INFO("last get token = {}", last_get_token);
				SPDLOG_INFO("Now = {}", now);

				us->mCommunicationManager->GetToken();
			}
		}
		catch (const std::exception& e)
		{
			SPDLOG_ERROR(e.what());
		}

		auto apps = us->mLocalVersionManager.GetLocalAppInfor();
		auto updates = us->mCommunicationManager->CheckAppStatus(us->mSystemInfo, apps);

		for (const auto& update : updates)
		{
			try
			{
				// check pom is updating in app layer
				PomData oldPomData;

				for (auto pom : us->mPoms)
				{
					if (pom.groupId == update.appId)
					{
						oldPomData = pom;
						SPDLOG_INFO("Found pom data {:p} in vector", (void*)&pom);
						break;
					}
				}
				
				if (oldPomData)
				{
					SPDLOG_INFO("Skip, pom {}@{} is updating", update.appId, update.versionId);
					continue;
				}

				// downlod new pom
				auto pomData = us->mCommunicationManager->DownloadPom(update.token, update.downloadUrl);
				if (!pomData)
					continue;

				pomData.versionId = update.versionId;

				//check dependencies need download
				auto deps = us->mLocalVersionManager.CompareWithLocalPom(pomData);
				if (deps.empty())
					continue;

				// downlod dependencies
				auto downloaded = us->mCommunicationManager->DownloadFiles(update.token, pomData, deps);
				if (downloaded.empty())
					continue;

				//copy downloaded to pom data
				pomData.downloaded.insert(pomData.downloaded.end(), downloaded.begin(), downloaded.end());

				// save pom
				us->AddPomData(pomData);

				// call callback to app
				std::async(std::launch::async, us->mCallback, us, pomData);
			}
			catch (const std::exception& e)
			{
				SPDLOG_ERROR(e.what());
			}
		}
		Config::Set("last_updated", Utils::GetDateTimeString());
		us->SaveConfig();
		SPDLOG_INFO("Thread Sleep in {} seconds", us->mCommunicationManager->mCheckStatisPeriod);
		std::this_thread::sleep_for(std::chrono::seconds(us->mCommunicationManager->mCheckStatisPeriod));
	}
	SPDLOG_INFO("Thread Ended");
}