#include "CommunicationManager.h"
#include <json.hpp>
#include <spdlog/spdlog.h>
#include "Config.h"
#include <filesystem.hpp>
#include "SdaException.h"

CommunicationManager::CommunicationManager()
{
	// load deviceid from config
	try
	{
		auto deviceId = Config::GetString("deviceId");
		if (false == deviceId.empty())
		{
			mDeviceId = deviceId;
			SPDLOG_INFO("Get device id in config = {}", deviceId);
		}
	}
	catch (const std::exception&)
	{
		SPDLOG_ERROR("Can not get device id in config");
	}
}

string CommunicationManager::GetFromServer(const string& url,
	const HttpAuthorization& auth)
{
	auto host = Utils::GetHost(url);
	auto path = Utils::GetPath(url);
	if (host.empty())
		host = Utils::API_HOST;

	httplib::Client client(host.c_str());

	if (false == auth.token.empty())
	{
		client.set_bearer_token_auth(auth.token.c_str());
	}

	if (false == auth.username.empty() && false == auth.password.empty())
	{
		client.set_basic_auth(auth.username.c_str(), auth.password.c_str());
	}

	SPDLOG_INFO("[GET] {}{}", host, path);
	auto response = client.Get(path.c_str());
	if (response)
	{
		SPDLOG_INFO("Response status = {}", response->status);

		if (response->body.size() < 3000)
			SPDLOG_INFO("Response body = \n{}", response->body);

		if (response->status == 401)
			throw SdaException(string("Unauthorized " + to_string(response->status)), HTTP_UNAUTHORIZED);
		if (response->status != 200)
			throw SdaException(string("http status " + to_string(response->status)), HTTP_NOT_OK_ERROR);
		return response->body;
	}
	else
	{
		SPDLOG_ERROR("Http error code  = {}", response.error());
		throw SdaException(string("http error " + to_string(response->status)), HTTP_ERROR);
	}
}

string CommunicationManager::PostToServer(const string& url,
	const string& content,
	const HttpAuthorization& auth)
{
	auto host = Utils::GetHost(url);
	auto path = Utils::GetPath(url);
	if (host.empty())
		host = Utils::API_HOST;

	httplib::Client client(host.c_str());

	if (false == auth.token.empty())
	{
		client.set_bearer_token_auth(auth.token.c_str());
	}

	if (false == auth.username.empty() && false == auth.password.empty())
	{
		client.set_basic_auth(auth.username.c_str(), auth.password.c_str());
	}

	SPDLOG_INFO("[POST] {}{}", host, path);
	SPDLOG_INFO("Request body =\n{}", content);
	
	auto response = client.Post(path.c_str(), content.c_str(), "application/json");

	if (response)
	{
		SPDLOG_INFO("Response status = {}", response->status);
		SPDLOG_INFO("Response body = \n{}", response->body);

		if (response->status == 401)
			throw SdaException(string("Unauthorized " + to_string(response->status)), HTTP_UNAUTHORIZED);
		if (response->status != 200)
			throw SdaException(string("http status " + to_string(response->status)), HTTP_NOT_OK_ERROR);
		return response->body;
	}
}

bool CommunicationManager::GetToken()
{
	HttpAuthorization auth;
	auth.username = Utils::API_CREDENTIALS_CLIENT_ID;
	auth.password = Utils::API_CREDENTIALS_CLIEN_SECRET;

	auto response = PostToServer(Utils::API_CREDENTIALS_URL, "", auth);
	if (response.empty())
	{
		throw SdaException("Can not get token from " + Utils::API_CREDENTIALS_URL,
			NOT_GET_TOKEN_ERROR);
	}
	auto responseObject = nlohmann::json::parse(response);
	mToken = responseObject["access_token"].get<string>();
	SPDLOG_INFO("Token = \n{}", mToken);

	try
	{
		Config::GetInt("get_token_period");
	}
	catch (const std::exception&)
	{
		Config::Set("get_token_period", 60 * 60);
	}
	Config::Set("last_get_token", Utils::GetTimestamp());
	return true;
}

bool CommunicationManager::RegisterAgent(const SystemInfo& sys_info)
{
	// check reregister
	if (false == mDeviceId.empty())
	{
		throw SdaException(string("Device already register wiith id = " + mDeviceId),
			ALREADY_REGISTER_DEVICE_ERROR);
	}

	nlohmann::json body;
	body["serialNo"] = sys_info.mSerial;
	body["mac"] = sys_info.mMacAddress;

	body["model"] = sys_info.mModel;
	body["type"] = sys_info.mDeviceType;
	body["loaction"] = sys_info.mLocation;
	body["address"] = sys_info.mAddress;

	string content = body.dump();

	HttpAuthorization auth;
	auth.token = mToken;

	string response = PostToServer(Utils::API_REGISTER, content, auth);
	if (false == response.empty())
	{
		auto responseObject = nlohmann::json::parse(response);
		mDeviceId = responseObject["id"].get<string>();
		Config::Set("deviceId", mDeviceId);
		return true;
	}
	return true;
}

vector<UpdateInfo> CommunicationManager::CheckAppStatus(const SystemInfo& sys_info,
	const vector<AppInfo>& apps)
{
	nlohmann::json body;

	// add deviceInfo
	nlohmann::json deviceInfoObject;
	deviceInfoObject["deviceId"] = mDeviceId;
	deviceInfoObject["serialNo"] = sys_info.mSerial;
	deviceInfoObject["model"] = sys_info.mModel;
	deviceInfoObject["type"] = sys_info.mDeviceType;
	deviceInfoObject["mac"] = sys_info.mMacAddress;
	deviceInfoObject["location"] = sys_info.mLocation;
	deviceInfoObject["address"] = sys_info.mAddress;
	body["deviceInfo"] = deviceInfoObject;

	//add appInfo
	auto appInfoArray = nlohmann::json::array();
	for (auto& app : apps)
	{
		nlohmann::json infoObject;
		infoObject["appId"] = app.appId;
		infoObject["versionId"] = app.versionId;
		appInfoArray.push_back(infoObject);
	}
	body["appInfo"] = appInfoArray;

	//post to server
	auto content = body.dump();
	try
	{
		HttpAuthorization auth;
		auth.token = mToken;

		auto response = PostToServer(Utils::API_CHECK_STATUS, content, auth);

		vector<UpdateInfo> updates;

		auto responseObject = nlohmann::json::parse(response);

		// update period
		auto period = responseObject["updateInfo"].get<int>();
		mCheckStatisPeriod = period;
		Config::Set("update_period", period);

		auto updateInfoResponse = responseObject["updateInfo"];
		if (updateInfoResponse.type() != nlohmann::json::value_t::array)
			throw SdaException("invalid type", INVALID_TYPE_ERROR);

		for (auto& update : updateInfoResponse)
		{
			UpdateInfo updateInfo;
			updateInfo.appId = update["appId"].get<string>();
			updateInfo.versionId = update["versionId"].get<string>();
			updateInfo.versionName = update["versionName"].get<string>();
			updateInfo.downloadUrl = update["downloadUrl"].get<string>();
			updateInfo.token = update["token"].get<string>();
			updates.push_back(updateInfo);
		}
		
		return updates;
	}
	catch (const std::exception& e)
	{
		SPDLOG_ERROR(e.what());
		throw e;
	}
}

PomData CommunicationManager::DownloadPom(const string& token, const string& pom_url)
{
	HttpAuthorization auth;
	auth.token = token;

	auto content = GetFromServer(pom_url, auth);
	auto pomData = Utils::ParsePom(content);
	pomData.raw = content;
	return pomData;
}

vector<string> CommunicationManager::DownloadFiles(
	const string& token,
	const PomData& pom_data,
	const vector<Dependency>& dependencies)
{
	vector<string> paths;
	if (pom_data.repositories.empty())
		throw SdaException("no repositories", NO_REPOSITORIES_ERROR);

	string repository_url = pom_data.repositories[0].url;
	if (repository_url.empty())
		throw SdaException("no repository url", NO_REPOSITORY_URL_ERROR);

	for (auto dep : dependencies)
	{
		auto uri = repository_url + dep.Path();

		try
		{
			HttpAuthorization auth;
			auth.token = token;

			auto response = GetFromServer(uri, auth);

			//check response OK 200
			if (false == response.empty())
			{
				// write content to file

				auto folderPath = ghc::filesystem::path(Utils::PATH_DOWNLOAD_DIR) /
					pom_data.groupId /
					pom_data.version;
				ghc::filesystem::create_directories(folderPath);

				auto fileName = dep.artifactId + "." + dep.type;
				auto fullPath = folderPath / fileName;

				ofstream outFile;
				outFile.open(fullPath, ios::binary);
				outFile << response;
				outFile.close();

				SPDLOG_INFO("Save {} at {}", fileName, fullPath.generic_string());
				paths.push_back(fullPath.generic_string());
			}
			else
			{
				//if download fail...
			}
		}
		catch (const std::exception& e)
		{
			SPDLOG_ERROR(e.what());
		}
	}
	return paths;
}

bool CommunicationManager::ConfirmUpdateStatus(
	const vector<InstallationResult>& results)
{
	nlohmann::json body;
	body["deviceId"] = mDeviceId;

	auto resultArray = nlohmann::json::array();
	for (auto result : results)
	{
		nlohmann::json res;
		res["appId"] = result.appId;
		res["versionId"] = result.versionId;
		if (true == result.status)
			res["status"] = "SUCESS";
		else
			res["status"] = "FAILURE";
		if (false == result.message.empty())
			res["message"] = result.message;
		resultArray.push_back(res);
	}
	body["result"] = resultArray;

	auto content = body.dump();
	try
	{
		HttpAuthorization auth;
		auth.token = mToken;

		auto response = PostToServer(Utils::API_CONFIRM_UPATE_STATUS, content, auth);
		if (response == "true")
			return true;
	}
	catch (const std::exception& e)
	{
		SPDLOG_ERROR(e.what());
	}
	return false;
}