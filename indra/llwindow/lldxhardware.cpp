/** 
 * @file lldxhardware.cpp
 * @brief LLDXHardware implementation
 *
 * $LicenseInfo:firstyear=2001&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2010, Linden Research, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */
#ifdef LL_WINDOWS
#include "linden_common.h"
#define INITGUID
#include <dxdiag.h>
#undef INITGUID
#include <wbemidl.h>
#include <boost/tokenizer.hpp>
#include "lldxhardware.h"
#include "llerror.h"
#include "llstring.h"
#include "llstl.h"
#include "lltimer.h"
void (*gWriteDebug)(const char* msg) = NULL;
LLDXHardware gDXHardware;
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
typedef BOOL ( WINAPI* PfnCoSetProxyBlanket )( IUnknown* pProxy, DWORD dwAuthnSvc, DWORD dwAuthzSvc,
											   OLECHAR* pServerPrincName, DWORD dwAuthnLevel, DWORD dwImpLevel,
											   RPC_AUTH_IDENTITY_HANDLE pAuthInfo, DWORD dwCapabilities );
HRESULT GetVideoMemoryViaWMI( WCHAR* strInputDeviceID, DWORD* pdwAdapterRam )
{
	HRESULT hr;
	bool bGotMemory = false;
	HRESULT hrCoInitialize = S_OK;
	IWbemLocator* pIWbemLocator = nullptr;
	IWbemServices* pIWbemServices = nullptr;
	BSTR pNamespace = nullptr;
	*pdwAdapterRam = 0;
	hrCoInitialize = CoInitialize( 0 );
	hr = CoCreateInstance( CLSID_WbemLocator,
						   nullptr,
						   CLSCTX_INPROC_SERVER,
						   IID_IWbemLocator,
						   ( LPVOID* )&pIWbemLocator );
#ifdef PRINTF_DEBUGGING
	if( FAILED( hr ) ) wprintf( L"WMI: CoCreateInstance failed: 0x%0.8x\n", hr );
#endif
	if( SUCCEEDED( hr ) && pIWbemLocator )
	{
		pNamespace = SysAllocString( L"\\\\.\\root\\cimv2" );
		hr = pIWbemLocator->ConnectServer( pNamespace, nullptr, nullptr, 0L,
										   0L, nullptr, nullptr, &pIWbemServices );
#ifdef PRINTF_DEBUGGING
		if( FAILED( hr ) ) wprintf( L"WMI: pIWbemLocator->ConnectServer failed: 0x%0.8x\n", hr );
#endif
		if( SUCCEEDED( hr ) && pIWbemServices != 0 )
		{
			HINSTANCE hinstOle32 = nullptr;
			hinstOle32 = LoadLibraryW( L"ole32.dll" );
			if( hinstOle32 )
			{
				PfnCoSetProxyBlanket pfnCoSetProxyBlanket = nullptr;
				pfnCoSetProxyBlanket = ( PfnCoSetProxyBlanket )GetProcAddress( hinstOle32, "CoSetProxyBlanket" );
				if( pfnCoSetProxyBlanket != 0 )
				{
					pfnCoSetProxyBlanket( pIWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
										  RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, 0 );
				}
				FreeLibrary( hinstOle32 );
			}
			IEnumWbemClassObject* pEnumVideoControllers = nullptr;
			BSTR pClassName = nullptr;
			pClassName = SysAllocString( L"Win32_VideoController" );
			hr = pIWbemServices->CreateInstanceEnum( pClassName, 0,
													 nullptr, &pEnumVideoControllers );
#ifdef PRINTF_DEBUGGING
			if( FAILED( hr ) ) wprintf( L"WMI: pIWbemServices->CreateInstanceEnum failed: 0x%0.8x\n", hr );
#endif
			if( SUCCEEDED( hr ) && pEnumVideoControllers )
			{
				IWbemClassObject* pVideoControllers[10] = {0};
				DWORD uReturned = 0;
				BSTR pPropName = nullptr;
				pEnumVideoControllers->Reset();
				hr = pEnumVideoControllers->Next( 5000,
												  10,
												  pVideoControllers,
												  &uReturned );
#ifdef PRINTF_DEBUGGING
				if( FAILED( hr ) ) wprintf( L"WMI: pEnumVideoControllers->Next failed: 0x%0.8x\n", hr );
				if( uReturned == 0 ) wprintf( L"WMI: pEnumVideoControllers uReturned == 0\n" );
#endif
				VARIANT var;
				if( SUCCEEDED( hr ) )
				{
					bool bFound = false;
					for( UINT iController = 0; iController < uReturned; iController++ )
					{
						if ( !pVideoControllers[iController] )
							continue;
						pPropName = SysAllocString( L"PNPDeviceID" );
						hr = pVideoControllers[iController]->Get( pPropName, 0L, &var, nullptr, nullptr );
#ifdef PRINTF_DEBUGGING
						if( FAILED( hr ) )
							wprintf( L"WMI: pVideoControllers[iController]->Get PNPDeviceID failed: 0x%0.8x\n", hr );
#endif
						if( SUCCEEDED( hr ) )
						{
							if( wcsstr( var.bstrVal, strInputDeviceID ) != 0 )
								bFound = true;
						}
						VariantClear( &var );
						if( pPropName ) SysFreeString( pPropName );
						if( bFound )
						{
							pPropName = SysAllocString( L"AdapterRAM" );
							hr = pVideoControllers[iController]->Get( pPropName, 0L, &var, nullptr, nullptr );
#ifdef PRINTF_DEBUGGING
							if( FAILED( hr ) )
								wprintf( L"WMI: pVideoControllers[iController]->Get AdapterRAM failed: 0x%0.8x\n",
										 hr );
#endif
							if( SUCCEEDED( hr ) )
							{
								bGotMemory = true;
								*pdwAdapterRam = var.ulVal;
							}
							VariantClear( &var );
							if( pPropName ) SysFreeString( pPropName );
							break;
						}
						SAFE_RELEASE( pVideoControllers[iController] );
					}
				}
			}
			if( pClassName )
				SysFreeString( pClassName );
            SAFE_RELEASE( pEnumVideoControllers );
		}
		if( pNamespace )
			SysFreeString( pNamespace );
		SAFE_RELEASE( pIWbemServices );
	}
	SAFE_RELEASE( pIWbemLocator );
	if( SUCCEEDED( hrCoInitialize ) )
		CoUninitialize();
	if( bGotMemory )
		return S_OK;
	else
		return E_FAIL;
}
void get_wstring(IDxDiagContainer* containerp, const WCHAR* wszPropName, WCHAR* wszPropValue, int outputSize)
{
	HRESULT hr;
	VARIANT var;
	VariantInit( &var );
	hr = containerp->GetProp(wszPropName, &var );
	if( SUCCEEDED(hr) )
	{
		switch( var.vt )
		{
			case VT_UI4:
				swprintf(wszPropValue, outputSize, L"%d", var.ulVal);
				break;
			case VT_I4:
				swprintf(wszPropValue, outputSize, L"%d", var.lVal);
				break;
			case VT_BOOL:
				wcscpy( wszPropValue, (var.boolVal) ? L"true" : L"false" );
				break;
			case VT_BSTR:
				wcsncpy( wszPropValue, var.bstrVal, outputSize-1 );
				wszPropValue[outputSize-1] = 0;
				break;
		}
	}
	VariantClear( &var );
}
std::string get_string(IDxDiagContainer *containerp, const WCHAR *wszPropName)
{
	WCHAR wszPropValue[256];
	get_wstring(containerp, wszPropName, wszPropValue, 256);
	return utf16str_to_utf8str(wszPropValue);
}
LLVersion::LLVersion()
{
	mValid = FALSE;
	S32 i;
	for (i = 0; i < 4; i++)
	{
		mFields[i] = 0;
	}
}
BOOL LLVersion::set(const std::string &version_string)
{
	S32 i;
	for (i = 0; i < 4; i++)
	{
		mFields[i] = 0;
	}
	std::string str(version_string);
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
	boost::char_separator<char> sep(".", "", boost::keep_empty_tokens);
	tokenizer tokens(str, sep);
	tokenizer::iterator iter = tokens.begin();
	S32 count = 0;
	for (;(iter != tokens.end()) && (count < 4);++iter)
	{
		mFields[count] = atoi(iter->c_str());
		count++;
	}
	if (count < 4)
	{
		for (i = 0; i < 4; i++)
		{
			mFields[i] = 0;
		}
		mValid = FALSE;
	}
	else
	{
		mValid = TRUE;
	}
	return mValid;
}
S32 LLVersion::getField(const S32 field_num)
{
	if (!mValid)
	{
		return -1;
	}
	else
	{
		return mFields[field_num];
	}
}
std::string LLDXDriverFile::dump()
{
	if (gWriteDebug)
	{
		gWriteDebug("Filename:");
		gWriteDebug(mName.c_str());
		gWriteDebug("\n");
		gWriteDebug("Ver:");
		gWriteDebug(mVersionString.c_str());
		gWriteDebug("\n");
		gWriteDebug("Date:");
		gWriteDebug(mDateString.c_str());
		gWriteDebug("\n");
	}
	LL_INFOS() << mFilepath << LL_ENDL;
	LL_INFOS() << mName << LL_ENDL;
	LL_INFOS() << mVersionString << LL_ENDL;
	LL_INFOS() << mDateString << LL_ENDL;
	return "";
}
LLDXDevice::~LLDXDevice()
{
	for_each(mDriverFiles.begin(), mDriverFiles.end(), DeletePairedPointer());
	mDriverFiles.clear();
}
std::string LLDXDevice::dump()
{
	if (gWriteDebug)
	{
		gWriteDebug("StartDevice\n");
		gWriteDebug("DeviceName:");
		gWriteDebug(mName.c_str());
		gWriteDebug("\n");
		gWriteDebug("PCIString:");
		gWriteDebug(mPCIString.c_str());
		gWriteDebug("\n");
	}
	LL_INFOS() << LL_ENDL;
	LL_INFOS() << "DeviceName:" << mName << LL_ENDL;
	LL_INFOS() << "PCIString:" << mPCIString << LL_ENDL;
	LL_INFOS() << "Drivers" << LL_ENDL;
	LL_INFOS() << "-------" << LL_ENDL;
	for (driver_file_map_t::iterator iter = mDriverFiles.begin(),
			 end = mDriverFiles.end();
		 iter != end; iter++)
	{
		LLDXDriverFile *filep = iter->second;
		filep->dump();
	}
	if (gWriteDebug)
	{
		gWriteDebug("EndDevice\n");
	}
	return "";
}
LLDXDriverFile *LLDXDevice::findDriver(const std::string &driver)
{
	for (driver_file_map_t::iterator iter = mDriverFiles.begin(),
			 end = mDriverFiles.end();
		 iter != end; iter++)
	{
		LLDXDriverFile *filep = iter->second;
		if (!utf8str_compare_insensitive(filep->mName,driver))
		{
			return filep;
		}
	}
	return NULL;
}
LLDXHardware::LLDXHardware()
{
	mVRAM = 0;
	gWriteDebug = NULL;
}
void LLDXHardware::cleanup()
{
}
BOOL LLDXHardware::getInfo(BOOL vram_only, S32Megabytes system_ram)
{
	LLTimer hw_timer;
	BOOL ok = FALSE;
    HRESULT       hr;
    hr = CoInitialize(NULL);
	if (FAILED(hr))
	{
		LL_WARNS() << "COM initialization failure!" << LL_ENDL;
		gWriteDebug("COM initialization failure!\n");
		return ok;
	}
    IDxDiagProvider *dx_diag_providerp = NULL;
    IDxDiagContainer *dx_diag_rootp = NULL;
	IDxDiagContainer *devices_containerp = NULL;
	IDxDiagContainer *device_containerp = NULL;
	IDxDiagContainer *file_containerp = NULL;
	IDxDiagContainer *driver_containerp = NULL;
	LL_DEBUGS("AppInit") << "CoCreateInstance IID_IDxDiagProvider" << LL_ENDL;
    hr = CoCreateInstance(CLSID_DxDiagProvider,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IDxDiagProvider,
                          (LPVOID*) &dx_diag_providerp);
	if (FAILED(hr))
	{
		LL_WARNS("AppInit") << "No DXDiag provider found!  DirectX 9 not installed!" << LL_ENDL;
		gWriteDebug("No DXDiag provider found!  DirectX 9 not installed!\n");
		goto LCleanup;
	}
	if (SUCCEEDED(hr))
	{
		DXDIAG_INIT_PARAMS dx_diag_init_params;
		ZeroMemory(&dx_diag_init_params, sizeof(DXDIAG_INIT_PARAMS));
		dx_diag_init_params.dwSize = sizeof(DXDIAG_INIT_PARAMS);
		dx_diag_init_params.dwDxDiagHeaderVersion = DXDIAG_DX9_SDK_VERSION;
		dx_diag_init_params.bAllowWHQLChecks = TRUE;
		dx_diag_init_params.pReserved = NULL;
		LL_DEBUGS("AppInit") << "dx_diag_providerp->Initialize" << LL_ENDL;
		hr = dx_diag_providerp->Initialize(&dx_diag_init_params);
		if (FAILED(hr))
		{
			goto LCleanup;
		}
		LL_DEBUGS("AppInit") << "dx_diag_providerp->GetRootContainer" << LL_ENDL;
		hr = dx_diag_providerp->GetRootContainer(&dx_diag_rootp);
		if (FAILED(hr) || !dx_diag_rootp)
		{
			goto LCleanup;
		}
		LL_DEBUGS("AppInit") << "dx_diag_rootp->GetChildContainer" << LL_ENDL;
		hr = dx_diag_rootp->GetChildContainer(L"DxDiag_DisplayDevices", &devices_containerp);
		if (FAILED(hr) || !devices_containerp)
		{
			goto LCleanup;
		}
		DWORD device_count;
		devices_containerp->GetNumberOfChildContainers(&device_count);
		LL_DEBUGS("AppInit") << "devices_containerp->GetChildContainer" << LL_ENDL;
		S32 vram_max = -1;
		std::string gpu_string_max;
		for (DWORD i = 0; i < device_count; ++i)
		{
			std::wstring str = L"";
			str += std::to_wstring(i);
			hr = devices_containerp->GetChildContainer(str.data(), &device_containerp);
			if (FAILED(hr) || !device_containerp)
			{
				continue;
			}
			DWORD vram = 0;
			S32 detected_ram;
			std::string ram_str;
			WCHAR deviceID[512];
			get_wstring(device_containerp, L"szDeviceID", deviceID, 512);
			if (SUCCEEDED(GetVideoMemoryViaWMI(deviceID, &vram)))
			{
				detected_ram = vram / (1024 * 1024);
				LL_INFOS("AppInit") << "VRAM  for device[" << i << "]: " << detected_ram << " WMI" << LL_ENDL;
			}
			else
			{
				ram_str = get_string(device_containerp, L"szDisplayMemoryEnglish");
				SAFE_RELEASE(device_containerp);
				char* stopstring;
				detected_ram = strtol(ram_str.c_str(), &stopstring, 10);
				detected_ram = llmax(0, detected_ram - ((S32)(system_ram / (2 * device_count)) + 1));
				LL_INFOS("AppInit") << "VRAM for device[" << i << "]: " << detected_ram << " DX9 string: " << ram_str << LL_ENDL;
			}
			if (detected_ram > vram_max)
			{
				gpu_string_max = ram_str;
				vram_max = detected_ram;
			}
		}
		if (vram_max <= 0)
		{
			gpu_string_max = "<No sutable gpu device>";
			LL_INFOS("AppInit") << "No dedicated VRAM. Using system memory instead." << LL_ENDL;
			vram_max = (S32)system_ram / 2;
		}
		LL_INFOS("AppInit") << "VRAM Detected: " << vram_max << " DX9 string: " << gpu_string_max << LL_ENDL;
		if (vram_max == -1)
		{
			goto LCleanup;
		}
		mVRAM = vram_max;
		if (vram_only)
		{
			ok = TRUE;
			goto LCleanup;
		}
	}
    ok = TRUE;
LCleanup:
	if (!ok)
	{
		LL_WARNS("AppInit") << "DX9 probe failed" << LL_ENDL;
		gWriteDebug("DX9 probe failed\n");
	}
	SAFE_RELEASE(file_containerp);
	SAFE_RELEASE(driver_containerp);
	SAFE_RELEASE(device_containerp);
	SAFE_RELEASE(devices_containerp);
    SAFE_RELEASE(dx_diag_rootp);
    SAFE_RELEASE(dx_diag_providerp);
    CoUninitialize();
    return ok;
    }
LLSD LLDXHardware::getDisplayInfo()
{
	LLTimer hw_timer;
    HRESULT       hr;
	LLSD ret;
    hr = CoInitialize(NULL);
	if (FAILED(hr))
	{
		LL_WARNS() << "COM initialization failure!" << LL_ENDL;
		gWriteDebug("COM initialization failure!\n");
		return ret;
	}
    IDxDiagProvider *dx_diag_providerp = NULL;
    IDxDiagContainer *dx_diag_rootp = NULL;
	IDxDiagContainer *devices_containerp = NULL;
	IDxDiagContainer *device_containerp = NULL;
	IDxDiagContainer *file_containerp = NULL;
	IDxDiagContainer *driver_containerp = NULL;
	LL_INFOS() << "CoCreateInstance IID_IDxDiagProvider" << LL_ENDL;
    hr = CoCreateInstance(CLSID_DxDiagProvider,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_IDxDiagProvider,
                          (LPVOID*) &dx_diag_providerp);
	if (FAILED(hr))
	{
		LL_WARNS() << "No DXDiag provider found!  DirectX 9 not installed!" << LL_ENDL;
		gWriteDebug("No DXDiag provider found!  DirectX 9 not installed!\n");
		goto LCleanup;
	}
    if (SUCCEEDED(hr))
    {
        DXDIAG_INIT_PARAMS dx_diag_init_params;
        ZeroMemory(&dx_diag_init_params, sizeof(DXDIAG_INIT_PARAMS));
        dx_diag_init_params.dwSize                  = sizeof(DXDIAG_INIT_PARAMS);
        dx_diag_init_params.dwDxDiagHeaderVersion   = DXDIAG_DX9_SDK_VERSION;
        dx_diag_init_params.bAllowWHQLChecks        = TRUE;
        dx_diag_init_params.pReserved               = NULL;
		LL_INFOS() << "dx_diag_providerp->Initialize" << LL_ENDL;
        hr = dx_diag_providerp->Initialize(&dx_diag_init_params);
        if(FAILED(hr))
		{
            goto LCleanup;
		}
		LL_INFOS() << "dx_diag_providerp->GetRootContainer" << LL_ENDL;
        hr = dx_diag_providerp->GetRootContainer( &dx_diag_rootp );
        if(FAILED(hr) || !dx_diag_rootp)
		{
            goto LCleanup;
		}
		LL_INFOS() << "dx_diag_rootp->GetChildContainer" << LL_ENDL;
		hr = dx_diag_rootp->GetChildContainer(L"DxDiag_DisplayDevices", &devices_containerp);
		if(FAILED(hr) || !devices_containerp)
		{
            goto LCleanup;
		}
		LL_INFOS() << "devices_containerp->GetChildContainer" << LL_ENDL;
		hr = devices_containerp->GetChildContainer(L"0", &device_containerp);
		if(FAILED(hr) || !device_containerp)
		{
            goto LCleanup;
		}
		std::string ram_str = get_string(device_containerp, L"szDisplayMemoryEnglish");
		char *stopstring;
		ret["VRAM"] = strtol(ram_str.c_str(), &stopstring, 10);
		std::string device_name = get_string(device_containerp, L"szDescription");
		ret["DeviceName"] = device_name;
		std::string device_driver=  get_string(device_containerp, L"szDriverVersion");
		ret["DriverVersion"] = device_driver;
        if(device_name.length() >= 4 && device_name.substr(0,4) == "ATI ")
        {
            HKEY hKey;
            const DWORD RV_SIZE = 100;
            WCHAR release_version[RV_SIZE];
            if(ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\ATI Technologies\\CBT"), &hKey))
            {
                DWORD dwType = REG_SZ;
                DWORD dwSize = sizeof(WCHAR) * RV_SIZE;
                if(ERROR_SUCCESS == RegQueryValueEx(hKey, TEXT("ReleaseVersion"),
                    NULL, &dwType, (LPBYTE)release_version, &dwSize))
                {
                    release_version[RV_SIZE - 1] = NULL;
                    ret["DriverVersion"] = utf16str_to_utf8str(release_version);
                }
                RegCloseKey(hKey);
            }
        }
    }
LCleanup:
	SAFE_RELEASE(file_containerp);
	SAFE_RELEASE(driver_containerp);
	SAFE_RELEASE(device_containerp);
	SAFE_RELEASE(devices_containerp);
    SAFE_RELEASE(dx_diag_rootp);
    SAFE_RELEASE(dx_diag_providerp);
    CoUninitialize();
	return ret;
}
void LLDXHardware::setWriteDebugFunc(void (*func)(const char*))
{
	gWriteDebug = func;
}
#endif
