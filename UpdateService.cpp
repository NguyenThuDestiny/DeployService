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
				//us->mCallback(us, pomData);
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

bool UpdateService::Start()
{
	try
	{
		// get token
		try
		{
			auto tokenSuccess = mCommunicationManager->GetToken();
		}
		catch (const std::exception& e)
		{
			SPDLOG_ERROR(e.what());
			throw e;
		}

		//register agent
		try
		{
			auto registerSuccess = mCommunicationManager->RegisterAgent(mSystemInfo);
			if (false == registerSuccess)
			{
				SPDLOG_ERROR("Register Agent fail");
				return false;
			}
		}
		catch (const SdaException &e)
		{
			printf(e.what());
			if (e.get_code() != ALREADY_REGISTER_DEVICE_ERROR)
				throw e;
		}

		active = true;
		mUpdateThread = new std::thread(&UpdateService::CheckStatusTask, this);
		//mUpdateThread->detach();
		//mUpdateThread->join();
		return true;
	}
	catch (const std::exception& e)
	{
		SPDLOG_ERROR(e.what());
	}
	return false;
}

bool UpdateService::Stop()
{
	try
	{
		active = false;
		return true;
	}
	catch (const std::exception& e)
	{
		SPDLOG_ERROR(e.what());
	}

	return false;
}

bool UpdateService::UpdateLocalVersionApp(const InstallationResult& result)
{
	SPDLOG_INFO("Confirm update app={}@{} status={} message={}",
		result.appId,
		result.versionId,
		result.status,
		result.message);

	vector<InstallationResult> results;
	results.push_back(result);

	//find pom in vector
	PomData pomData;

	for (auto pom : mPoms)
	{
		if (pom.groupId == result.appId && pom.modelVersionl == result.versionId)
		{
			pomData = pom;
			SPDLOG_INFO("Found pom data {:p} in vector", (void*)&pom);
			break;
		}
	}

	// not found pom in vector
	if (!pomData)
	{
		SPDLOG_ERROR("Can not found pom data in vector with {}@{}",
			result.appId,
			result.versionId);
		return false;
	}

	//delete all file downloaded
	auto downloadedPath = ghc::filesystem::path(Utils::PATH_DOWNLOAD_DIR) / pomData.groupId;
	try
	{
		ghc::filesystem::remove_all(downloadedPath);
	}
	catch (const std::exception& ex)
	{
		SPDLOG_ERROR("Can not remove downloaded folder {}", downloadedPath.generic_string());
		SPDLOG_ERROR("{}", ex.what());
	}

	// remove pomdata in vector
	mPoms.erase(remove(mPoms.begin(), mPoms.end(), pomData), mPoms.end());

	auto response = mCommunicationManager->ConfirmUpdateStatus(results);
	if (false == response)
		return false;

	SPDLOG_INFO("Update {}@{} in remote server success.",
		result.appId,
		result.versionId);

	if (true == result.status)
	{
		// save local version
		response = mLocalVersionManager.SaveLocalVersionApp(pomData);
		if (false == response)
			return false;

		SPDLOG_INFO("Save local version {}@{} success.",
			result.appId,
			result.versionId);
	}

	return true;
}

bool UpdateService::LoadConfig()
{
	if (false == ghc::filesystem::exists(Utils::PATH_CONFIG_FILE))
		throw SdaException("Config file not exit", NO_CONFIG_FILE_ERROR);

	// load json config
	try
	{
		ifstream t(Utils::PATH_CONFIG_FILE);
		if (t.good())
		{
			t >> Config::json;
		}
		t.close();
	}
	catch (const std::exception& e)
	{
		// can not read config file
		SPDLOG_ERROR(e.what());
		throw SdaException("Can not parse config file", PARSE_CONFIG_FILE_ERROR);
	}

	SPDLOG_INFO("Load config from {}", Utils::PATH_CONFIG_FILE);

	// update static from json
	try
	{
		Utils::API_HOST = Config::GetString("API_HOST");
	}
	catch (const std::exception&)
	{
		throw SdaException("No API_HOST in config", NO_CONFIG_ERROR);
	}

	try
	{
		Utils::API_REGISTER = Config::GetString("API_REGISTER");
	}
	catch (const std::exception&)
	{
		throw SdaException("No API_REGISTER in config", NO_CONFIG_ERROR);
	}

	try
	{
		Utils::API_CHECK_STATUS = Config::GetString("API_CHECK_STATUS");
	}
	catch (const std::exception&)
	{
		throw SdaException("No API_CHECK_STATUS in config", NO_CONFIG_ERROR);
	}

	try
	{
		Utils::API_CONFIRM_UPATE_STATUS = Config::GetString("API_CONFIRM_UPATE_STATUS");
	}
	catch (const std::exception&)
	{
		throw SdaException("No API_CONFIRM_UPATE_STATUS in config", NO_CONFIG_ERROR);
	}

	try
	{
		Utils::API_CREDENTIALS_URL = Config::GetString("API_CREDENTIALS_URL");
	}
	catch (const std::exception&)
	{
		throw SdaException("No API_CREDENTIALS_URL in config", NO_CONFIG_ERROR);
	}

	try
	{
		Utils::API_CREDENTIALS_CLIENT_ID = Config::GetString("API_CREDENTIALS_CLIENT_ID");
	}
	catch (const std::exception&)
	{
		throw SdaException("No API_CREDENTIALS_CLIENT_ID in config", NO_CONFIG_ERROR);
	}

	try
	{
		Utils::API_CREDENTIALS_CLIEN_SECRET = Config::GetString("API_CREDENTIALS_CLIEN_SECRET");
	}
	catch (const std::exception&)
	{
		throw SdaException("No API_CREDENTIALS_CLIEN_SECRET in config", NO_CONFIG_ERROR);
	}
}

bool UpdateService::SaveConfig()
{
	try
	{
		// save json to file
		ofstream o(Utils::PATH_CONFIG_FILE);
		o << setw(4) << Config::json << std::endl;
		o.close();

		SPDLOG_INFO("Save config to {}", Utils::PATH_CONFIG_FILE);
		return true;
	}
	catch (const std::exception& e)
	{
		SPDLOG_ERROR("Can not save config file : {}", e.what());
	}
	return false;
}

void UpdateService::AddPomData(const PomData& pom_data)
{
	mPoms.push_back(pom_data);
}