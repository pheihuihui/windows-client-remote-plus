#include "pch.h"

#include "DShowLib.h"

using namespace DShowLib;

RemoteScreen::RemoteScreen(HWND hwnd) {
    hWindow = hwnd;
    g_pVW = NULL;
    g_pMC = NULL;
    g_pME = NULL;
    g_pGraph = NULL;
    g_pCapture = NULL;
    g_pConfig_streaming = NULL;
    g_pConfig_vmr = NULL;
    g_pWc = NULL;
};

HRESULT RemoteScreen::InitWindowlessVMR(IBaseFilter** ppDestFt, IVMRWindowlessControl9** ppWc) {
    if (!ppWc) {
        return E_POINTER;
    }
    IBaseFilter* pVmr = NULL;
    IVMRWindowlessControl9* pWc = NULL;
    // Create the VMR. 
    HRESULT hr = CoCreateInstance(CLSID_VideoMixingRenderer, NULL, CLSCTX_INPROC, IID_IBaseFilter, (void**)&pVmr);
    if (FAILED(hr)) {
        return hr;
    }

    // Add the VMR to the filter graph.
    //hr = g_pGraph->AddFilter(pVmr, L"Video Mixing Renderer");
    //if (FAILED(hr)) {
    //    pVmr->Release();
    //    return hr;
    //}

    hr = pVmr->QueryInterface(IID_IVMRFilterConfig, (void**)&g_pConfig_vmr);
    if (SUCCEEDED(hr)) {
        hr = g_pConfig_vmr->SetRenderingMode(VMRMode_Windowless);
        g_pConfig_vmr->Release();
    }
    if (SUCCEEDED(hr)) {
        // Set the window. 
        hr = pVmr->QueryInterface(IID_IVMRWindowlessControl, (void**)&pWc);
        if (SUCCEEDED(hr)) {
            hr = pWc->SetVideoClippingWindow(hWindow);
            if (SUCCEEDED(hr)) {
                *ppWc = pWc; // Return this as an AddRef'd pointer. 
            } else {
                // An error occurred, so release the interface.
                pWc->Release();
            }
        }
    }
    *ppDestFt = pVmr;
    pVmr->Release();
    return hr;
}

HRESULT RemoteScreen::GetInterfaces(void) {
    HRESULT hr;
    // Create the filter graph
    // IGraphBuilder* g_pGraph;
    hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC, IID_IGraphBuilder, (void**)&g_pGraph);
    if (FAILED(hr))
        return hr;

    // Create the capture graph builder
    hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC, IID_ICaptureGraphBuilder2, (void**)&g_pCapture);
    if (FAILED(hr))
        return hr;

    // Obtain interfaces for media control and Video Window
    hr = g_pGraph->QueryInterface(IID_IMediaControl, (LPVOID*)&g_pMC);
    if (FAILED(hr))
        return hr;

    hr = g_pGraph->QueryInterface(IID_IVideoWindow, (LPVOID*)&g_pVW);
    if (FAILED(hr))
        return hr;

    hr = g_pGraph->QueryInterface(IID_IMediaEventEx, (LPVOID*)&g_pME);
    if (FAILED(hr))
        return hr;

    //hr = g_pGraph->QueryInterface(IID_IAMStreamConfig, (LPVOID*)&g_pConfig);
    //if (FAILED(hr))
    //    return hr;
    // Set the window handle used to process graph events
    //hr = g_pME->SetNotifyWindow((OAHWND)ghApp, WM_GRAPHNOTIFY, 0);

    return hr;
}

HRESULT RemoteScreen::FindCaptureDevice(IBaseFilter** ppSrcFilter) {
    HRESULT hr = S_OK;
    IBaseFilter* pSrc = NULL;
    IMoniker* pMoniker = NULL;
    ICreateDevEnum* pDevEnum = NULL;
    IEnumMoniker* pClassEnum = NULL;

    if (!ppSrcFilter) {
        return E_POINTER;
    }

    // Create the system device enumerator
    hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC, IID_ICreateDevEnum, (void**)&pDevEnum);
    if (FAILED(hr)) {
        return hr;
    }

    // Create an enumerator for the video capture devices
    if (SUCCEEDED(hr)) {
        hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pClassEnum, 0);
        if (FAILED(hr)) {
            return hr;
        }
    }

    if (SUCCEEDED(hr)) {
        // If there are no enumerators for the requested type, then 
        // CreateClassEnumerator will succeed, but pClassEnum will be NULL.
        if (pClassEnum == NULL) {
            hr = E_FAIL;
        }
    }

    // Use the first video capture device on the device list.
    // Note that if the Next() call succeeds but there are no monikers,
    // it will return S_FALSE (which is not a failure).  Therefore, we
    // check that the return code is S_OK instead of using SUCCEEDED() macro.
    if (SUCCEEDED(hr)) {
        hr = pClassEnum->Next(1, &pMoniker, NULL);
        if (hr == S_FALSE) {
            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr)) {
        // Bind Moniker to a filter object
        hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pSrc);
        if (FAILED(hr)) {
        }
    }

    // Copy the found filter pointer to the output parameter.
    if (SUCCEEDED(hr)) {
        *ppSrcFilter = pSrc;
        (*ppSrcFilter)->AddRef();
    }

    pSrc->Release();
    pMoniker->Release();
    pDevEnum->Release();
    pClassEnum->Release();

    return hr;
}

HRESULT RemoteScreen::CaptureVideo() {
    HRESULT hr;
    IBaseFilter* pSrcFilter = NULL;
    IBaseFilter* pDestFilter = NULL;

    // Get DirectShow interfaces
    hr = GetInterfaces();
    if (FAILED(hr)) {
        return hr;
    }

    // Attach the filter graph to the capture graph
    hr = g_pCapture->SetFiltergraph(g_pGraph);
    if (FAILED(hr)) {
        return hr;
    }

    // Use the system device enumerator and class enumerator to find
    // a video capture/preview device, such as a desktop USB video camera.
    hr = FindCaptureDevice(&pSrcFilter);
    if (FAILED(hr)) {
        // Don't display a message because FindCaptureDevice will handle it
        return hr;
    }

    // Add Capture filter to our graph.
    hr = g_pGraph->AddFilter(pSrcFilter, L"Video Capture");
    if (FAILED(hr)) {
        pSrcFilter->Release();
        return hr;
    }

    hr = g_pCapture->FindInterface(&PIN_CATEGORY_CAPTURE, NULL, pSrcFilter, IID_IAMStreamConfig, (void**)&g_pConfig_streaming);

    int iCount = 0, iSize = 0;

    hr = g_pConfig_streaming->GetNumberOfCapabilities(&iCount, &iSize);

    //if (iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS)) {
    //    // Use the video capabilities structure.

    //    for (int iFormat = 0; iFormat < iCount; iFormat++) {
    //        VIDEO_STREAM_CONFIG_CAPS scc;
    //        AM_MEDIA_TYPE* pmtConfig;
    //        hr = g_pConfig->GetStreamCaps(iFormat, &pmtConfig, (BYTE*)&scc);
    //        if (SUCCEEDED(hr)) {
    //            if (pmtConfig->lSampleSize == 24883200) {
    //                g_pConfig->SetFormat(pmtConfig);
    //            }
    //        }
    //    }
    //}

    // Render the preview pin on the video capture filter
    // Use this instead of g_pGraph->RenderFile
    hr = InitWindowlessVMR(&pDestFilter, &g_pWc);

    hr = g_pGraph->AddFilter(pDestFilter, L"windowless");

    long lWidth, lHeight;
    hr = g_pWc->GetNativeVideoSize(&lWidth, &lHeight, NULL, NULL);

    if (SUCCEEDED(hr)) {
        RECT rcSrc, rcDest;
        // Set the source rectangle.
        SetRect(&rcSrc, 0, 0, lWidth, lHeight);
        // Get the window client area.
        GetClientRect(hWindow, &rcDest);
        // Set the destination rectangle.
        SetRect(&rcDest, 0, 0, rcDest.right, rcDest.bottom);
        // Set the video position.
        hr = g_pWc->SetVideoPosition(&rcSrc, &rcDest);
    }

    if (SUCCEEDED(hr)) {
        hr = g_pCapture->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pSrcFilter, NULL, pDestFilter);
    }
    if (FAILED(hr)) {
        pSrcFilter->Release();
        pDestFilter->Release();
        g_pWc->Release();
        return hr;
    }

    // Now that the filter has been added to the graph and we have
    // rendered its stream, we can release this reference to the filter.
    pSrcFilter->Release();

    // Set video window style and position
    // hr = SetupVideoWindow();
    if (FAILED(hr)) {
        return hr;
    }

#ifdef REGISTER_FILTERGRAPH
    // Add our graph to the running object table, which will allow
    // the GraphEdit application to "spy" on our graph
    hr = AddGraphToRot(g_pGraph, &g_dwGraphRegister);
    if (FAILED(hr)) {
        Msg(TEXT("Failed to register filter graph with ROT!  hr=0x%x"), hr);
        g_dwGraphRegister = 0;
    }
#endif

    // Start previewing video data
    hr = g_pMC->Run();
    if (FAILED(hr)) {
        return hr;
    }

    return S_OK;
}

void RemoteScreen::Start() {
    CaptureVideo();
}

void RefRemoteScreen::PlayAt(void* hwnd) {
    auto scr = RemoteScreen((HWND)hwnd);
    scr.Start();
}