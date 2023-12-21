//	MIT License
//
//	Copyright(c) 2017 Thomas Monkman
//
//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files(the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions :
//
//	The above copyright notice and this permission notice shall be included in all
//	copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//	SOFTWARE.

#ifndef IE_FILESYSTEM_HPP
#define IE_FILESYSTEM_HPP

#include <cstdio>
#include <fstream>
#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif
#	include <windows.h>
#	include <cstdlib>
#	include <cstdio>
#	include <tchar.h>
#	include <Pathcch.h>
#	include <shlwapi.h>
#endif // _WIN32

#if __unix__
#	include <stdio.h>
#	include <stdlib.h>
#	include <errno.h>
#	include <sys/types.h>
#	include <sys/inotify.h>
#	include <sys/stat.h>
#	include <fcntl.h>
#	include <dirent.h>
#	include <unistd.h>
#endif // __unix__

#ifdef __linux__
#	include <linux/limits.h>
#endif

#include <functional>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <utility>
#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <system_error>
#include <string>
#include <algorithm>
#include <type_traits>
#include <future>
#include <regex>
#include <cstddef>
#include <cstring>
#include <cwchar>
#include <cassert>
#include <cstdlib>
#include <iostream>

namespace filesystem {
enum class Event
{
	added,
	removed,
	modified,
	renamed_old,
	renamed_new
};

template <typename StringType> struct IsWChar
{
	static constexpr bool value = false;
};

template <> struct IsWChar<wchar_t>
{
	static constexpr bool value = true;
};

template <typename Fn, typename... Args> struct Invokable
{
	static Fn make() { return (Fn*)0; }

	template <typename T> static T defaultValue() { return *(T*)0; }

	static void call(int) { make()(defaultValue<Args...>()); }

	static int call(long value);

	static constexpr bool value = std::is_same<decltype(call(0)), int>::value;
};

template <typename StringType>
static typename std::
    enable_if<std::is_same<typename StringType::value_type, wchar_t>::value, bool>::type
    isParentOrSelfDirectory(const StringType& path)
{
	return path == L"." || path == L"..";
}

template <typename StringType>
static typename std::
    enable_if<std::is_same<typename StringType::value_type, char>::value, bool>::type
    isParentOrSelfDirectory(const StringType& path)
{
	return path == "." || path == "..";
}

/**
	* \class FileWatch
	*
	* \brief Watches a folder or file, and will notify of changes via function callback.
	*
	* \author Thomas Monkman
	*
	*/
template <class StringType> class FileWatch
{
	typedef typename StringType::value_type C;
	typedef std::basic_string<C, std::char_traits<C>> UnderpinningString;
	typedef std::basic_regex<C, std::regex_traits<C>> UnderpinningRegex;

public:
	FileWatch(
	    StringType path,
	    UnderpinningRegex pattern,
	    std::function<void(const StringType& file, const Event event_type)> callback)
	    : _path(absolute_path_of(path)),
	      _pattern(pattern),
	      _callback(callback),
	      _directory(get_directory(path))
	{
		init();
	}

	FileWatch(
	    StringType path,
	    std::function<void(const StringType& file, const Event event_type)> callback)
	    : FileWatch<StringType>(path, UnderpinningRegex(_regex_all), callback)
	{
	}

	~FileWatch() { destroy(); }

	FileWatch(const FileWatch<StringType>& other)
	    : FileWatch<StringType>(other._path, other._callback)
	{
	}

	FileWatch<StringType>& operator=(const FileWatch<StringType>& other)
	{
		if (this == &other) { return *this; }

		destroy();
		_path = other._path;
		_callback = other._callback;
		_directory = get_directory(other._path);
		init();
		return *this;
	}

	// Const memeber varibles don't let me implent moves nicely, if moves are really wanted std::unique_ptr should be used and move that.
	FileWatch<StringType>(FileWatch<StringType>&&) = delete;
	FileWatch<StringType>& operator=(FileWatch<StringType>&&) & = delete;

private:
	static constexpr C _regex_all[] = {'.', '*', '\0'};
	static constexpr C _this_directory[] = {'.', '/', '\0'};

	struct PathParts
	{
		PathParts(StringType directory, StringType filename)
		    : directory(directory), filename(filename)
		{
		}
		StringType directory;
		StringType filename;
	};
	const StringType _path;

	UnderpinningRegex _pattern;

	static constexpr std::size_t _buffer_size = {1024 * 256};

	// only used if watch a single file
	StringType _filename;

	std::function<void(const StringType& file, const Event event_type)> _callback;

	std::thread _watch_thread;

	std::condition_variable _cv;
	std::mutex _callback_mutex;
	std::vector<std::pair<StringType, Event>> _callback_information;
	std::thread _callback_thread;

	std::promise<void> _running;
	std::atomic<bool> _destory = {false};
	bool _watching_single_file = {false};

#pragma mark "Platform specific data"
#ifdef _WIN32
	HANDLE _directory = {nullptr};
	HANDLE _close_event = {nullptr};

	const DWORD _listen_filters =
	    FILE_NOTIFY_CHANGE_SECURITY | FILE_NOTIFY_CHANGE_CREATION |
	    FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_LAST_WRITE |
	    FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_DIR_NAME |
	    FILE_NOTIFY_CHANGE_FILE_NAME;

	const std::unordered_map<DWORD, Event> _event_type_mapping = {
	    {FILE_ACTION_ADDED, Event::added},
	    {FILE_ACTION_REMOVED, Event::removed},
	    {FILE_ACTION_MODIFIED, Event::modified},
	    {FILE_ACTION_RENAMED_OLD_NAME, Event::renamed_old},
	    {FILE_ACTION_RENAMED_NEW_NAME, Event::renamed_new}};
#endif // _WIN32

#if __unix__
	struct FolderInfo
	{
		int folder;
		int watch;
	};

	FolderInfo _directory;

	const std::uint32_t _listen_filters = IN_MODIFY | IN_CREATE | IN_DELETE;

	const static std::size_t event_size = (sizeof(struct inotify_event));
#endif // __unix__

	void init()
	{
#ifdef _WIN32
		_close_event = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (!_close_event) { throw std::system_error(GetLastError(), std::system_category()); }
#endif // _WIN32

		_callback_thread = std::thread([this]() {
			try
			{
				callback_thread();
			}
			catch (...)
			{
				try
				{
					_running.set_exception(std::current_exception());
				}
				catch (...)
				{
				} // set_exception() may throw too
			}
		});

		_watch_thread = std::thread([this]() {
			try
			{
				monitor_directory();
			}
			catch (...)
			{
				try
				{
					_running.set_exception(std::current_exception());
				}
				catch (...)
				{
				} // set_exception() may throw too
			}
		});

		std::future<void> future = _running.get_future();
		future.get(); //block until the monitor_directory is up and running
	}

	void destroy()
	{
		_destory = true;
		_running = std::promise<void>();

#ifdef _WIN32
		SetEvent(_close_event);
#elif __unix__
		inotify_rm_watch(_directory.folder, _directory.watch);
#endif // _WIN32

		_cv.notify_all();
		_watch_thread.join();
		_callback_thread.join();

#ifdef _WIN32
		CloseHandle(_directory);
#elif __unix__
		close(_directory.folder);
#endif // _WIN32
	}

	const PathParts split_directory_and_file(const StringType& path) const
	{
		const auto predict = [](C character) {
#ifdef _WIN32
			return character == C('\\') || character == C('/');
#elif __unix__
			return character == C('/');
#endif // _WIN32
		};

		UnderpinningString path_string = path;
		const auto pivot =
		    std::find_if(path_string.rbegin(), path_string.rend(), predict).base();
		//if the path is something like "test.txt" there will be no directory part, however we still need one, so insert './'
		const StringType directory = [&]() {
			const auto extracted_directory = UnderpinningString(path_string.begin(), pivot);
			return (extracted_directory.size() > 0)
			           ? extracted_directory
			           : UnderpinningString(_this_directory);
		}();
		const StringType filename = UnderpinningString(pivot, path_string.end());
		return PathParts(directory, filename);
	}

	bool pass_filter(const UnderpinningString& file_path)
	{
		if (_watching_single_file)
		{
			const UnderpinningString extracted_filename = {
			    split_directory_and_file(file_path).filename};
			//if we are watching a single file, only that file should trigger action
			return extracted_filename == _filename;
		}
		return std::regex_match(file_path, _pattern);
	}

#ifdef _WIN32
	template <typename... Args> DWORD GetFileAttributesX(const char* lpFileName, Args... args)
	{
		return GetFileAttributesA(lpFileName, args...);
	}
	template <typename... Args>
	DWORD GetFileAttributesX(const wchar_t* lpFileName, Args... args)
	{
		return GetFileAttributesW(lpFileName, args...);
	}

	template <typename... Args> HANDLE CreateFileX(const char* lpFileName, Args... args)
	{
		return CreateFileA(lpFileName, args...);
	}
	template <typename... Args> HANDLE CreateFileX(const wchar_t* lpFileName, Args... args)
	{
		return CreateFileW(lpFileName, args...);
	}

	HANDLE get_directory(const StringType& path)
	{
		auto file_info = GetFileAttributesX(path.c_str());

		if (file_info == INVALID_FILE_ATTRIBUTES)
		{
			throw std::system_error(GetLastError(), std::system_category());
		}
		_watching_single_file = (file_info & FILE_ATTRIBUTE_DIRECTORY) == false;

		const StringType watch_path = [this, &path]() {
			if (_watching_single_file)
			{
				const auto parsed_path = split_directory_and_file(path);
				_filename = parsed_path.filename;
				return parsed_path.directory;
			}
			else { return path; }
		}();

		HANDLE directory = CreateFileX(
		    watch_path.c_str(), // pointer to the file name
		    FILE_LIST_DIRECTORY, // access (read/write) mode
		    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, // share mode
		    nullptr, // security descriptor
		    OPEN_EXISTING, // how to create
		    FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, // file attributes
		    HANDLE(0)); // file with attributes to copy

		if (directory == INVALID_HANDLE_VALUE)
		{
			throw std::system_error(GetLastError(), std::system_category());
		}
		return directory;
	}

	void convert_wstring(const std::wstring& wstr, std::string& out)
	{
		int size_needed =
		    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
		out.resize(size_needed, '\0');
		WideCharToMultiByte(
		    CP_UTF8, 0, &wstr[0], (int)wstr.size(), &out[0], size_needed, NULL, NULL);
	}

	void convert_wstring(const std::wstring& wstr, std::wstring& out) { out = wstr; }

	void monitor_directory()
	{
		std::vector<BYTE> buffer(_buffer_size);
		DWORD bytes_returned = 0;
		OVERLAPPED overlapped_buffer{0};

		overlapped_buffer.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		if (!overlapped_buffer.hEvent)
		{
			std::cerr << "Error creating monitor event" << std::endl;
		}

		std::array<HANDLE, 2> handles{overlapped_buffer.hEvent, _close_event};

		auto async_pending = false;
		_running.set_value();
		do {
			std::vector<std::pair<StringType, Event>> parsed_information;
			ReadDirectoryChangesW(
			    _directory, buffer.data(), static_cast<DWORD>(buffer.size()), TRUE,
			    _listen_filters, &bytes_returned, &overlapped_buffer, NULL);

			async_pending = true;

			switch (WaitForMultipleObjects(2, handles.data(), FALSE, INFINITE))
			{
				case WAIT_OBJECT_0: {
					if (!GetOverlappedResult(
					        _directory, &overlapped_buffer, &bytes_returned, TRUE))
					{
						throw std::system_error(GetLastError(), std::system_category());
					}
					async_pending = false;

					if (bytes_returned == 0) { break; }

					auto* file_information =
					    reinterpret_cast<FILE_NOTIFY_INFORMATION*>(&buffer[0]);
					do {
						std::wstring changed_file_w{
						    file_information->FileName,
						    file_information->FileNameLength /
						        sizeof(file_information->FileName[0])};
						UnderpinningString changed_file;
						convert_wstring(changed_file_w, changed_file);
						if (pass_filter(changed_file))
						{
							parsed_information.emplace_back(
							    StringType{changed_file},
							    _event_type_mapping.at(file_information->Action));
						}

						if (file_information->NextEntryOffset == 0) { break; }

						file_information = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
						    reinterpret_cast<BYTE*>(file_information) +
						    file_information->NextEntryOffset);
					} while (true);
					break;
				}
				case WAIT_OBJECT_0 + 1:
					// quit
					break;
				case WAIT_FAILED: break;
			}
			//dispatch callbacks
			{
				std::lock_guard<std::mutex> lock(_callback_mutex);
				_callback_information.insert(
				    _callback_information.end(), parsed_information.begin(),
				    parsed_information.end());
			}
			_cv.notify_all();
		} while (!_destory);

		if (async_pending)
		{
			//clean up running async io
			CancelIo(_directory);
			GetOverlappedResult(_directory, &overlapped_buffer, &bytes_returned, TRUE);
		}
	}
#endif // _WIN32

#if __unix__

	bool is_file(const StringType& path) const
	{
		struct stat statbuf = {};
		if (stat(path.c_str(), &statbuf) != 0)
		{
			throw std::system_error(errno, std::system_category());
		}
		return S_ISREG(statbuf.st_mode);
	}

	FolderInfo get_directory(const StringType& path)
	{
		const auto folder = inotify_init();
		if (folder < 0) { throw std::system_error(errno, std::system_category()); }

		_watching_single_file = is_file(path);

		const StringType watch_path = [this, &path]() {
			if (_watching_single_file)
			{
				const auto parsed_path = split_directory_and_file(path);
				_filename = parsed_path.filename;
				return parsed_path.directory;
			}
			else { return path; }
		}();

		const auto watch =
		    inotify_add_watch(folder, watch_path.c_str(), IN_MODIFY | IN_CREATE | IN_DELETE);
		if (watch < 0) { throw std::system_error(errno, std::system_category()); }
		return {folder, watch};
	}

	void monitor_directory()
	{
		std::vector<char> buffer(_buffer_size);

		_running.set_value();
		while (_destory == false)
		{
			const auto length =
			    read(_directory.folder, static_cast<void*>(buffer.data()), buffer.size());
			if (length > 0)
			{
				int i = 0;
				std::vector<std::pair<StringType, Event>> parsed_information;
				while (i < length)
				{
					struct inotify_event* event =
					    reinterpret_cast<struct inotify_event*>(&buffer[i]); // NOLINT
					if (event->len)
					{
						const UnderpinningString changed_file{event->name};
						if (pass_filter(changed_file))
						{
							if (event->mask & IN_CREATE)
							{
								parsed_information
								    .emplace_back(StringType{changed_file}, Event::added);
							}
							else if (event->mask & IN_DELETE)
							{
								parsed_information
								    .emplace_back(StringType{changed_file}, Event::removed);
							}
							else if (event->mask & IN_MODIFY)
							{
								parsed_information
								    .emplace_back(StringType{changed_file}, Event::modified);
							}
						}
					}
					i += event_size + event->len;
				}
				//dispatch callbacks
				{
					std::lock_guard<std::mutex> lock(_callback_mutex);
					_callback_information.insert(
					    _callback_information.end(), parsed_information.begin(),
					    parsed_information.end());
				}
				_cv.notify_all();
			}
		}
	}
#endif // __unix__

#if defined(__unix__)
	static StringType absolute_path_of(const StringType& path)
	{
		char buf[PATH_MAX];
		const char* str = buf;
		struct stat stat;
		mbstate_t state;

		realpath((const char*)path.c_str(), buf);
		::stat((const char*)path.c_str(), &stat);

		if (stat.st_mode & S_IFREG || stat.st_mode & S_IFLNK)
		{
			size_t len = strlen(buf);

			for (size_t i = len - 1; i >= 0; i--)
			{
				if (buf[i] == '/')
				{
					buf[i] = '\0';
					break;
				}
			}
		}

		if (IsWChar<C>::value)
		{
			size_t needed = mbsrtowcs(nullptr, &str, 0, &state) + 1;
			StringType s;

			s.reserve(needed);
			mbsrtowcs((wchar_t*)&s[0], &str, s.size(), &state);
			return s;
		}
		return StringType{buf};
	}
#elif _WIN32
	static StringType absolute_path_of(const StringType& path)
	{
		constexpr size_t size = IsWChar<C>::value ? MAX_PATH : 32767 * sizeof(wchar_t);
		char buf[size];

		DWORD length =
		    IsWChar<C>::value
		        ? GetFullPathNameW(
		              (LPCWSTR)path.c_str(), size / sizeof(TCHAR), (LPWSTR)buf, nullptr)
		        : GetFullPathNameA((LPCSTR)path.c_str(), size / sizeof(TCHAR), buf, nullptr);
		return StringType{(C*)buf, length};
	}
#endif

	void callback_thread()
	{
		while (!_destory)
		{
			std::unique_lock<std::mutex> lock(_callback_mutex);
			if (_callback_information.empty() && !_destory)
			{
				_cv.wait(lock, [this] { return !_callback_information.empty() || _destory; });
			}
			decltype(_callback_information) callback_information = {};
			std::swap(callback_information, _callback_information);
			lock.unlock();

			for (const auto& file : callback_information)
			{
				if (_callback)
				{
					try
					{
						_callback(file.first, file.second);
					}
					catch (const std::exception&)
					{
					}
				}
			}
		}
	}
};

template <class StringType>
constexpr typename FileWatch<StringType>::C FileWatch<StringType>::_regex_all[];
template <class StringType>
constexpr typename FileWatch<StringType>::C FileWatch<StringType>::_this_directory[];
} // namespace filesystem

namespace filesystem {

inline std::unique_ptr<char[]>
readFile(const char* filename, size_t* fileSize)
{
	FILE* f = fopen(filename, "rb");

	if (!f) return nullptr; // file does not exist

	fseek(f, 0, SEEK_END);
	size_t length = ftell(f);
	fseek(f, 0, SEEK_SET);

	// 1 GiB; best not to load a whole large file in one string
	if (length > 1073741824) return nullptr; // file is too large

	std::unique_ptr<char[]> buffer(new char[length + 1]);

	size_t readLen;
	if (length)
	{
		readLen = fread(buffer.get(), 1, length, f);
		if (length != readLen) return nullptr; // cannot read the file
	}

	fclose(f);

	buffer[length] = '\0';
	if (fileSize) *fileSize = length;

	return buffer;
}

} // namespace filesystem
#endif // IE_FILESYSTEM_HPP



// On linux or none unicode windows change std::wstring
// for std::string or std::filesystem (boost should work as well).

// Simple:
#if 0
filesystem::FileWatch<std::wstring> watch(
    L"C:/Users/User/Desktop/Watch/Test"s,
    [](const std::wstring& path, const filesystem::Event change_type) {
	    std::wcout << path << L"\n";
    });
#endif

// Change Type:
#if 0
filesystem::FileWatch<std::wstring> watch(
    L"C:/Users/User/Desktop/Watch/Test"s,
    [](const std::wstring& path, const filesystem::Event change_type) {
	    std::wcout << path << L" : ";
	    switch (change_type)
	    {
		    case filesystem::Event::added:
			    std::cout << "The file was added to the directory." << '\n';
			    break;
		    case filesystem::Event::removed:
			    std::cout << "The file was removed from the directory." << '\n';
			    break;
		    case filesystem::Event::modified:
			    std::cout
			        << "The file was modified. This can be a change in the time stamp or "
			           "attributes."
			        << '\n';
			    break;
		    case filesystem::Event::renamed_old:
			    std::cout << "The file was renamed and this is the old name." << '\n';
			    break;
		    case filesystem::ChangeType::renamed_new:
			    std::cout << "The file was renamed and this is the new name." << '\n';
			    break;
	    };
    });
#endif

// Regex:
//  Using the standard regex libary you can filter the file paths that will trigger. When using wstring you will have to use std::wregex
#if 0
filesystem::FileWatch<std::wstring> watch(
    L"C:/Users/User/Desktop/Watch/Test"s,
    std::wregex(L"test.*"),
    [](const std::wstring& path, const filesystem::Event change_type) {
	    std::wcout << path << L"\n";
    });
#endif

// Using std::filesystem:
#if 0
filesystem::FileWatch<std::filesystem::path> watch(
    L"C:/Users/User/Desktop/Watch/Test"s,
    [](const std::filesystem::path& path, const filesystem::Event change_type) {
	    std::wcout << std::filesystem::absolute(path) << L"\n";
    });
#endif

// Works with relative paths:
#if 0
filesystem::FileWatch<std::filesystem::path>
    watch(L"./"s, [](const std::filesystem::path& path, const filesystem::Event change_type) {
	    std::wcout << std::filesystem::absolute(path) << L"\n";
    });
#endif

// Single file watch:
#if 0
filesystem::FileWatch<std::wstring>
    watch(L"./test.txt"s, [](const std::wstring& path, const filesystem::Event change_type) {
	    std::wcout << path << L"\n";
    });
#endif
