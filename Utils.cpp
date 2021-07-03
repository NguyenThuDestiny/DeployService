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
	Utils::PATH_LOCAL_VERSION_DIR = localPath;
	ghc::filesystem::create_directories(Utils::PATH_LOCAL_VERSION_DIR);

	//create downloead folder to download binary file
	auto downloadPath = cwd / "download";
	Utils::PATH_DOWNLOAD_DIR = downloadPath;
	error_code ec;
	ghc::filesystem::remove_all(Utils::PATH_DOWNLOAD_DIR, ec);
	ghc::filesystem::create_directories(Utils::PATH_DOWNLOAD_DIR);

	auto configPath = cwd / "config.json";
	Utils::PATH_CONFIG_FILE = configPath;
}