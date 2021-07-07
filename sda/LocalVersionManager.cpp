#include "LocalVersionManager.h"
#include <spdlog/spdlog.h>
#include <filesystem.hpp>
#include <pugixml.hpp>
#include "SdaException.h"

bool LocalVersionManager::SaveLocalVersionApp(const PomData& pom_data) {
	try {
		auto fileName = pom_data.groupId + ".xml";
		auto filePath = (ghc::filesystem::path(Utils::PATH_LOCAL_VERSION_DIR) / fileName).generic_string();

		// process add versionId to pom xml

		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load_string(pom_data.raw.c_str());

		if (!result)
		{
			SPDLOG_ERROR("Can not parse pom offset={} error={}",
				result.offset,
				result.description());
			throw SdaException(result.description(), PARSE_POM_ERROR);
		}

		auto project = doc.child("project");
		auto versionIdNode = project.append_child("versionId");
		versionIdNode.text().set(pom_data.versionId.c_str());

		auto saveSuccess = doc.save_file(filePath.c_str(), PUGIXML_TEXT(" "));
		
		if (false == saveSuccess) {
			SPDLOG_ERROR("Can not save pom to {}", filePath);
			return false;
		}

		SPDLOG_INFO("Save pom to app={}@{} at {}",
			pom_data.groupId,
			pom_data.version,
			filePath);
		return true;
	}
	catch (const std::exception& e) {
		SPDLOG_ERROR(e.what());
	}
	return false;
}

PomData LocalVersionManager::GetLocalPom(const string& groupId)
{
	PomData pomData;
	try
	{
		auto fileName = groupId + ".xml";
		auto filePath = (ghc::filesystem::path(Utils::PATH_LOCAL_VERSION_DIR) / fileName).generic_string();

		ifstream pomFile(filePath);
		if (pomFile.good())
		{
			string pomContent((std::istreambuf_iterator<char>(pomFile)),
				std::istreambuf_iterator<char>());
			pomData = Utils::ParsePom(pomContent);

			SPDLOG_INFO("Get pom of app={}@{} at {}",
				pomData.groupId,
				pomData.version,
				filePath);
		}
		else
		{
			SPDLOG_ERROR("Not found pom at {}", filePath);
		}
		pomFile.close();
	}
	catch (const std::exception& e)
	{
		SPDLOG_ERROR(e.what());
	}

	return pomData;
}

vector<Dependency> LocalVersionManager::CompareWithLocalPom(const PomData& pom_data)
{
	vector<Dependency> deps;
	auto localPom = GetLocalPom(pom_data.groupId);

	//no local version => download all
	if (!localPom)
	{
		SPDLOG_INFO("Add all dependency, because not found local pom of{}", pom_data.groupId);
		for (auto dep : pom_data.dependencies)
		{
			deps.push_back(dep);
			SPDLOG_INFO("Add app = {} dep = {} new = {}",
				pom_data.groupId,
				dep.artifactId + "." + dep.type,
				dep.version);
		}
		return deps;
	}

	// check pom version
	auto pomNotUpdate = Utils::VersionCompare(localPom.version, pom_data.version) >= 0;
	if (pomNotUpdate)
	{
		SPDLOG_INFO("Local pom has version >= than remote pom {} vs {}",
			localPom.version,
			pom_data.version);
		return deps;
	}

	// download all
	for (auto dep : pom_data.dependencies)
	{
		deps.push_back(dep);
		SPDLOG_INFO("Add app = {} dep = {} new = {}",
			pom_data.groupId,
			dep.artifactId + "." + dep.type,
			dep.version);
	}
	return deps;
}

vector<AppInfo> LocalVersionManager::GetLocalAppInfor()
{
	vector<AppInfo> list;

	// get all file in folder locla
	auto pomDirectory = Utils::PATH_LOCAL_VERSION_DIR;
	int countTotalFiles = 0;

	try
	{
		for (const auto& entry : ghc::filesystem::directory_iterator(pomDirectory))
		{
			countTotalFiles++;
			auto file = entry.path().generic_string();
			try
			{
				auto groupId = entry.path().stem().generic_string();
				if (groupId.empty())
				{
					continue;
				}

				auto pomData = GetLocalPom(groupId);
				if (pomData)
				{
					AppInfo appInfo;
					appInfo.appId = pomData.groupId;
					appInfo.versionId = pomData.versionId;
					list.push_back(appInfo);
					SPDLOG_INFO("Add app={}@{} to info list", appInfo.appId, appInfo.versionId);
				}
				else
				{
					SPDLOG_ERROR("Can not parese pom file={}", file);
				}
			}
			catch (const std::exception& e)
			{
				SPDLOG_ERROR("Can not open pom file={}", file);
				SPDLOG_ERROR(e.what());
			}
		}
	}
	catch (const std::exception& e)
	{
		SPDLOG_ERROR(e.what());
	}

	SPDLOG_INFO("Get app info of {}/{} pom files", list.size(), countTotalFiles);
	return list;
}