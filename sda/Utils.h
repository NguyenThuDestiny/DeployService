#ifndef SDA_UTILS_H
#define SDA_UTILS_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>

using namespace std;

struct UpdateInfo {
	string appId;
	string versionId;
	string versionName;
	string downloadUrl;
	string token;
};

struct AppInfo
{
	string appId;
	string versionId;
};

struct Repository {
	string id;
	string url;
};

struct Dependency {
	string groupid;
	string artifactId;
	string version;
	string type;

	string Path() {
		return "/" +
			groupid + "/" +
			artifactId + "/" +
			version + "/" +
			artifactId + "-" + version + "." + type;
	}
};

struct PomData
{
	string modelVersionl;
	string groupId;
	string artifactId;
	string version;
	string packaging;

	vector<Dependency> dependencies;
	vector<Repository> repositories;

	string raw;
	vector<string> downloaded;
	string versionId;

	operator bool() const { return !groupId.empty(); }
	bool operator==(std::nullptr_t) const { return groupId.empty(); }
	bool operator!=(std::nullptr_t) const { return !groupId.empty(); }

};

struct InstallationResult {
	string appId;
	string versionId;
	bool status;
	string message;
};


struct HttpAuthorization {
	string token;
	string username;
	string password;
};


class Utils
{
public:
	static string API_CREDENTIALS_URL;
	static string API_CREDENTIALS_CLIENT_ID;
	static string API_CREDENTIALS_CLIEN_SECRET;

	static string API_HOST;
	static string API_REGISTER;
	static string API_CHECK_STATUS;
	static string API_CONFIRM_UPATE_STATUS;
	static string PATH_CONFIG_FILE;
	static string PATH_LOCAL_VERSION_DIR;
	static string PATH_DOWNLOAD_DIR;

	static PomData ParsePom(const string& pom_content);
	static int VersionCompare(const string& v1, const string& v2);
	static string GetHost(const string& uri);
	static string GetPath(const string& uri);
	static int Random(const int& min, const int& max);
	static string GetDateTimeString();
	static long long GetTimestamp();

	class Initializer
	{
	public:
		Initializer();
	};

	friend class Initializer;
	static Initializer initializer;
};

#endif
