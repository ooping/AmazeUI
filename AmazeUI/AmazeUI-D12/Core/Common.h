/*-------------------------------------------------- Framework Environment --------------------------------------------------*/
#pragma once

#define RENDERER_D3D9 0
#define RENDERER_D3D12 1

#define NOMINMAX

/*-------------------------------------------------- Header files / Standard --------------------------------------------------*/
// C Lib
#include <ctime>				// 
#include <cmath>
#include <assert.h>
#include <stdio.h>
#include <tchar.h>				// only win??

// STL
#include <string>
#include <vector>
#include <deque>
#include <list>
#include <stack>
#include <variant>
#include <algorithm>
#include <numeric>
#include <functional>		
#include <iostream>
#include <fstream>
#include <sstream>
//#include <format>
#include <map>
#include <unordered_map>
#include <format>

#include <exception>
#include <memory>
#include <stdexcept>

//
#include <thread>             	// std::thread
#include <mutex>              	// std::mutex, std::unique_lock
#include <condition_variable> 	// std::condition_variable

#include <atomic>
#include <chrono>
#include <future>

// DirectX apps don't need GDI
#define NODRAWTEXT
//#define NOGDI
#define NOBITMAP

// Include <mcx.h> if you need this
#define NOMCX

// Include <winsvc.h> if you need this
#define NOSERVICE

// WinHelp is deprecated
#define NOHELP

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <wrl/client.h>
#include <wrl/event.h>



/*-------------------------------------------------- String Helper --------------------------------------------------*/
#if _WIN32
struct StringPasteFromClipboard {
    void operator()(std::wstring& strBuf, HWND dstHwnd);
};

struct StringCopyToClipboard {
    void operator()(const std::wstring& strBuf, HWND srcHwnd);
};
#endif

struct StringHelper {
	static std::wstring StringToWString(const std::string& str) {
        if (str.empty()) {
            return std::wstring();
        }

    #ifdef _WIN32
        int size = MultiByteToWideChar(CP_ACP, 0, str.c_str(), static_cast<int>(str.length()), nullptr, 0);
        std::wstring result(size, 0);
        MultiByteToWideChar(CP_ACP, 0, str.c_str(), static_cast<int>(str.length()), &result[0], size);
        return result;
    #endif
    }

    static std::string WStringToString(const std::wstring& wstr) {
        if (wstr.empty()) {
            return std::string();
        }
 
    #ifdef _WIN32
        int size = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), static_cast<int>(wstr.length()), nullptr, 0, nullptr, nullptr);
        std::string result(size, 0);
        WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), static_cast<int>(wstr.length()), &result[0], size, nullptr, nullptr);
        return result;
    #endif
    }

    static std::wstring ToLower(const std::wstring& str) {
        std::wstring result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::towlower);
        return result;
    }

    static std::wstring ToUpper(const std::wstring& str) {
        std::wstring result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::towupper);
        return result;
    }

    static void SplitWStringToWords(std::wstring line, std::vector<std::wstring>& words, const std::wstring& delimiters) {
        size_t start = 0;
        size_t end = 0;

        while ((start = line.find_first_not_of(delimiters, end)) != std::wstring::npos) {
            end = line.find_first_of(delimiters, start);
            words.push_back(line.substr(start, end - start));
        }
    }

    static void SplitWStringToLineWords(std::wstring str, std::vector<std::vector<std::wstring>>& allLineWordsList, const std::wstring& delimiters) {
        // remove \r
        str.erase(remove(str.begin(), str.end(), L'\r'), str.end());

        // get lines
        std::vector<std::wstring> lines;
        SplitWStringToWords(str, lines, L"\n");

        for (size_t i = 0; i < lines.size(); ++i) {
            std::vector<std::wstring> words;
            SplitWStringToWords(lines[i], words, delimiters);
            allLineWordsList.push_back(words);
        }
    }

    static void SplitStringToWords(std::string line, std::vector<std::string>& words, const std::string& delimiters) {
        size_t start = 0;
        size_t end = 0;

        while ((start = line.find_first_not_of(delimiters, end)) != std::string::npos) {
            end = line.find_first_of(delimiters, start);
            words.push_back(line.substr(start, end - start));
        }
    }

    static void SplitStringToLineWords(std::string str, std::vector<std::vector<std::string>>& allLineWordsList, const std::string& delimiters) {
        // remove \r
        str.erase(remove(str.begin(), str.end(), '\r'), str.end());

        // get lines
        std::vector<std::string> lines;
        SplitStringToWords(str, lines, "\n");

        for (size_t i = 0; i < lines.size(); ++i) {
            std::vector<std::string> words;
            SplitStringToWords(lines[i], words, delimiters);
            allLineWordsList.push_back(words);
        }
    }
};

#define STR_TO_WSTR(str) StringHelper::StringToWString(str)
#define WSTR_TO_STR(wstr) StringHelper::WStringToString(wstr)
#define STR_TO_LOWER(str) StringHelper::ToLower(str)
#define STR_TO_UPPER(str) StringHelper::ToUpper(str)


class UIString {
public:
    UIString(const std::string& str) : _data(STR_TO_WSTR(str)) {}
    UIString(const std::wstring& str) : _data(str) {}
    UIString(const char* str) : _data(STR_TO_WSTR(str)) {}
    UIString(const wchar_t* str) : _data(str) {}

    const char* c_str() const {
        _dataCache = WSTR_TO_STR(_data);
        return _dataCache.c_str();
    }
    const wchar_t* w_str() const { return _data.c_str(); }

    operator std::wstring() const { return _data; }
    operator std::string() const { return WSTR_TO_STR(_data); }

private:
    std::wstring _data;
    mutable std::string _dataCache;
};

/*-------------------------------------------------- SingletonPattern Pattern --------------------------------------------------*/
template<class T>
class SingletonPattern {
public:
    // Get the singleton instance (thread-safe, C++11 guarantees)
    static T* GetSingletonInstance() {
        // C++11 magic statics: Thread-safe initialization guaranteed by standard
        static T instance;
        return &instance;
    }

    // Delete copy and move operations
    SingletonPattern(const SingletonPattern&) = delete;
    SingletonPattern& operator=(const SingletonPattern&) = delete;
    SingletonPattern(SingletonPattern&&) = delete;
    SingletonPattern& operator=(SingletonPattern&&) = delete;

protected:
    SingletonPattern() = default;
    virtual ~SingletonPattern() = default;
};

/*-------------------------------------------------- Thread Manager --------------------------------------------------*/
template<class T>
class SingleThreadHelper {
protected:
    SingleThreadHelper() = default;
    ~SingleThreadHelper() {
        StopThread();
    }
    
    // Start thread with arbitrary parameters (C++20)
    template<typename Func, typename... Args>
    bool StartThread(Func&& func, Args&&... args) {
        std::lock_guard<std::mutex> lock(_threadMutex);
        
        if (_threadData && _threadData->_thread.joinable()) {
            return false;
        }

        _threadData = std::make_unique<ThreadData>();
        
        _threadData->_thread = std::jthread([this,
                                             func = std::forward<Func>(func), 
                                             ...args = std::forward<Args>(args)]() mutable {
            if constexpr (std::is_member_function_pointer_v<std::decay_t<Func>>) {
                (static_cast<T*>(this)->*func)(std::forward<Args>(args)...);
            } else {
                func(std::forward<Args>(args)...);
            }
        });
        
        return true;
    }

    // Stop thread with timeout
    bool StopThread(std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
        std::unique_lock<std::mutex> lock(_threadMutex);
        
        if (!_threadData || !_threadData->_thread.joinable()) {
            _threadData.reset();
            return false;
        }

        _threadData->_thread.request_stop();

        std::jthread tempThread = std::move(_threadData->_thread);
        lock.unlock();

        std::future<void> future = std::async(std::launch::async, 
            [&tempThread]() { tempThread.join(); });
        
        bool success = (future.wait_for(timeout) == std::future_status::ready);
        
        if (!success && tempThread.joinable()) {
            tempThread.detach();
        }
        
        lock.lock();
        _threadData.reset();
        return success;
    }

    // Check if thread should stop
    bool isThreadShouldStop() const {
        std::lock_guard<std::mutex> lock(_threadMutex);
        
        if (!_threadData || !_threadData->_thread.joinable()) {
            return true;
        }
        
        return _threadData->_thread.get_stop_token().stop_requested();
    }

    // Check if thread is running
    bool IsThreadRunning() const {
        std::lock_guard<std::mutex> lock(_threadMutex);
        return _threadData && _threadData->_thread.joinable();
    }

private:
    struct ThreadData {
        std::jthread _thread;
    };

    mutable std::mutex _threadMutex;
    std::unique_ptr<ThreadData> _threadData;

    // Disable copy and assignment
    SingleThreadHelper(const SingleThreadHelper&) = delete;
    SingleThreadHelper& operator=(const SingleThreadHelper&) = delete;
};

template<class T>
class MultiThreadHelper {
protected:
    MultiThreadHelper() = default;
    ~MultiThreadHelper() {
        StopAllThreads();
    }
    
    // Start thread with arbitrary parameters (C++20)
    template<typename Func, typename... Args>
    bool StartThread(const std::string& threadName, Func&& func, Args&&... args) {
        std::lock_guard<std::mutex> lock(_threadMutex);
        
        if (_threads.contains(threadName) && 
            _threads[threadName]->_thread.joinable()) {
            return false;
        }

        _threads[threadName] = std::make_unique<ThreadData>();
        auto& threadData = _threads[threadName];
        
        threadData->_thread = std::jthread([this, threadName,
                                           func = std::forward<Func>(func), 
                                           ...args = std::forward<Args>(args)]() mutable {
            if constexpr (std::is_member_function_pointer_v<std::decay_t<Func>>) {
                (static_cast<T*>(this)->*func)(std::forward<Args>(args)...);
            } else {
                func(std::forward<Args>(args)...);
            }
        });
        
        return true;
    }

    // Stop thread with timeout
    bool StopThread(const std::string& threadName, 
                     std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
        std::unique_lock<std::mutex> lock(_threadMutex);
        
        if (!_threads.contains(threadName) || 
            !_threads[threadName]->_thread.joinable()) {
            _threads.erase(threadName);
            return false;
        }

        auto& threadData = _threads[threadName];
        threadData->_thread.request_stop();

        std::jthread tempThread = std::move(threadData->_thread);
        lock.unlock();

        std::future<void> future = std::async(std::launch::async, 
            [&tempThread]() { tempThread.join(); });
        
        bool success = (future.wait_for(timeout) == std::future_status::ready);
        
        if (!success && tempThread.joinable()) {
            tempThread.detach();
        }
        
        lock.lock();
        _threads.erase(threadName);
        return success;
    }

    // Check if thread should stop
    bool isThreadShouldStop(const std::string& threadName) const {
        std::lock_guard<std::mutex> lock(_threadMutex);
        
        if (!_threads.contains(threadName) || 
            !_threads.at(threadName)->_thread.joinable()) {
            return true;
        }
        
        return _threads.at(threadName)->_thread.get_stop_token().stop_requested();
    }

    // Check if thread is running
    bool IsThreadRunning(const std::string& threadName) const {
        std::lock_guard<std::mutex> lock(_threadMutex);
        return _threads.contains(threadName) && 
               _threads.at(threadName)->_thread.joinable();
    }

    // Get running thread count (C++20)
    size_t GetRunningThreadCount() const {
        std::lock_guard<std::mutex> lock(_threadMutex);
        return std::count_if(_threads.begin(), _threads.end(),
            [](const auto& pair) { return pair.second->_thread.joinable(); });
    }

public:
    // Stop all threads with timeout
    void StopAllThreads(std::chrono::milliseconds timeout = std::chrono::seconds(5)) {
        std::vector<std::pair<std::string, std::jthread>> threadsToJoin;
        
        {
            std::lock_guard<std::mutex> lock(_threadMutex);
            
            for (auto& pair : _threads) {
                if (pair.second->_thread.joinable()) {
                    pair.second->_thread.request_stop();
                    threadsToJoin.emplace_back(pair.first, std::move(pair.second->_thread));
                }
            }
            
            _threads.clear();
        }
        
        for (auto& [name, thread] : threadsToJoin) {
            std::future<void> future = std::async(std::launch::async, 
                [&thread]() { thread.join(); });
            
            if (future.wait_for(timeout) == std::future_status::timeout) {
                if (thread.joinable()) {
                    thread.detach();
                }
            }
        }
    }

private:
    struct ThreadData {
        std::jthread _thread;
    };

    mutable std::mutex _threadMutex;
    std::unordered_map<std::string, std::unique_ptr<ThreadData>> _threads;

    MultiThreadHelper(const MultiThreadHelper&) = delete;
    MultiThreadHelper& operator=(const MultiThreadHelper&) = delete;
};

/*-------------------------------------------------- Timer Helper --------------------------------------------------*/
struct TimerHelper {
    TimerHelper(long long waitTime) : _waitTime(std::chrono::milliseconds(waitTime)) {}

    void BeginWait() {
        _beginTime = std::chrono::steady_clock::now();
    }

    bool BreakWait() {
        _endTime = std::chrono::steady_clock::now();
        auto dxTime = _endTime - _beginTime;

        return dxTime >= _waitTime;
    }

    void EndWait() {
        _endTime = std::chrono::steady_clock::now();
        auto dxTime = _endTime - _beginTime;

        if (dxTime < _waitTime) {
            std::this_thread::sleep_for(_waitTime - dxTime);
        }
    }

    void EndWaitByFlag(bool& flag) {
        while (true) {
            if (flag) {
                break;
            }

            _endTime = std::chrono::steady_clock::now();
            auto dxTime = _endTime - _beginTime;

            if (dxTime < _waitTime) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            } else {
                break;
            }
        }
    }

private:
    std::chrono::steady_clock::time_point _beginTime;
    std::chrono::steady_clock::time_point _endTime;
    std::chrono::milliseconds _waitTime;
};

class DateTimeHelper {
public:
	DateTimeHelper() { Refresh(); }

	void Refresh() {
        // get current time
        _now = std::chrono::system_clock::now();
        _t = std::chrono::system_clock::to_time_t(_now);
        localtime_s(&_tm, &_t);
        
        // calculate milliseconds
        auto duration = _now.time_since_epoch();
        _milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;
	}

	std::string operator()() {
        return std::format("{:d}-{:02d}-{:02d} {:02d}:{:02d}:{:02d}", 
            _tm.tm_year+1900, 
            _tm.tm_mon+1, 
            _tm.tm_mday, 
            _tm.tm_hour, 
            _tm.tm_min, 
            _tm.tm_sec);
	}

	// Get the current year, month, day, hour, minute, and second information
	inline int GetYear()        { return _tm.tm_year+1900;  }
	inline int GetMonth()       { return _tm.tm_mon+1;		}
	inline int GetMonthDay()    { return _tm.tm_mday;		}
	inline int GetHour()        { return _tm.tm_hour;		}
	inline int GetMinute()      { return _tm.tm_min;		}
	inline int GetSecond()      { return _tm.tm_sec;		}
	inline int GetYearDay()     { return _tm.tm_yday;		}
    inline int GetMillisecond() { return _milliseconds;		}
	
protected:
	std::chrono::system_clock::time_point _now;
    time_t _t;
    std::tm _tm;
    int _milliseconds;

    DateTimeHelper(const DateTimeHelper&) = delete;
    DateTimeHelper& operator=(const DateTimeHelper&) = delete;
};



/*-------------------------------------------------- Utility --------------------------------------------------*/
// DLL Resource RAII
struct DLLResourceRAII {
public:
    DLLResourceRAII(const std::wstring& dllPath) {
        _hDLL = LoadLibraryW(dllPath.c_str());
    }

    ~DLLResourceRAII() {
        if (_hDLL) {
            FreeLibrary(_hDLL);
        }
    }

    HMODULE _hDLL;
};


#if _WIN32
struct CriticalSection
{
    CriticalSection() { ::InitializeCriticalSection(&_criticalSection); }
	~CriticalSection() { ::DeleteCriticalSection(&_criticalSection); }

    void Lock() { ::EnterCriticalSection(&_criticalSection); }
	void UnLock() { ::LeaveCriticalSection(&_criticalSection); }

	CRITICAL_SECTION _criticalSection;
};

struct CSGuard {
    CriticalSection& _cs;

    CSGuard(CriticalSection& cs) : _cs(cs) {
        _cs.Lock();
    }

    ~CSGuard() {
        _cs.UnLock();
    }
};

inline bool GetPCComList(std::vector<std::string>& comList) {
	HKEY hKey;
	wchar_t portName[256], comW[256];
	// Open the corresponding key in the serial port registry
	if (ERROR_SUCCESS == ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Hardware\\DeviceMap\\SerialComm"), NULL, KEY_READ, &hKey)) {
		int i = 0;
		DWORD  dwLong, dwSize;
		while (TRUE) {
			dwLong = dwSize = sizeof(portName);
			// Enumeration of serial ports
			if (ERROR_NO_MORE_ITEMS == ::RegEnumValue(hKey, i, portName, &dwLong, NULL, NULL, (PUCHAR)comW, &dwSize)) {
				break;
			}
			comList.push_back(WSTR_TO_STR(comW));
			i++;
		}

		// Closing the registry
		RegCloseKey(hKey);
	}

	std::sort(comList.begin(), comList.end(), [](std::string& s1, std::string& s2){return atoi(s1.substr(3).c_str()) < atoi(s2.substr(3).c_str()) ? true:false;});
	comList.erase(std::unique(comList.begin(), comList.end()), comList.end());

	return true;
}

struct IsKeyDown {
	bool operator()(int key) {
        return GetKeyState(key)&0x80 ? true:false;
    }
};

// Copy from ATGTK
inline void FindMediaFile(
    _Out_writes_(cchDest) wchar_t* strDestPath,
    _In_ int cchDest,
    _In_z_ const wchar_t* strFilename,
    _In_opt_ const wchar_t* const * searchFolders = nullptr)
{
    if (!strFilename || strFilename[0] == 0 || !strDestPath || cchDest < 10)
        throw std::invalid_argument("FindMediaFile");

    // Check CWD for quick out
    wcscpy_s(strDestPath, size_t(cchDest), strFilename);
    if (GetFileAttributesW(strDestPath) != 0xFFFFFFFF)
        return;

    // Search CWD with folder names
    static const wchar_t* s_defSearchFolders[] =
    {
        //L"Assets",
        //L"Assets\\Fonts",
        //L"Assets\\Textures",
        L"Media",
        L"Media\\Textures",
        L"Media\\Fonts",
        L"Media\\Meshes",
        L"Media\\Models",
        //L"Media\\PBR",
        //L"Media\\CubeMaps",
        //L"Media\\HDR",
        //L"Media\\Sounds",
        //L"Media\\Videos",
        0
    };

    if (!searchFolders)
        searchFolders = s_defSearchFolders;

    wchar_t strFullFileName[MAX_PATH] = {};
    for (const wchar_t* const * searchFolder = searchFolders; *searchFolder != 0; ++searchFolder)
    {
        swprintf_s(strFullFileName, MAX_PATH, L"%ls\\%ls", *searchFolder, strFilename);
        if (GetFileAttributesW(strFullFileName) != 0xFFFFFFFF)
        {
            wcscpy_s(strDestPath, size_t(cchDest), strFullFileName);
            return;
        }
    }

    // Get the exe name, and exe path
    wchar_t strExePath[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, strExePath, MAX_PATH);
    strExePath[MAX_PATH - 1] = 0;

    // Search all parent directories starting at .\ and using strFilename as the leaf name
    wchar_t strLeafName[MAX_PATH] = {};
    wcscpy_s(strLeafName, MAX_PATH, strFilename);

    wchar_t strFullPath[MAX_PATH] = {};
    wchar_t strSearch[MAX_PATH] = {};
    wchar_t* strFilePart = nullptr;

    GetFullPathNameW(strExePath, MAX_PATH, strFullPath, &strFilePart);

    while (strFilePart && *strFilePart != '\0')
    {
        swprintf_s(strFullFileName, MAX_PATH, L"%ls\\%ls", strFullPath, strLeafName);
        if (GetFileAttributesW(strFullFileName) != 0xFFFFFFFF)
        {
            wcscpy_s(strDestPath, size_t(cchDest), strFullFileName);
            return;
        }

        for (const wchar_t* const * searchFolder = searchFolders; *searchFolder != 0; ++searchFolder)
        {
            swprintf_s(strFullFileName, MAX_PATH, L"%ls\\%ls\\%ls", strFullPath, *searchFolder, strLeafName);
            if (GetFileAttributesW(strFullFileName) != 0xFFFFFFFF)
            {
                wcscpy_s(strDestPath, size_t(cchDest), strFullFileName);
                return;
            }
        }

        swprintf_s(strSearch, MAX_PATH, L"%ls\\..", strFullPath);
        GetFullPathNameW(strSearch, MAX_PATH, strFullPath, &strFilePart);
    }

    // On failure, return the file as the path but also throw an error
    wcscpy_s(strDestPath, size_t(cchDest), strFilename);

    throw std::exception("File not found");
}




#endif