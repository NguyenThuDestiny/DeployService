#include "Utils.h"
#include "SdaException.h"
#include <pugixml.hpp>
#include <url.hpp>
#include <spdlog/spdlog.h>
#include <chrono>

#include <whereami.h>
#include <filesystem.hpp>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#define LOG_FILE_LOCATION "core.log"

string Utils::API_CREDENTIALS_URL = "";
string Utils::API_CREDENTIALS_CLIENT_ID = "";
string Utils::API_CREDENTIALS_CLIEN_SECRET = "";

string Utils::API_HOST = "";
string Utils::API_REGISTER = "";
string Utils::API_CHECK_STATUS = "";
string Utils::API_CONFIRM_UPATE_STATUS = "";

string Utils::PATH_CONFIG_FILE = "./config.json";
string Utils::PATH_LOCAL_VERSION_DIR = "./local";
string Utils::PATH_DOWNLOAD_DIR = "./download";

Utils::Initializer Utils::initializer;

Utils::Initializer::Initializer() {
	//create random seed
	srand((unsigned int)time(NULL));

	//get current executable director
	auto cwd = ghc::filesystem::current_path();
	int dirname_length;
	int length = wai_getExecutablePath(NULL, 0, &dirname_length);
	char* path = (char*)malloc(length + 1);
	if (path)
	{
		wai_getExecutablePath(path, length, &dirname_length);
		path[dirname_length] = '\0';
	}
	else
	{
		SPDLOG_ERROR("Can not get executable directory");
	}

	try {
		auto prevLogger = spdlog::get("multi_sink");
		if (NULL == prevLogger)
		{
			auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
			console_sink->set_level(spdlog::level::info);

			//crete a file rotation logger with 5mb size max and 3 rotated files
			auto max_size = 1048576 * 5;
			auto max_files = 3;
			auto log_path = cwd / LOG_FILE_LOCATION;
			auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_path.generic_string(), max_size, max_files, false);
			file_sink->set_level(spdlog::level::trace);

			std::vector<spdlog::sink_ptr> sinks({ console_sink, file_sink });
			auto logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());
			logger->flush_on(spdlog::level::err);

			spdlog::register_logger(logger);
			spdlog::set_default_logger(logger);
			spdlog::flush_on(spdlog::level::err);
			spdlog::flush_every(std::chrono::seconds(5));

			SPDLOG_INFO("#########################################");
			SPDLOG_INFO("log will save at {}", log_path.generic_string());
		}
	}
	catch (const spdlog::spdlog_ex& ex)
	{
		cout << "Log init failed:" << ex.what() << endl;
	}

	//create local folder to save pom file
	auto localPath = cwd / "local";
	Utils::PATH_LOCAL_VERSION_DIR = localPath.generic_string();
	ghc::filesystem::create_directories(Utils::PATH_LOCAL_VERSION_DIR);

	//create downloead folder to download binary file
	auto downloadPath = cwd / "download";
	Utils::PATH_DOWNLOAD_DIR = downloadPath.generic_string();
	error_code ec;
	ghc::filesystem::remove_all(Utils::PATH_DOWNLOAD_DIR, ec);
	ghc::filesystem::create_directories(Utils::PATH_DOWNLOAD_DIR);

	auto configPath = cwd / "config.json";
	Utils::PATH_CONFIG_FILE = configPath.generic_string();
}

PomData Utils::ParsePom(const string& pom_content) {
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_string(pom_content.c_str());

	if (!result) {
		SPDLOG_ERROR("Cannot parse pom offset = {} error={}",
			result.offset,
			result.description());
		throw SdaException(result.description(), PARSE_POM_ERROR);
	}

	auto project = doc.child("project");

	PomData pomData;
	pomData.modelVersionl = project.child("modelVersion").text().get();
	pomData.groupId = project.child("groupId").text().get();
	pomData.artifactId = project.child("artifactId").text().get();
	pomData.version = project.child("version").text().get();
	pomData.packaging = project.child("packaging").text().get();
	pomData.versionId = project.child("versionId").text().get();

	auto dependencies = project.child("dependencies");
	for (pugi::xml_node dependency = dependencies.child("dependency"); dependency; dependency = dependency.next_sibling("dependency")) {
		Dependency dep;
		dep.groupid = dependency.child("groupId").text().get();
		dep.artifactId = dependency.child("artifactId").text().get();
		dep.version = dependency.child("version").text().get();
		dep.type = dependency.child("type").text().get();

		pomData.dependencies.push_back(dep);
	}

	auto repositories = project.child("repositories");
	for (pugi::xml_node repository = repositories.child("repository"); repository; repository = repository.next_sibling("repository")) {
		Repository rep;
		rep.id = repository.child("id").text().get();
		rep.url = repository.child("url").text().get();

		pomData.repositories.push_back(rep);
	}
	
	return pomData;
}

int Utils::VersionCompare(const string& v1, const string& v2) {
	unsigned int vnum1 = 0, vnum2 = 0;
	for (unsigned int i = 0, j = 0; (i < v1.length()) || (j < v2.length());) {
		while (i < v1.length() && v1[i] != '.') {
			vnum1 = vnum1 * 10 + (v1[i] - '0');
			i++;
		}
		while (j < v2.length() && v2[j] != '.') {
			vnum2 = vnum2 * 10 + (v2[j] - '0');
			j++;
		}
		if (vnum1 > vnum2)
			return 1;
		if (vnum2 > vnum1)
			return -1;

		vnum1 = vnum2 = 0;
		i++;
		j++;
	}
	return 0;
}

string Utils::GetHost(const string& uri) {
	Url url(uri);
	string result = "";
	if (url.host().empty())
		return result;

	if (false == url.scheme().empty())
		result += url.scheme() + "://";
	result += url.host();
	if (false == url.port().empty())
		result += ":" + url.port();
	return result;
}

string Utils::GetPath(const string& uri) {
	Url url(uri);
	string result = url.path();
	auto query = url.query();
	if (false == query.empty()) {
		result += "?";
		for (const auto& q : query) {
			result += q.key() + "=" + q.val();
		}
	}
	return result;
}

int Utils::Random(const int& min, const int& max) {
	return min + (rand() % static_cast<int>(max - min + 1));
}

string Utils::GetDateTimeString() {
	time_t rawtime;
	struct tm* timeinfo;
	char buffer[80];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
	std::string str(buffer);
	
	return str;
}

long long Utils::GetTimestamp() {
	auto sec = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch());
	return sec.count();
}