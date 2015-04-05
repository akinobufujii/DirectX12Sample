// インクルード
#include <tchar.h>
#include <Windows.h>

#include <memory>

#include <d3d12.h>
#include <dxgi1_2.h>
#include <Dxgi1_3.h>
#include <D3d12SDKLayers.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "D3DCompiler.lib")

// 定義
// 入力レイアウト
const D3D12_INPUT_ELEMENT_DESC INPUT_LAYOUT[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_PER_VERTEX_DATA, 0 }
};

// 定数
static const int SCREEN_WIDTH = 1280;
static const int SCREEN_HEIGHT = 720;
static const LPTSTR	CLASS_NAME = TEXT("DirectX");

enum DESCRIPTOR_HEAP_TYPE 
{
	DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,	// UAVなど用
	DESCRIPTOR_HEAP_TYPE_SAMPLER,		// サンプラ用
	DESCRIPTOR_HEAP_TYPE_RTV,			// レンダーターゲット用	
	DESCRIPTOR_HEAP_TYPE_DSV,			// デプスステンシル用	
	DESCRIPTOR_HEAP_TYPE_MAX,
	DESCRIPTOR_HEAP_TYPE_SET = DESCRIPTOR_HEAP_TYPE_SAMPLER + 1,
};

// グローバル変数
ID3D12Device*				g_pDevice;											// デバイス
ID3D12CommandAllocator*		g_pCommandAllocator;								// コマンドアロケータ
ID3D12CommandQueue*			g_pCommandQueue;									// コマンドキュー
IDXGIDevice2*				g_pGIDevice;										// GIデバイス
IDXGIAdapter*				g_pGIAdapter;										// GIアダプター
IDXGIFactory2*				g_pGIFactory;										// GIファクトリー
IDXGISwapChain*				g_pGISwapChain;										// GIスワップチェーン
ID3D12PipelineState*		g_pPipelineState;									// パイプラインステート
ID3D12DescriptorHeap*		g_pDescripterHeapArray[DESCRIPTOR_HEAP_TYPE_MAX];	// ディスクリプタヒープ配列
ID3D12GraphicsCommandList*	g_pGraphicsCommandList;								// 描画コマンドリスト

ID3D12Resource*				g_pBackBufferResource;								// バックバッファのリソース
D3D12_CPU_DESCRIPTOR_HANDLE g_hBackBufferRTV;									// バックバッファレンダーターゲットビュー
ID3D12Resource*				g_pDepthStencilResource;							// デプスステンシルのリソース
D3D12_CPU_DESCRIPTOR_HANDLE g_hDepthStencilView;								// デプスステンシルビュー

LPD3DBLOB					g_pVSBlob;											// 頂点シェーダブロブ
LPD3DBLOB					g_pPSBlob;											// ピクセルシェーダブロブ

// エラーメッセージ
// エラーが起こったらtrueを返すようにする
bool showErrorMessage(HRESULT hr, const LPTSTR text)
{
	if (FAILED(hr)) 
	{
		MessageBox(nullptr, text, TEXT("Error"), MB_OK);
		return true;
	}

	return false;
}

// 安全な開放
template<typename T>
void safeRelease(T*& object)
{
	if (object)
	{
		object->Release();
		object = nullptr;
	}
}

// シェーダコンパイル
bool compileShaderFlomFile(LPCWSTR pFileName, LPCSTR pEntrypoint, LPCSTR pTarget, ID3DBlob** ppblob)
{
	// シェーダコンパイル
	HRESULT hr;
	LPD3DBLOB pError = nullptr;
	hr = D3DCompileFromFile(
		pFileName, 
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		pEntrypoint, 
		pTarget,
		0,
		0,
		ppblob,
		&pError);

	if (FAILED(hr))
	{
		if (pError && pError->GetBufferPointer())
		{
			// エラーコード表示
#ifdef _UNICODE
			// Unicodeの時はUnicode文字に変換する
			int bufferSize = static_cast<int>(pError->GetBufferSize());
			int strLen = MultiByteToWideChar(CP_ACP, 0, reinterpret_cast<CHAR*>(pError->GetBufferPointer()), bufferSize, nullptr, 0);

			std::shared_ptr<wchar_t> errorStr(new wchar_t[strLen]);
			MultiByteToWideChar(CP_ACP, 0, reinterpret_cast<char*>(pError->GetBufferPointer()), bufferSize, errorStr.get(), strLen);

			OutputDebugString(errorStr.get());
#else
			OutputDebugString(reinterpret_cast<LPSTR>(lperror->GetBufferPointer()));
#endif
		}

		safeRelease(pError);

		// コンパイル失敗
		return false;
	}

	// コンパイル成功
	return true;
}

// シェーダ読み込み
void loadShader()
{	
	// 頂点シェーダコンパイル
	if (compileShaderFlomFile(L"VertexShader.hlsl", "main", "vs_5_0", &g_pVSBlob) == false) 
	{
		showErrorMessage(E_FAIL, TEXT("頂点シェーダコンパイル失敗"));
	}

	// ピクセルシェーダコンパイル
	if (compileShaderFlomFile(L"PixelShader.hlsl", "main", "ps_5_0", &g_pPSBlob) == false)
	{
		showErrorMessage(E_FAIL, TEXT("頂点シェーダコンパイル失敗"));
	}
	
	UINT numElements = ARRAYSIZE(INPUT_LAYOUT);


#if 0
	// 空のルートシグニチャ作成
	ID3D12RootSignature* mRootSignature;
	ID3DBlob* pOutBlob, pErrorBlob;
	D3D12_ROOT_SIGNATURE descRootSignature = D3D12_ROOT_SIGNATURE();
	descRootSignature.Flags = D3D12_ROOT_SIGNATURE_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	ThrowIfFailed(D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_V1, pOutBlob.GetAddressOf(), pErrorBlob.GetAddressOf()));
	ThrowIfFailed(mDevice->CreateRootSignature(pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)mRootSignature.GetAddressOf()));

	// create a PSO description
	D3D12_GRAPHICS_PIPELINE_STATE_DESC descPso;
	ZeroMemory(&descPso, sizeof(descPso));
	descPso.InputLayout = { layout, numElements };
	descPso.pRootSignature = mRootSignature.Get();
	descPso.VS = { reinterpret_cast<BYTE*>(blobShaderVert->GetBufferPointer()), blobShaderVert->GetBufferSize() };
	descPso.PS = { reinterpret_cast<BYTE*>(blobShaderPixel->GetBufferPointer()), blobShaderPixel->GetBufferSize() };
	descPso.RasterizerState = CD3D12_RASTERIZER_DESC(D3D12_DEFAULT);
	descPso.BlendState = CD3D12_BLEND_DESC(D3D12_DEFAULT);
	descPso.DepthStencilState.DepthEnable = FALSE;
	descPso.DepthStencilState.StencilEnable = FALSE;
	descPso.SampleMask = UINT_MAX;
	descPso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	descPso.NumRenderTargets = 1;
	descPso.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	descPso.SampleDesc.Count = 1;

	// create the actual PSO
	ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&descPso, mPSO.GetAddressOf()));

	// create descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC descHeap = {};
	descHeap.NumDescriptors = 1;
	descHeap.Type = D3D12_RTV_DESCRIPTOR_HEAP;
	descHeap.Flags = D3D12_DESCRIPTOR_HEAP_NONE;
	ThrowIfFailed(mDevice->CreateDescriptorHeap(&descHeap, __uuidof(ID3D12DescriptorHeap), (void**)mDescriptorHeap.GetAddressOf()));

	// create command list
	ThrowIfFailed(mDevice->CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator.Get(), mPSO.Get(), IID_PPV_ARGS(mCommandList.GetAddressOf())));

	// create backbuffer/rendertarget
	ThrowIfFailed(mSwapChain->GetBuffer(0, __uuidof(ID3D12Resource), (LPVOID*)mRenderTarget.GetAddressOf()));
	mDevice->CreateRenderTargetView(mRenderTarget.Get(), nullptr, mDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// set the viewport
	mViewPort =
	{
		0.0f,
		0.0f,
		static_cast<float>(mWidth),
		static_cast<float>(mHeight),
		0.0f,
		1.0f
	};

	// create scissor rectangle
	mRectScissor = { 0, 0, mWidth, mHeight };

	// create geometry for a triangle
	VERTEX triangleVerts[] =
	{
		{ 0.0f, 0.5f, 0.0f,{ 1.0f, 0.0f, 0.0f, 1.0f } },
		{ 0.45f, -0.5, 0.0f,{ 0.0f, 1.0f, 0.0f, 1.0f } },
		{ -0.45f, -0.5f, 0.0f,{ 0.0f, 0.0f, 1.0f, 1.0f } }
	};

	// actually create the vert buffer
	// Note: using upload heaps to transfer static data like vert buffers is not recommended.
	// Every time the GPU needs it, the upload heap will be marshalled over.  Please read up on Default Heap usage.
	// An upload heap is used here for code simplicity and because there are very few verts to actually transfer
	ThrowIfFailed(mDevice->CreateCommittedResource(
		&CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_MISC_NONE,
		&CD3D12_RESOURCE_DESC::Buffer(3 * sizeof(VERTEX)),
		D3D12_RESOURCE_USAGE_GENERIC_READ,
		nullptr,    // Clear value
		IID_PPV_ARGS(mBufVerts.GetAddressOf())));

	// copy the triangle data to the vertex buffer
	UINT8* dataBegin;
	mBufVerts->Map(0, nullptr, reinterpret_cast<void**>(&dataBegin));
	memcpy(dataBegin, triangleVerts, sizeof(triangleVerts));
	mBufVerts->Unmap(0, nullptr);

	// create vertex buffer view
	mDescViewBufVert.BufferLocation = mBufVerts->GetGPUVirtualAddress();
	mDescViewBufVert.StrideInBytes = sizeof(VERTEX);
	mDescViewBufVert.SizeInBytes = sizeof(triangleVerts);

	// create fencing object
	ThrowIfFailed(mDevice->CreateFence(0, D3D12_FENCE_MISC_NONE, &mFence));
	mCurrentFence = 1;

	// close the command list and use it to execute the initial GPU setup
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* ppCommandLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// create event handle
	mHandleEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);

	// wait for the command list to execute; we are reusing the same command list in our main loop but for now, we just want to wait for setup to complete before continuing
	waitForPrevFrame();

#endif
}

#if 0
///
/// Fill the command list with all the render commands and dependent state
///
void populateCommandLists()
{
	// command list allocators can be only be reset when the associated command lists have finished execution on the GPU; apps should use fences to determine GPU execution progress
	ThrowIfFailed(mCommandAllocator->Reset());

	// HOWEVER, when ExecuteCommandList() is called on a particular command list, that command list can then be reset anytime and must be before rerecording
	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), mPSO.Get()));

	// set the viewport and scissor rectangle
	mCommandList->RSSetViewports(1, &mViewPort);
	mCommandList->RSSetScissorRects(1, &mRectScissor);

	// indicate this resource will be in use as a render target
	setResourceBarrier(mCommandList.Get(), mRenderTarget.Get(), D3D12_RESOURCE_USAGE_PRESENT, D3D12_RESOURCE_USAGE_RENDER_TARGET);

	// record commands
	float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	mCommandList->ClearRenderTargetView(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), clearColor, nullptr, 0);
	mCommandList->SetRenderTargets(&mDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), true, 1, nullptr);
	mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mCommandList->SetVertexBuffers(0, &mDescViewBufVert, 1);
	mCommandList->DrawInstanced(3, 1, 0, 0);

	// indicate that the render target will now be used to present when the command list is done executing
	setResourceBarrier(mCommandList.Get(), mRenderTarget.Get(), D3D12_RESOURCE_USAGE_RENDER_TARGET, D3D12_RESOURCE_USAGE_PRESENT);

	// all we need to do now is execute the command list
	ThrowIfFailed(mCommandList->Close());
}
#endif

// DirectX初期化
bool initDirectX(HWND hWnd)
{
	HRESULT hr;
	
	// デバイス作成
	hr = D3D12CreateDevice(
		nullptr, 
		D3D_DRIVER_TYPE_WARP,
		D3D12_CREATE_DEVICE_DEBUG,	// デバッグデバイス
		D3D_FEATURE_LEVEL_11_1,
		D3D12_SDK_VERSION, 
		__uuidof(ID3D12Device), 
		reinterpret_cast<void**>(&g_pDevice));

	if(showErrorMessage(hr, TEXT("デバイス作成失敗")))
	{
		return false;
	}

	// コマンドアロケータ作成
	hr = g_pDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		__uuidof(ID3D12CommandAllocator),
		reinterpret_cast<void**>(&g_pCommandAllocator));

	if (showErrorMessage(hr, TEXT("コマンドアロケータ作成失敗")))
	{
		return false;
	}

	// コマンドキュー作成
	// ドキュメントにある以下のようなメソッドはなかった
	//g_pDevice->GetDefaultCommandQueue(&g_pCommandQueue);
	D3D12_COMMAND_QUEUE_DESC commandQueueDesk;
	commandQueueDesk.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;	// コマンドリストタイプ
	commandQueueDesk.Priority = 0;							// 優先度
	commandQueueDesk.Flags = D3D12_COMMAND_QUEUE_NONE;		// フラグ
	commandQueueDesk.NodeMask = 0x00000001;					// ノードマスク

	hr = g_pDevice->CreateCommandQueue(
		&commandQueueDesk,
		__uuidof(ID3D12CommandQueue),
		reinterpret_cast<void**>(&g_pCommandQueue));

	if (showErrorMessage(hr, TEXT("コマンドキュー作成失敗")))
	{
		return false;
	}

	// ドキュメントの以下のような取得方法だと
	// GIデバイス作成でエラーが出るので変更
#if 0
	// GIデバイス作成
	hr = g_pDevice->QueryInterface(__uuidof(IDXGIDevice2), reinterpret_cast<void**>(&g_pGIDevice));
	CreateDXGIFactory2();
	if (showErrorMessage(hr, TEXT("GIデバイス作成失敗"));)
	{
		return false;
	}

	// GIアダプタ獲得
	hr = g_pGIDevice->GetAdapter(&g_pGIAdapter);
	if (showErrorMessage(hr, TEXT("GIアダプタ獲得失敗")))
	{
		return false;
	}

	// GIファクトリ獲得
	hr = g_pGIAdapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&g_pGIFactory));
	if (showErrorMessage(hr, TEXT("GIファクトリ獲得失敗"));)
	{
		return false;
	}
#else
	// GIファクトリ獲得
	hr = CreateDXGIFactory2(
		DXGI_CREATE_FACTORY_DEBUG,					// デバッグモードのファクトリ作成
		__uuidof(IDXGIFactory2),
		reinterpret_cast<void**>(&g_pGIFactory));
	if (showErrorMessage(hr, TEXT("GIファクトリ獲得失敗")))
	{
		return false;
	}
#endif

	// スワップチェーンを作成
	// ここはDirectX11とあまり変わらない
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.OutputWindow = hWnd;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.Windowed = TRUE;
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	hr = g_pGIFactory->CreateSwapChain(g_pDevice, &swapChainDesc, &g_pGISwapChain);
	if (showErrorMessage(hr, TEXT("スワップチェーン作成失敗")))
	{
		return false;
	}

	// 描画コマンドリスト作成
	hr = g_pDevice->CreateCommandList(
		commandQueueDesk.NodeMask,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		g_pCommandAllocator,
		g_pPipelineState,
		__uuidof(ID3D12GraphicsCommandList),
		reinterpret_cast<void**>(&g_pGraphicsCommandList));

	if (showErrorMessage(hr, TEXT("コマンドラインリスト作成失敗")))
	{
		return false;
	}

	// ディスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	ZeroMemory(&heapDesc, sizeof(heapDesc));

	heapDesc.NumDescriptors = 1;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_SHADER_VISIBLE;
	heapDesc.NodeMask = 0x00000001;

	for (int i = 0; i < DESCRIPTOR_HEAP_TYPE_MAX; ++i)
	{
		heapDesc.Type = static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i);
		hr = g_pDevice->CreateDescriptorHeap(
			&heapDesc,
			__uuidof(ID3D12DescriptorHeap),
			reinterpret_cast<void**>(&g_pDescripterHeapArray[i]));
		if (showErrorMessage(hr, TEXT("ディスクリプタヒープ作成失敗")))
		{
			return false;
		}
	}

	// ディスクリプタヒープとコマンドリストの関連づけ
	g_pGraphicsCommandList->SetDescriptorHeaps(g_pDescripterHeapArray, DESCRIPTOR_HEAP_TYPE_SET);

	// レンダーターゲットビューを作成
	hr = g_pGISwapChain->GetBuffer(0, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&g_pBackBufferResource));
	if (showErrorMessage(hr, TEXT("レンダーターゲットビュー作成失敗")))
	{
		return false;
	}
	g_hBackBufferRTV = g_pDescripterHeapArray[DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart();
	g_pDevice->CreateRenderTargetView(g_pBackBufferResource, nullptr, g_hBackBufferRTV);

#if 0
	// 深度ステンシルビュー作成
	hr = g_pGISwapChain->GetBuffer(0, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&g_pDepthStencilResource));
	if (showErrorMessage(hr, TEXT("深度ステンシルビュー作成失敗")))
	{
		return false;
	}
	g_hDepthStencilView = g_pDescripterHeapArray[DESCRIPTOR_HEAP_TYPE_DSV]->GetCPUDescriptorHandleForHeapStart();


	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

	depthStencilDesc.Format				= DXGI_FORMAT_D32_FLOAT;			// フォーマット
	depthStencilDesc.ViewDimension		= D3D12_DSV_DIMENSION_TEXTURE2D;	// リソースアクセス方法
	depthStencilDesc.Flags				= D3D12_DSV_NONE;					// アクセスフラグ
	depthStencilDesc.Texture2D.MipSlice = 0;								// 使用する最初のミップマップ

	g_pDevice->CreateDepthStencilView(g_pDepthStencilResource, &depthStencilDesc, g_hDepthStencilView);
#endif

	// レンダーターゲットビューを設定
	//g_pGraphicsCommandList->SetRenderTargets(&g_hBackBufferRTV, TRUE, 1, &g_hDepthStencilView);
	g_pGraphicsCommandList->SetRenderTargets(&g_hBackBufferRTV, TRUE, 1, nullptr);

	// ブレンドステート設定
	D3D12_BLEND_DESC blendDesc;
	blendDesc.RenderTarget[0].BlendEnable = true;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// シェーダ読み込み
	loadShader();
#if 0
	// パイプラインステート作成
	// の前になんかいろいろ用意しないといけない・・・

	// ルートシグニチャの作成


	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(psoDesc));

	psoDesc.pRootSignature;	// ルートシグニチャ
	psoDesc.NumRenderTargets = 1;

	hr = g_pDevice->CreateGraphicsPipelineState(
		&psoDesc,
		__uuidof(ID3D12PipelineState),
		reinterpret_cast<void**>(&g_pPipelineState));
	if (showErrorMessage(hr, TEXT("パイプラインステート作成失敗")))
	{
		return false;
	}
#else
	//g_pGraphicsCommandList->SetPipelineState(nullptr);
#endif

	return true;
}

// DirectX12クリーンアップ
void cleanupDirectX()
{
	safeRelease(g_pGISwapChain);
	safeRelease(g_pGIFactory);
	safeRelease(g_pGIAdapter);
	safeRelease(g_pGIDevice);
	safeRelease(g_pCommandQueue);
	safeRelease(g_pCommandAllocator);
	safeRelease(g_pDevice);
}

void setResourceBarrier(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, UINT StateBefore, UINT StateAfter)
{
	D3D12_RESOURCE_BARRIER_DESC barrierDesc = {};

	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierDesc.Transition.pResource = resource;
	barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrierDesc.Transition.StateBefore = StateBefore;
	barrierDesc.Transition.StateAfter = StateAfter;

	commandList->ResourceBarrier(1, &barrierDesc);
}


// 描画
void Render()
{
#if 1
	// ビューポート設定
	D3D12_VIEWPORT vp;
	vp.TopLeftX = 0;			// X座標
	vp.TopLeftY = 0;			// Y座標
	vp.Width = SCREEN_WIDTH;	// 幅
	vp.Height = SCREEN_HEIGHT;	// 高さ
	vp.MinDepth = 0.0f;			// 最少深度
	vp.MaxDepth = 1.0f;			// 最大深度
	g_pGraphicsCommandList->RSSetViewports(1, &vp);

	// Indicate that this resource will be in use as a render target.
	//setResourceBarrier(mCommandList, mRenderTarget, D3D12_RESOURCE_USAGE_PRESENT, D3D12_RESOURCE_USAGE_RENDER_TARGET);

	// クリア.
	float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	CD3D12_RECT clearRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	// レンダーターゲットビューを設定
	g_pGraphicsCommandList->SetRenderTargets(&g_hBackBufferRTV, TRUE, 1, nullptr);
	g_pGraphicsCommandList->ClearRenderTargetView(g_hBackBufferRTV, clearColor, &clearRect, 1);

	// Indicate that the render target will now be used to present when the command list is done executing.
	setResourceBarrier(g_pGraphicsCommandList, g_pBackBufferResource, D3D12_RESOURCE_USAGE_RENDER_TARGET, D3D12_RESOURCE_USAGE_PRESENT);

	// Execute the command list.
	g_pGraphicsCommandList->Close();

	ID3D12CommandList* pTemp = g_pGraphicsCommandList;
	g_pCommandQueue->ExecuteCommandLists(1, &pTemp);

	// フリップ
	g_pGISwapChain->Present(1, 0);
	
	g_pCommandAllocator->Reset();
	g_pGraphicsCommandList->Reset(g_pCommandAllocator, g_pPipelineState);
#endif
}

// ウィンドウプロシージャ
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// メッセージ分岐
	switch (msg)
	{
	case WM_KEYDOWN:	// キーが押された時
		switch (wparam)
		{
		case VK_ESCAPE:
			DestroyWindow(hwnd);
			break;
		}
		break;

	case WM_DESTROY:	// ウィンドウ破棄
		PostQuitMessage(0);
		break;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

// エントリーポイント
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	HWND hwnd;
	MSG msg;
	WNDCLASS winc;

	winc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	winc.lpfnWndProc = WndProc;					// ウィンドウプロシージャ
	winc.cbClsExtra = 0;
	winc.cbWndExtra = 0;
	winc.hInstance = hInstance;
	winc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	winc.hCursor = LoadCursor(NULL, IDC_ARROW);
	winc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	winc.lpszMenuName = NULL;
	winc.lpszClassName = CLASS_NAME;

	// ウィンドウクラス登録
	if (RegisterClass(&winc) == false)
	{
		return 1;
	}

	// ウィンドウ作成
	hwnd = CreateWindow(
		CLASS_NAME,
		CLASS_NAME,
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
		NULL,
		NULL,
		hInstance,
		NULL);

	if (hwnd == NULL)
	{
		return 0;
	}

	// DirectX初期化
	if (initDirectX(hwnd) == false)
	{
		return 1;
	}

	// ウィンドウ表示
	ShowWindow(hwnd, nCmdShow);

	// メッセージループ
	do {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// メイン
			Render();
		}
	} while (msg.message != WM_QUIT);

	// 解放処理
	cleanupDirectX();

	// 登録したクラスを解除
	UnregisterClass(CLASS_NAME, hInstance);

	return 0;
}