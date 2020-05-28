#pragma once

#include "pch.h"
#include <windows.h>
#include <dshow.h>
#include <d3d9.h>
#include <vmr9.h>

namespace DShowLib {
	public class RemoteScreen {
	private:
		IVideoWindow* g_pVW;
		IMediaControl* g_pMC;
		IMediaEventEx* g_pME;
		IGraphBuilder* g_pGraph;
		ICaptureGraphBuilder2* g_pCapture;
		IAMStreamConfig* g_pConfig_streaming;
		IVMRFilterConfig9* g_pConfig_vmr;
		IVMRWindowlessControl9* g_pWc;
		HWND hWindow;

		HRESULT GetInterfaces(void);
		HRESULT FindCaptureDevice(IBaseFilter**);
		HRESULT CaptureVideo();
		HRESULT InitWindowlessVMR(IBaseFilter**, IVMRWindowlessControl9**);
	public:
		RemoteScreen(HWND);
		void Start();
	};

	public ref class RefRemoteScreen {
	public:
		static void PlayAt(void*);
	};
}
