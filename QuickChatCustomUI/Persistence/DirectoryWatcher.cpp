#include "pch.h"
#include "DirectoryWatcher.h"
#include <windows.h>
#include <thread>
#include <atomic>
#include <chrono>

namespace DirectoryWatcher
{
    // ==================== State ====================

    static std::thread watcherThread;
    static std::atomic<bool> isRunning{false};
    static std::atomic<bool> changeDetected{false};
    static std::atomic<int> ignoreCount{0};
    static std::chrono::steady_clock::time_point lastChangeTime;
    static constexpr int DEBOUNCE_MS = 500;
    static HANDLE stopEvent = nullptr;
    static std::function<void()> changeCallback;
    // ==================== Watcher Thread ====================

    static void WatchDirectory(const std::string& path)
    {
        HANDLE directoryHandle = CreateFileA(
            path.c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
            nullptr
        );
        
        if (directoryHandle == INVALID_HANDLE_VALUE)
        {
            LOG("[DirWatcher] Failed to open directory: " + path);
            isRunning = false;
            return;
        }
        
        char buffer[4096];
        OVERLAPPED overlapped = {};
        overlapped.hEvent = CreateEventA(nullptr, TRUE, FALSE, nullptr);
        
        while (isRunning)
        {
            ResetEvent(overlapped.hEvent);
            
            DWORD bytesReturned;
            BOOL success = ReadDirectoryChangesW(
                directoryHandle,
                buffer,
                sizeof(buffer),
                TRUE,
                FILE_NOTIFY_CHANGE_FILE_NAME | 
                FILE_NOTIFY_CHANGE_DIR_NAME | 
                FILE_NOTIFY_CHANGE_LAST_WRITE | 
                FILE_NOTIFY_CHANGE_CREATION,
                &bytesReturned,
                &overlapped,
                nullptr
            );
            
            if (!success)
            {
                break;
            }
            
            HANDLE handles[] = {overlapped.hEvent, stopEvent};
            DWORD waitResult = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
            
            if (waitResult == WAIT_OBJECT_0)
            {
                if (GetOverlappedResult(directoryHandle, &overlapped, &bytesReturned, FALSE))
                {
                    changeDetected = true;
                }
            }
            else if (waitResult == WAIT_OBJECT_0 + 1)
            {
                break;
            }
            else
            {
                break;
            }
        }
        
        CloseHandle(overlapped.hEvent);
        CloseHandle(directoryHandle);
        
        LOG("[DirWatcher] Watcher thread exiting");
    }
    // ==================== Public API ====================

    void Start(const std::string& path, std::function<void()> callback)
    {
        if (isRunning)
        {
            return;
        }
        
        changeCallback = callback;
        changeDetected = false;
        ignoreCount = 0;
        lastChangeTime = std::chrono::steady_clock::now() - std::chrono::milliseconds(DEBOUNCE_MS * 2);
        isRunning = true;
        
        if (stopEvent)
        {
            CloseHandle(stopEvent);
        }
        stopEvent = CreateEventA(nullptr, TRUE, FALSE, nullptr);
        
        watcherThread = std::thread(WatchDirectory, path);
        
        LOG("[DirWatcher] Started watching: " + path);
    }
    
    void Stop()
    {
        if (!isRunning)
        {
            return;
        }
        
        isRunning = false;
        
        if (stopEvent)
        {
            SetEvent(stopEvent);
        }
        
        if (watcherThread.joinable())
        {
            watcherThread.join();
        }
        
        if (stopEvent)
        {
            CloseHandle(stopEvent);
            stopEvent = nullptr;
        }
        
        LOG("[DirWatcher] Stopped");
    }
    
    bool IsRunning()
    {
        if (changeDetected.exchange(false))
        {
            if (ignoreCount > 0)
            {
                ignoreCount--;
                return isRunning;
            }
            
            // Debounce
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastChangeTime).count();
            
            if (elapsed < DEBOUNCE_MS)
            {
                return isRunning;
            }
            
            lastChangeTime = now;
            if (changeCallback)
            {
                changeCallback();
            }
        }
        
        return isRunning;
    }
    
    void IgnoreNextChange()
    {
        ignoreCount++;
    }
}
