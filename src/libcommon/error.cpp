#include "stdafx.h"
#include "error.h"
#include "string.h"
#include <exception>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace common::error {

std::wstring FormatWindowsError(DWORD errorCode)
{
	LPWSTR buffer;

	auto status = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr, errorCode, 0, (LPWSTR)&buffer, 0, nullptr);

	if (0 == status)
	{
		std::wstringstream ss;

		ss << L"System error 0x" << std::setw(8) << std::setfill(L'0') << std::hex << errorCode;

		return ss.str();
	}

	auto result = common::string::TrimRight(std::wstring(buffer));
	LocalFree(buffer);

	return result;
}

std::string FormatWindowsErrorPlain(DWORD errorCode)
{
	//
	// Duplicated logic, but preferred to converting the string from
	// wide char to multibyte
	//

	LPSTR buffer;

	auto status = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr, errorCode, 0, (LPSTR)&buffer, 0, nullptr);

	if (0 == status)
	{
		std::stringstream ss;

		ss << "System error 0x" << std::setw(8) << std::setfill('0') << std::hex << errorCode;

		return ss.str();
	}

	auto result = common::string::TrimRight(std::string(buffer));
	LocalFree(buffer);

	return result;
}

void Throw(const char *operation, DWORD errorCode)
{
	std::stringstream ss;

	ss << operation << ": " << common::error::FormatWindowsErrorPlain(errorCode);

	if (std::current_exception())
	{
		std::throw_with_nested(std::runtime_error(ss.str()));
	}

	throw std::runtime_error(ss.str());
}

void UnwindException(const std::exception &err, std::shared_ptr<common::logging::ILogSink> logSink)
{
	logSink->error(err.what());

	try
	{
		std::rethrow_if_nested(err);
	}
	catch (const std::exception &innerErr)
	{
		UnwindException(innerErr, logSink);
	}
	catch (...)
	{
	}
}

}
