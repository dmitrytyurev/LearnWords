#include "stdafx.h"
#include "SoundClip.h"

//===============================================================================================
// 
//===============================================================================================

struct handle_data
{
	unsigned long process_id;
	HWND best_handle;
};

//===============================================================================================
// 
//===============================================================================================

BOOL is_main_window(HWND handle)
{
	return GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);
}

//===============================================================================================
// 
//===============================================================================================

BOOL CALLBACK enum_windows_callback(HWND handle, LPARAM lParam)
{
	handle_data& data = *(handle_data*)lParam;
	unsigned long process_id = 0;
	GetWindowThreadProcessId(handle, &process_id);
	if (data.process_id != process_id || !is_main_window(handle)) {
		return TRUE;
	}
	data.best_handle = handle;
	return FALSE;
}

//===============================================================================================
// 
//===============================================================================================

HWND find_main_window(unsigned long process_id)
{
	handle_data data;
	data.process_id = process_id;
	data.best_handle = 0;
	EnumWindows(enum_windows_callback, (LPARAM)&data);
	return data.best_handle;
}

//===============================================================================================
// 
//===============================================================================================

SoundClip::SoundClip(const std::string& fileNameWithPath, int startSec, int stopSec)
{
	hdl = (HANDLE)_spawnl(_P_NOWAIT, 
		"C:\\Program Files\\VideoLAN\\VLC\\vlc.exe", 
		"vlc.exe", 
		fileNameWithPath.c_str(), 
		"--start-time", 
		std::to_string(startSec).c_str(), 
		"--stop-time", 
		std::to_string(stopSec).c_str(),
		"--play-and-exit", 
		"", 
		NULL);
}

//===============================================================================================
// 
//===============================================================================================

SoundClip::~SoundClip()
{
	if (int(hdl) == -1)
		return;

	DWORD pid = GetProcessId(hdl);
	if (pid == 0)
		return;

	HWND hWnd = find_main_window(pid);
	if (!hWnd)
		return;

	::SendMessage(hWnd, WM_CLOSE, NULL, NULL);
	CloseHandle(hWnd);
}

