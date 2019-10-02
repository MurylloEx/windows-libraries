#include "stdafx.h"
#include "string.h"
#include "memory.h"
#include <algorithm>
#include <iomanip>
#include <memory>
#include <sddl.h>
#include <sstream>
#include <stdexcept>
#include <wchar.h>

namespace common::string {

std::wstring FormatGuid(const GUID &guid)
{
	LPOLESTR buffer;

	auto status = StringFromCLSID(guid, &buffer);

	if (status != S_OK)
	{
		throw std::runtime_error("Failed to format GUID");
	}

	std::wstring formatted(buffer);

	CoTaskMemFree(buffer);

	return formatted;
}

std::wstring FormatSid(const SID &sid)
{
	LPWSTR buffer;

	auto status = ConvertSidToStringSidW(const_cast<SID *>(&sid), &buffer);

	if (0 == status)
	{
		throw std::runtime_error("Failed to format SID");
	}

	std::wstring formatted(buffer);

	LocalFree((HLOCAL)buffer);

	return formatted;
}

std::wstring Join(const std::vector<std::wstring> &parts, const std::wstring &delimiter)
{
	switch (parts.size())
	{
	case 0:
		return L"";
	case 1:
		return parts[0];
	default:
		{
			std::wstring joined;

			size_t reserveSize = 0;

			std::for_each(parts.begin(), parts.end(), [&reserveSize, &delimiter](const std::wstring &part)
			{
				reserveSize += (part.size() + delimiter.size());
			});

			joined.reserve(reserveSize);

			std::for_each(parts.begin(), parts.end(), [&joined, &delimiter](const std::wstring &part)
			{
				if (!joined.empty())
				{
					joined.append(delimiter);
				}

				joined.append(part);
			});

			return joined;
		}
	};
}

std::wstring FormatIpv4(uint32_t ip)
{
	std::wstringstream ss;

	ss	<< ((ip & 0xFF000000) >> 24) << L'.'
		<< ((ip & 0x00FF0000) >> 16) << L'.'
		<< ((ip & 0x0000FF00) >> 8) << L'.'
		<< ((ip & 0x000000FF));

	return ss.str();
}

std::wstring FormatIpv4(uint32_t ip, uint8_t routingPrefix)
{
	std::wstringstream ss;

	ss << FormatIpv4(ip) << L"/" << routingPrefix;

	return ss.str();
}

std::wstring FormatIpv6(const uint8_t ip[16])
{
	//
	// TODO: Omit longest sequence of zeros to create compact representation
	//

	std::wstringstream ss;

	auto wptr = (const uint16_t *)ip;

	//
	// Since this is executing on a little-endian system
	// the words will be byte swapped when read into registers
	//

	const auto swap = ::common::memory::ByteSwap;

	ss	<< std::hex
		<< swap(*(wptr + 0)) << L':' << swap(*(wptr + 1)) << L':'
		<< swap(*(wptr + 2)) << L':' << swap(*(wptr + 3)) << L':'
		<< swap(*(wptr + 4)) << L':' << swap(*(wptr + 5)) << L':'
		<< swap(*(wptr + 6)) << L':' << swap(*(wptr + 7));

	return ss.str();
}

std::wstring FormatIpv6(const uint8_t ip[16], uint8_t routingPrefix)
{
	std::wstringstream ss;

	ss << FormatIpv6(ip) << L"/" << routingPrefix;

	return ss.str();
}

std::wstring FormatTime(const FILETIME &filetime)
{
	FILETIME ft2;

	if (FALSE == FileTimeToLocalFileTime(&filetime, &ft2))
	{
		throw std::runtime_error("Failed to convert time");
	}

	return FormatLocalTime(ft2);
}

std::wstring FormatLocalTime(const FILETIME &filetime)
{
	SYSTEMTIME st;

	if (FALSE == FileTimeToSystemTime(&filetime, &st))
	{
		throw std::runtime_error("Failed to convert time");
	}

	std::wstringstream ss;

	ss << st.wYear << L'-'
		<< std::setw(2) << std::setfill(L'0') << st.wMonth << L'-'
		<< std::setw(2) << std::setfill(L'0') << st.wDay << L' '
		<< std::setw(2) << std::setfill(L'0') << st.wHour << L':'
		<< std::setw(2) << std::setfill(L'0') << st.wMinute << L':'
		<< std::setw(2) << std::setfill(L'0') << st.wSecond;

	return ss.str();
}

std::wstring Lower(const std::wstring &str)
{
	auto bufferSize = str.size() + 1;

	auto buffer = std::make_unique<wchar_t[]>(bufferSize);
	wcscpy_s(buffer.get(), bufferSize, str.c_str());

	_wcslwr_s(buffer.get(), bufferSize);

	return buffer.get();
}

std::vector<std::wstring> Tokenize(const std::wstring &str, const std::wstring &delimiters)
{
	auto bufferSize = str.size() + 1;

	auto buffer = std::make_unique<wchar_t[]>(bufferSize);
	wcscpy_s(buffer.get(), bufferSize, str.c_str());

	wchar_t *context = nullptr;

	auto token = wcstok_s(buffer.get(), delimiters.c_str(), &context);

	std::vector<std::wstring> tokens;

	while (token != nullptr)
	{
		tokens.push_back(token);
		token = wcstok_s(nullptr, delimiters.c_str(), &context);
	}

	return tokens;
}

std::string ToAnsi(const std::wstring &str)
{
	std::string ansi;

	ansi.reserve(str.size());

	std::transform(str.begin(), str.end(), std::back_inserter(ansi), [](wchar_t c)
	{
		return (c > 255 ? '?' : static_cast<char>(c));
	});

	return ansi;
}

std::wstring ToWide(const std::string &str)
{
	return std::wstring(str.begin(), str.end());
}

std::wstring Summary(const std::wstring &str, size_t max)
{
	if (str.size() <= max)
	{
		return str;
	}

	const wchar_t *padding = L"...";
	const size_t paddingLength = 3;

	if (max < paddingLength)
	{
		throw std::runtime_error("Requested summary is too short");
	}

	auto summary = str.substr(0, max - paddingLength);
	summary.append(padding);

	return summary;
}

KeyValuePairs SplitKeyValuePairs(const std::vector<std::wstring> &serializedPairs)
{
	KeyValuePairs result;

	for (const auto &pair : serializedPairs)
	{
		auto index = pair.find(L'=');

		if (index == std::wstring::npos)
		{
			// Insert key with empty value.
			result.insert(std::make_pair(pair, L""));
		}
		else
		{
			result.insert(std::make_pair(
				pair.substr(0, index),
				pair.substr(index + 1)
			));
		}
	}

	return result;
}

const char *TrimChars = "\r\n\t ";
const wchar_t *WideTrimChars = L"\r\n\t ";

}
