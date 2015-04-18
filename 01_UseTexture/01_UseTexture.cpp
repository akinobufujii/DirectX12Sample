//==============================================================================
// インクルード
//==============================================================================
#include <tchar.h>
#include <Windows.h>

#include <memory>

#include <d3d12.h>
#include <Dxgi1_3.h>
#include <D3d12SDKLayers.h>
#include <d3dcompiler.h>

#include <DirectXMath.h>

#pragma warning( push )
#pragma warning( disable: 4013 4068 4312 4456 4457 )
#define STB_IMAGE_IMPLEMENTATION
#include "../libs/stb-master/stb_image.h"
#pragma warning( pop )

//==============================================================================
// ライブラリリンク
//==============================================================================
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "D3DCompiler.lib")

//==============================================================================
// 定義
//==============================================================================
// 入力レイアウト
const D3D12_INPUT_ELEMENT_DESC INPUT_LAYOUT[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_PER_VERTEX_DATA, 0 },
};

// 頂点定義
struct UserVertex
{
	DirectX::XMFLOAT3 pos;		// 頂点座標
	DirectX::XMFLOAT4 color;	// 頂点カラー
	DirectX::XMFLOAT2 uv;		// UV座標
};


//==============================================================================
// 定数
//==============================================================================
static const int SCREEN_WIDTH = 1280;				// 画面幅
static const int SCREEN_HEIGHT = 720;				// 画面高さ
static const LPTSTR	CLASS_NAME = TEXT("DirectX");	// ウィンドウネーム

// ディスクリプタヒープタイプ
enum DESCRIPTOR_HEAP_TYPE 
{
	DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,	// UAVなど用
	DESCRIPTOR_HEAP_TYPE_SAMPLER,		// サンプラ用
	DESCRIPTOR_HEAP_TYPE_RTV,			// レンダーターゲット用	
	DESCRIPTOR_HEAP_TYPE_DSV,			// デプスステンシル用	
	DESCRIPTOR_HEAP_TYPE_MAX,			// ディスクリプタヒープ数
	DESCRIPTOR_HEAP_TYPE_SET = DESCRIPTOR_HEAP_TYPE_SAMPLER + 1,
};

// ルートパラメータータイプ
enum ROOT_PARAM_TYPE
{
	ROOT_PARAM_TYPE_DEFAULT,	// デフォルト
	ROOT_PARAM_TYPE_CBV,		// コンスタントバッファ用
	ROOT_PARAM_TYPE_SRV,		// シェーダリソースビュー用
	ROOT_PARAM_TYPE_SAMPLER,	// サンプラ用	
	ROOT_PARAM_TYPE_MAX,		// ディスクリプタヒープ数
};

//static const UINT	COMMAND_ALLOCATOR_MAX = 2;	// コマンドアロケータ数

//==============================================================================
// グローバル変数
//==============================================================================
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
ID3D12RootSignature*		g_pRootSignature;									// ルートシグニチャ
D3D12_VIEWPORT				g_viewPort;											// ビューポート
ID3D12Fence*				g_pFence;											// フェンスオブジェクト
HANDLE						g_hFenceEvent;										// フェンスイベントハンドル

ID3D12Resource*				g_pBackBufferResource;								// バックバッファのリソース
D3D12_CPU_DESCRIPTOR_HANDLE	g_hBackBuffer;										// バックバッファハンドル
ID3D12Resource*				g_pDepthStencilResource;							// デプスステンシルのリソース
D3D12_CPU_DESCRIPTOR_HANDLE	g_hDepthStencil;									// デプスステンシルのハンドル

LPD3DBLOB					g_pVSBlob;											// 頂点シェーダブロブ
LPD3DBLOB					g_pPSBlob;											// ピクセルシェーダブロブ

ID3D12Resource*				g_pVertexBufferResource;							// 頂点バッファのリソース
D3D12_VERTEX_BUFFER_VIEW	g_VertexBufferView;									// 頂点バッファビュー

ID3D12Heap*					g_pTextureHeap;										// テクスチャヒープ
ID3D12Resource*				g_pTextureResource;									// テクスチャのリソース
D3D12_CPU_DESCRIPTOR_HANDLE	g_hTexure;											// テクスチャハンドル
D3D12_GPU_DESCRIPTOR_HANDLE g_hGPUTexture;										// GPUテクスチャハンドル

D3D12_CPU_DESCRIPTOR_HANDLE	g_hSampler;											// サンプラハンドル
D3D12_GPU_DESCRIPTOR_HANDLE	g_hGPUSampler;										// GPUサンプラハンドル


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
		IID_PPV_ARGS(&g_pDevice));

	if(showErrorMessage(hr, TEXT("デバイス作成失敗")))
	{
		return false;
	}

	// コマンドアロケータ作成
	hr = g_pDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&g_pCommandAllocator));

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
	commandQueueDesk.NodeMask = 0x00000000;					// ノードマスク

	hr = g_pDevice->CreateCommandQueue(&commandQueueDesk, IID_PPV_ARGS(&g_pCommandQueue));

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
	// デバッグモードのファクトリ作成
	hr = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&g_pGIFactory));
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

	// デバイスじゃなくてコマンドキューを渡す
	// でないと実行時エラーが起こる
	hr = g_pGIFactory->CreateSwapChain(g_pCommandQueue, &swapChainDesc, &g_pGISwapChain);
	if (showErrorMessage(hr, TEXT("スワップチェーン作成失敗")))
	{
		return false;
	}

	// 描画コマンドリスト作成
	hr = g_pDevice->CreateCommandList(
		0x00000000,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		g_pCommandAllocator,
		g_pPipelineState,
		IID_PPV_ARGS(&g_pGraphicsCommandList));

	if (showErrorMessage(hr, TEXT("コマンドラインリスト作成失敗")))
	{
		return false;
	}
	
	// 頂点シェーダコンパイル
	if (compileShaderFlomFile(L"VertexShader.hlsl", "main", "vs_5_1", &g_pVSBlob) == false)
	{
		showErrorMessage(E_FAIL, TEXT("頂点シェーダコンパイル失敗"));
	}

	// ピクセルシェーダコンパイル
	if (compileShaderFlomFile(L"PixelShaderByTexture.hlsl", "main", "ps_5_1", &g_pPSBlob) == false)
	{
		showErrorMessage(E_FAIL, TEXT("ピクセルシェーダコンパイル失敗"));
	}

	// シェーダリソースビューを用にルートパラメータを初期化する
	D3D12_ROOT_PARAMETER	descRootParam[ROOT_PARAM_TYPE_MAX];

	descRootParam[ROOT_PARAM_TYPE_DEFAULT].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

	D3D12_DESCRIPTOR_RANGE descRange[ROOT_PARAM_TYPE_MAX];
	descRange[ROOT_PARAM_TYPE_CBV].Init(D3D12_DESCRIPTOR_RANGE_CBV, 2, 1, 0U, 0);
	descRootParam[ROOT_PARAM_TYPE_CBV].InitAsDescriptorTable(1, &descRange[ROOT_PARAM_TYPE_CBV], D3D12_SHADER_VISIBILITY_VERTEX);

	descRange[ROOT_PARAM_TYPE_SRV].Init(D3D12_DESCRIPTOR_RANGE_SRV, 1, 0, 0U, 0);
	descRootParam[ROOT_PARAM_TYPE_SRV].InitAsDescriptorTable(1, &descRange[ROOT_PARAM_TYPE_SRV], D3D12_SHADER_VISIBILITY_PIXEL);

	descRange[ROOT_PARAM_TYPE_SAMPLER].Init(D3D12_DESCRIPTOR_RANGE_SAMPLER, 1, 0, 0U, 0);
	descRootParam[ROOT_PARAM_TYPE_SAMPLER].InitAsDescriptorTable(1, &descRange[ROOT_PARAM_TYPE_SAMPLER], D3D12_SHADER_VISIBILITY_PIXEL);

	// テクスチャに対応したルートシグニチャ作成
	LPD3DBLOB pOutBlob = nullptr;
	D3D12_ROOT_SIGNATURE	descRootSignature = D3D12_ROOT_SIGNATURE();
	
	descRootSignature.Flags = D3D12_ROOT_SIGNATURE_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	descRootSignature.NumParameters = ROOT_PARAM_TYPE_MAX;
	descRootSignature.pParameters = descRootParam;
	D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_V1, &pOutBlob, nullptr);
	hr = g_pDevice->CreateRootSignature(
		0x00000001,
		pOutBlob->GetBufferPointer(),
		pOutBlob->GetBufferSize(),
		IID_PPV_ARGS(&g_pRootSignature));

	safeRelease(pOutBlob);
	if (showErrorMessage(hr, TEXT("ルートシグニチャ作成失敗")))
	{
		return false;
	}

	// パイプラインステートオブジェクト作成
	// 頂点シェーダとピクセルシェーダがないと、作成に失敗する
	D3D12_GRAPHICS_PIPELINE_STATE_DESC descPSO;
	ZeroMemory(&descPSO, sizeof(descPSO));
	descPSO.InputLayout = { INPUT_LAYOUT, ARRAYSIZE(INPUT_LAYOUT) };										// インプットレイアウト設定
	descPSO.pRootSignature = g_pRootSignature;																// ルートシグニチャ設定
	descPSO.VS = { reinterpret_cast<BYTE*>(g_pVSBlob->GetBufferPointer()), g_pVSBlob->GetBufferSize() };	// 頂点シェーダ設定
	descPSO.PS = { reinterpret_cast<BYTE*>(g_pPSBlob->GetBufferPointer()), g_pPSBlob->GetBufferSize() };	// ピクセルシェーダ設定
	descPSO.RasterizerState = CD3D12_RASTERIZER_DESC(D3D12_DEFAULT);										// ラスタライザ設定
	descPSO.BlendState = CD3D12_BLEND_DESC(D3D12_DEFAULT);													// ブレンド設定
	descPSO.DepthStencilState.DepthEnable = FALSE;															// 深度バッファ有効設定
	descPSO.DepthStencilState.StencilEnable = FALSE;														// ステンシルバッファ有効設定
	descPSO.SampleMask = UINT_MAX;																			// サンプルマスク設定
	descPSO.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;									// プリミティブタイプ	
	descPSO.NumRenderTargets = 1;																			// レンダーターゲット数
	descPSO.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;														// レンダーターゲットフォーマット
	descPSO.SampleDesc.Count = 1;																			// サンプルカウント

	hr = g_pDevice->CreateGraphicsPipelineState(&descPSO, IID_PPV_ARGS(&g_pPipelineState));

	if (showErrorMessage(hr, TEXT("パイプラインステートオブジェクト作成失敗")))
	{
		return false;
	}

	// ディスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	ZeroMemory(&heapDesc, sizeof(heapDesc));

	heapDesc.NumDescriptors = 1;

	for (int i = 0; i < DESCRIPTOR_HEAP_TYPE_MAX; ++i)
	{
		heapDesc.Flags = (i == D3D12_RTV_DESCRIPTOR_HEAP) ? D3D12_DESCRIPTOR_HEAP_NONE : D3D12_DESCRIPTOR_HEAP_SHADER_VISIBLE;
		heapDesc.Type = static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i);
		
		hr = g_pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&g_pDescripterHeapArray[i]));
		if (showErrorMessage(hr, TEXT("ディスクリプタヒープ作成失敗")))
		{
			return false;
		}
	}

	// ディスクリプタヒープとコマンドリストの関連づけ
	g_pGraphicsCommandList->SetDescriptorHeaps(g_pDescripterHeapArray, DESCRIPTOR_HEAP_TYPE_SET);

	// レンダーターゲットビューを作成
	hr = g_pGISwapChain->GetBuffer(0, IID_PPV_ARGS(&g_pBackBufferResource));
	if (showErrorMessage(hr, TEXT("レンダーターゲットビュー作成失敗")))
	{
		return false;
	}
	
	g_hBackBuffer = g_pDescripterHeapArray[DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart();
	g_pDevice->CreateRenderTargetView(g_pBackBufferResource, nullptr, g_hBackBuffer);

#if 0
	// 深度ステンシルビュー作成
	hr = g_pGISwapChain->GetBuffer(0, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&g_pDepthStencilResource));
	if (showErrorMessage(hr, TEXT("深度ステンシルビュー作成失敗")))
	{
		return false;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;			// フォーマット
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;	// リソースアクセス方法
	depthStencilDesc.Flags = D3D12_DSV_NONE;					// アクセスフラグ
	depthStencilDesc.Texture2D.MipSlice = 0;								// 使用する最初のミップマップ

	g_hDepthStencil = g_pDescripterHeapArray[DESCRIPTOR_HEAP_TYPE_DSV]->GetCPUDescriptorHandleForHeapStart();
	g_pDevice->CreateDepthStencilView(g_pDepthStencilResource, &depthStencilDesc, g_hDepthStencil);
#endif

	// ビューポート設定
	g_viewPort.TopLeftX = 0;			// X座標
	g_viewPort.TopLeftY = 0;			// Y座標
	g_viewPort.Width = SCREEN_WIDTH;	// 幅
	g_viewPort.Height = SCREEN_HEIGHT;	// 高さ
	g_viewPort.MinDepth = 0.0f;			// 最少深度
	g_viewPort.MaxDepth = 1.0f;			// 最大深度

	// フェンスオブジェクト作成
	g_pDevice->CreateFence(0, D3D12_FENCE_MISC_NONE, IID_PPV_ARGS(&g_pFence));

	if (showErrorMessage(hr, TEXT("フェンスオブジェクト作成失敗")))
	{
		return false;
	}

	// フェンスイベントハンドル作成
	g_hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	return true;
}

// DirectX12クリーンアップ
void cleanupDirectX()
{
	CloseHandle(g_hFenceEvent);
	safeRelease(g_pFence);
	safeRelease(g_pBackBufferResource);
	for (int i = 0; i < DESCRIPTOR_HEAP_TYPE_MAX; ++i)
	{
		safeRelease(g_pDescripterHeapArray[i]);
	}
	safeRelease(g_pPipelineState);
	safeRelease(g_pRootSignature);
	safeRelease(g_pPSBlob);
	safeRelease(g_pVSBlob);
	
	safeRelease(g_pGISwapChain);
	safeRelease(g_pGIFactory);
	safeRelease(g_pGIAdapter);
	safeRelease(g_pGIDevice);
	safeRelease(g_pCommandQueue);
	safeRelease(g_pGraphicsCommandList);
	safeRelease(g_pCommandAllocator);
	safeRelease(g_pDevice);
}

// リソース設定時のバリア関数
void setResourceBarrier(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, UINT StateBefore, UINT StateAfter)
{
	D3D12_RESOURCE_BARRIER_DESC descBarrier = {};
	ZeroMemory(&descBarrier, sizeof(descBarrier));

	descBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	descBarrier.Transition.pResource = resource;
	descBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	descBarrier.Transition.StateBefore = StateBefore;
	descBarrier.Transition.StateAfter = StateAfter;

	commandList->ResourceBarrier(1, &descBarrier);
}

// リソースセットアップ
bool setupResource() 
{
	HRESULT hr;

	// 頂点バッファ作成
	UserVertex vertex[] =
	{
		{ DirectX::XMFLOAT3(-0.5f,  0.5f, 0.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), DirectX::XMFLOAT2(0.0f, 0.0f) },	// 左上
		{ DirectX::XMFLOAT3( 0.5f,  0.5f, 0.0f), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f), DirectX::XMFLOAT2(1.0f, 0.0f) },	// 右上
		{ DirectX::XMFLOAT3(-0.5f, -0.5f, 0.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f), DirectX::XMFLOAT2(0.0f, 1.0f) },	// 左下
		{ DirectX::XMFLOAT3( 0.5f, -0.5f, 0.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f), DirectX::XMFLOAT2(1.0f, 1.0f) },	// 右下
	};

	// ※ドキュメントには静的データをアップロードヒープに渡すのは推奨しないとのこと
	// 　ヒープの消費量がよろしくないようです
	hr = g_pDevice->CreateCommittedResource(
		&CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_MISC_NONE,
		&CD3D12_RESOURCE_DESC::Buffer(ARRAYSIZE(vertex) * sizeof(UserVertex)),
		D3D12_RESOURCE_USAGE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&g_pVertexBufferResource));

	if (showErrorMessage(hr, TEXT("頂点バッファ作成失敗")))
	{
		return false;
	}

	// 頂点バッファに三角形情報をコピーする
	UINT8* dataBegin;
	if (SUCCEEDED(g_pVertexBufferResource->Map(0, nullptr, reinterpret_cast<void**>(&dataBegin)))) 
	{
		memcpy(dataBegin, vertex, sizeof(vertex));
		g_pVertexBufferResource->Unmap(0, nullptr);
	}
	else 
	{
		showErrorMessage(E_FAIL, TEXT("頂点バッファのマッピングに失敗"));
		return false;
	}

	// 頂点バッファビュー設定
	g_VertexBufferView.BufferLocation = g_pVertexBufferResource->GetGPUVirtualAddress();
	g_VertexBufferView.StrideInBytes = sizeof(UserVertex);
	g_VertexBufferView.SizeInBytes = sizeof(vertex);

	// サンプラオブジェクト作成
	D3D12_SAMPLER_DESC descSampler;
	ZeroMemory(&descSampler, sizeof(descSampler));
	descSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	descSampler.AddressU = D3D12_TEXTURE_ADDRESS_WRAP;
	descSampler.AddressV = D3D12_TEXTURE_ADDRESS_WRAP;
	descSampler.AddressW = D3D12_TEXTURE_ADDRESS_WRAP;
	descSampler.MipLODBias = 0.0f;
	descSampler.MaxAnisotropy = 1;
	descSampler.ComparisonFunc = D3D12_COMPARISON_ALWAYS;
	descSampler.MinLOD = 0;
	descSampler.MaxLOD = D3D12_FLOAT32_MAX;

	g_hSampler = g_pDescripterHeapArray[DESCRIPTOR_HEAP_TYPE_SAMPLER]->GetCPUDescriptorHandleForHeapStart();
	g_hGPUSampler = g_pDescripterHeapArray[DESCRIPTOR_HEAP_TYPE_SAMPLER]->GetGPUDescriptorHandleForHeapStart();
	g_pDevice->CreateSampler(&descSampler, g_hSampler);

	// テクスチャ作成(後で関数化を・・・)
	// テクスチャの生データを読み込み(stbライブラリで簡略化)
	int width, height, comp;
	stbi_uc* pixels = stbi_load("../resource/お気に入り.png", &width, &height, &comp, 0);

	// テクスチャのヒープ領域を確保
	hr = g_pDevice->CreateHeap(
		&CD3D12_HEAP_DESC(CD3D12_RESOURCE_ALLOCATION_INFO(width * height * 32, 0), D3D12_HEAP_TYPE_UPLOAD),
		IID_PPV_ARGS(&g_pTextureHeap));
	
	if (showErrorMessage(hr, TEXT("テクスチャー用ヒープ作成失敗")))
	{
		return false;
	}

	// 確保したヒープからリソースオブジェクトを作成
	hr = g_pDevice->CreatePlacedResource(
		g_pTextureHeap,
		0,
		&CD3D12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height),
		D3D12_RESOURCE_USAGE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&g_pTextureResource));

	if (showErrorMessage(hr, TEXT("テクスチャー作成失敗")))
	{
		return false;
	}

	// テクスチャの内容書き込み
	UINT rowPitch = (width * 32 + 7) / 8;	// 画像幅をBPPでかけて8ビットの要素が何個あるか計算する
	UINT depthPitch = width * height * 32;	// データの大きさ
	g_pTextureResource->WriteToSubresource(0, nullptr, pixels, rowPitch, depthPitch);

	// シェーダリソースビューとして関連付け
	g_hTexure = g_pDescripterHeapArray[DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetCPUDescriptorHandleForHeapStart();
	g_hGPUTexture = g_pDescripterHeapArray[DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetGPUDescriptorHandleForHeapStart();

	g_pDevice->CreateShaderResourceView(g_pTextureResource, nullptr, g_hTexure);

	// いらないもの解放
	stbi_image_free(pixels);

	return true;
}

// リソースクリーンアップ
void cleanupResource()
{
	safeRelease(g_pTextureResource);
	safeRelease(g_pTextureHeap);
	safeRelease(g_pVertexBufferResource);
}

// 描画
void Render()
{
	// 現在のコマンドリストを獲得
	ID3D12GraphicsCommandList* pCommand = g_pGraphicsCommandList;

	// パイプラインステートオブジェクトとルートシグニチャをセット
	pCommand->SetPipelineState(g_pPipelineState);
	pCommand->SetGraphicsRootSignature(g_pRootSignature);

	// レンダーターゲットビューを設定
	pCommand->SetRenderTargets(&g_hBackBuffer, TRUE, 1, nullptr);

	// 矩形を設定
	CD3D12_RECT clearRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	pCommand->RSSetViewports(1, &g_viewPort);
	pCommand->RSSetScissorRects(1, &clearRect);

	// レンダーターゲットへリソースを設定
	setResourceBarrier(
		pCommand,
		g_pBackBufferResource,
		D3D12_RESOURCE_USAGE_PRESENT,
		D3D12_RESOURCE_USAGE_RENDER_TARGET);

	// クリア
	static float count = 0;
	count = fmod(count + 0.01f, 1.0f);
	float clearColor[] = { count, 0.2f, 0.4f, 1.0f };
	pCommand->ClearRenderTargetView(g_hBackBuffer, clearColor, &clearRect, 1);

	// ディスクリプタヒープをセット
	ID3D12DescriptorHeap* pHeaps[2] = { 
		g_pDescripterHeapArray[DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV], 
		g_pDescripterHeapArray[DESCRIPTOR_HEAP_TYPE_SAMPLER] 
	};

	pCommand->SetDescriptorHeaps(pHeaps, 2);

	// リソースセット
	pCommand->SetGraphicsRootDescriptorTable(ROOT_PARAM_TYPE_SRV, g_hGPUTexture); 
	pCommand->SetGraphicsRootDescriptorTable(ROOT_PARAM_TYPE_SAMPLER, g_hGPUSampler);

	// 三角形描画
	pCommand->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	pCommand->SetVertexBuffers(0, &g_VertexBufferView, 1);
	pCommand->DrawInstanced(4, 1, 0, 0);
	
	// present前の処理
	setResourceBarrier(
		pCommand,
		g_pBackBufferResource,
		D3D12_RESOURCE_USAGE_RENDER_TARGET,
		D3D12_RESOURCE_USAGE_PRESENT);

	// 描画コマンドを実行してフリップ
	pCommand->Close();
	g_pCommandQueue->ExecuteCommandLists(1, CommandListCast(&pCommand));
	g_pGISwapChain->Present(1, 0);

	// コマンドキューの処理を待つ
	g_pFence->Signal(0);
	g_pFence->SetEventOnCompletion(1, g_hFenceEvent);
	g_pCommandQueue->Signal(g_pFence, 1);
	WaitForSingleObject(g_hFenceEvent, INFINITE);

	// コマンドアロケータとコマンドリストをリセット
	g_pCommandAllocator->Reset();
	pCommand->Reset(g_pCommandAllocator, g_pPipelineState);
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
		return 1;
	}

	// DirectX初期化
	if (initDirectX(hwnd) == false)
	{
		return 1;
	}

	if (setupResource() == false) 
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
	cleanupResource();
	cleanupDirectX();

	// 登録したクラスを解除
	UnregisterClass(CLASS_NAME, hInstance);

	return 0;
}