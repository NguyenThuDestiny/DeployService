#include <httplib.h>
#include <iostream>
#include <pugixml.hpp>
#include "Utils.h"
#include "UpdateService.h"
#define CA_CERT_FILE "../lib/cpp-httplib/ca-bundle.crt"

using namespace std;

void testHttpRequest()
{
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
	httplib::SSLClient cli("localhost", 8080);
	// httplib::SSLclient cli("google.com");
	cli.set_ca_cert_path(CA_CERT_FILE);
	cli.enable_service_certificate_verification(true);
#else
	httplib::Client cli("localhost", 8080);
#endif

	if (auto res = cli.Get("/hi"))
	{
		cout << res->status << endl;
		cout << res->get_header_value("Content-Type") << endl;
		cout << res->body << endl;
	}
	else
	{
		cout << "error code : " << res.error() << std::endl;
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
		auto result = cli.get_openssl_verify_result();
		if (result)
		{
			cout << "verify error: " << X509_verify_cert_error_string(result) << endl;
		}
#endif // !CPPHTTPLIB_OPENSSL_SUPPORT

	}
}

int main(int argc, char* argv[])
{
	if (argc == 1)
	{
		cout << "You need input argument : http, xml to run test";
		return 0;
	}

	for (int i = 0; i < argc; ++i)
	{
		auto arg = string(argv[i]);
		if (arg == "http")
		{
			testHttpRequest();
		}
		else if (arg == "xml")
		{
			int tmp = 1;
		}
	}
	return 0;
}