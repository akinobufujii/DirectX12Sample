//==============================================================================
// インクルード
//==============================================================================
#include <tchar.h>
#include <Windows.h>

#include <memory>

#include <d3d12.h>
/*
ドキュメントにはあるって書いているのにない拡張ライブラリ
https://msdn.microsoft.com/en-us/library/dn708058(v=vs.85).aspx
#include <d3dx12.h>
*/
#include <Dxgi1_2.h>
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
//#define RESOURCE_SETUP	// リソースセットアップをする

// 入力レイアウト
const D3D12_INPUT_ELEMENT_DESC INPUT_LAYOUT[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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

//static const UINT	COMMAND_ALLOCATOR_MAX = 2;	// コマンドアロケータ数

//==============================================================================
// グローバル変数
//==============================================================================
ID3D12Device*				g_pDevice;											// デバイス
ID3D12Debug*				g_pDebug;											// デバッグオブジェクト
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

ID3D12Resource*				g_pTextureResource;									// テクスチャのリソース
D3D12_CPU_DESCRIPTOR_HANDLE	g_hTexure;											// テクスチャハンドル

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

#if _DEBUG
	// デバッグレイヤー作成
	hr = D3D12GetDebugInterface(IID_PPV_ARGS(&g_pDebug));

	if(g_pDebug)
	{
		g_pDebug->EnableDebugLayer();
	}

	if (showErrorMessage(hr, TEXT("デバッグレイヤー作成失敗")))
	{
		return false;
	}
#endif

	// GIファクトリ獲得
	// デバッグモードのファクトリ作成
	hr = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&g_pGIFactory));
	if (showErrorMessage(hr, TEXT("GIファクトリ獲得失敗")))
	{
		return false;
	}

	IDXGIAdapter* pGIAdapter = nullptr;
	hr = g_pGIFactory->EnumAdapters(0, &pGIAdapter);
	if (showErrorMessage(hr, TEXT("GIアダプター獲得失敗")))
	{
		return false;
	}
	
	// デバイス作成
	hr = D3D12CreateDevice(
		pGIAdapter,
		D3D_FEATURE_LEVEL_11_0,
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
		hr = g_pDevice->GetDeviceRemovedReason();
		return false;
	}

	// コマンドキュー作成
	// ドキュメントにある以下のようなメソッドはなかった
	//g_pDevice->GetDefaultCommandQueue(&g_pCommandQueue);
	D3D12_COMMAND_QUEUE_DESC commandQueueDesk;
	commandQueueDesk.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;	// コマンドリストタイプ
	commandQueueDesk.Priority = 0;							// 優先度
	commandQueueDesk.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;	// フラグ
	commandQueueDesk.NodeMask = 0x00000000;					// ノードマスク

	hr = g_pDevice->CreateCommandQueue(&commandQueueDesk, IID_PPV_ARGS(&g_pCommandQueue));

	if (showErrorMessage(hr, TEXT("コマンドキュー作成失敗")))
	{
		return false;
	}

	// スワップチェーンを作成
	// ここはDirectX11とあまり変わらない
	// 参考URL:https://msdn.microsoft.com/en-us/library/dn859356(v=vs.85).aspx

	DXGI_SWAP_CHAIN_DESC descSwapChain;
	ZeroMemory(&descSwapChain, sizeof(descSwapChain));
	descSwapChain.BufferCount = 2;
	descSwapChain.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	descSwapChain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	descSwapChain.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	descSwapChain.OutputWindow = hWnd;
	descSwapChain.SampleDesc.Count = 1;
	descSwapChain.Windowed = TRUE;

	// デバイスじゃなくてコマンドキューを渡す
	// でないと実行時エラーが起こる
	hr = g_pGIFactory->CreateSwapChain(g_pCommandQueue, &descSwapChain, &g_pGISwapChain);
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
	if (compileShaderFlomFile(L"PixelShader.hlsl", "main", "ps_5_1", &g_pPSBlob) == false)
	{
		showErrorMessage(E_FAIL, TEXT("ピクセルシェーダコンパイル失敗"));
	}

	// 空のルートシグニチャ作成
	LPD3DBLOB pOutBlob = nullptr;
	D3D12_ROOT_SIGNATURE_DESC descRootSignature = D3D12_ROOT_SIGNATURE_DESC();
	descRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &pOutBlob, nullptr);
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

	// ライスタライザーステート設定
	// デフォルトの設定は以下のページを参照(ScissorEnableはないけど・・・)
	// https://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=JA-JP&k=k(d3d12%2FD3D12_RASTERIZER_DESC);k(D3D12_RASTERIZER_DESC);k(DevLang-C%2B%2B);k(TargetOS-Windows)&rd=true
	D3D12_RASTERIZER_DESC descRasterizer;
	descRasterizer.FillMode = D3D12_FILL_MODE_SOLID;
	descRasterizer.CullMode = D3D12_CULL_MODE_BACK;
	descRasterizer.FrontCounterClockwise = FALSE;
	descRasterizer.DepthBias = 0;
	descRasterizer.SlopeScaledDepthBias = 0.0f;
	descRasterizer.DepthBiasClamp = 0.0f;
	descRasterizer.DepthClipEnable = TRUE;
	descRasterizer.MultisampleEnable = FALSE;
	descRasterizer.AntialiasedLineEnable = FALSE;
	descRasterizer.ForcedSampleCount = 0;
	descRasterizer.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// ブレンドステート設定
	// https://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=JA-JP&k=k(d3d12%2FD3D12_BLEND_DESC);k(D3D12_BLEND_DESC);k(DevLang-C%2B%2B);k(TargetOS-Windows)&rd=true
	D3D12_BLEND_DESC descBlend;
	descBlend.AlphaToCoverageEnable = FALSE;
	descBlend.IndependentBlendEnable = FALSE;
	descBlend.RenderTarget[0].BlendEnable = FALSE;
	descBlend.RenderTarget[0].LogicOpEnable	= FALSE;
	descBlend.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	descBlend.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
	descBlend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	descBlend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	descBlend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	descBlend.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	descBlend.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	descBlend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// パイプラインステートオブジェクト作成
	// 頂点シェーダとピクセルシェーダがないと、作成に失敗する
	D3D12_GRAPHICS_PIPELINE_STATE_DESC descPSO;
	ZeroMemory(&descPSO, sizeof(descPSO));
	descPSO.InputLayout = { INPUT_LAYOUT, ARRAYSIZE(INPUT_LAYOUT) };										// インプットレイアウト設定
	descPSO.pRootSignature = g_pRootSignature;																// ルートシグニチャ設定
	descPSO.VS = { reinterpret_cast<BYTE*>(g_pVSBlob->GetBufferPointer()), g_pVSBlob->GetBufferSize() };	// 頂点シェーダ設定
	descPSO.PS = { reinterpret_cast<BYTE*>(g_pPSBlob->GetBufferPointer()), g_pPSBlob->GetBufferSize() };	// ピクセルシェーダ設定
	descPSO.RasterizerState = descRasterizer;																// ラスタライザ設定
	descPSO.BlendState = descBlend;																			// ブレンド設定
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
		heapDesc.Flags = (i == D3D12_DESCRIPTOR_HEAP_TYPE_RTV) ? D3D12_DESCRIPTOR_HEAP_FLAG_NONE : D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.Type = static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i);
		
		hr = g_pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&g_pDescripterHeapArray[i]));
		if (showErrorMessage(hr, TEXT("ディスクリプタヒープ作成失敗")))
		{
			return false;
		}
	}

	// ディスクリプタヒープとコマンドリストの関連づけ
	g_pGraphicsCommandList->SetDescriptorHeaps(DESCRIPTOR_HEAP_TYPE_SET, g_pDescripterHeapArray);

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
	g_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_pFence));

	if (showErrorMessage(hr, TEXT("フェンスオブジェクト作成失敗")))
	{
		return false;
	}

	// フェンスイベントハンドル作成
	g_hFenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

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

#if defined(RESOURCE_SETUP)
// リソースセットアップ
bool setupResource() 
{
	HRESULT hr;

	// 頂点バッファ作成
	UserVertex vertex[] =
	{
		{ DirectX::XMFLOAT3( 0.0f,  0.5f, 0.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), DirectX::XMFLOAT2(0.0f, 0.0f) },
		{ DirectX::XMFLOAT3( 0.5f, -0.5,  0.0f), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f), DirectX::XMFLOAT2(1.0f, 0.0f) },
		{ DirectX::XMFLOAT3(-0.5f, -0.5f, 0.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f), DirectX::XMFLOAT2(1.0f, 1.0f) },
	};

	// ※ドキュメントには静的データをアップロードヒープに渡すのは推奨しないとのこと
	// 　ヒープの消費量がよろしくないようです
	hr = g_pDevice->CreateCommittedResource(
		&CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_MISC_NONE,
		&CD3D12_RESOURCE_DESC::Buffer(ARRAYSIZE(vertex) * sizeof(UserVertex)),
		D3D12_RESOURCE_USAGE_GENERIC_READ,
		nullptr,
		__uuidof(ID3D12Resource),
		reinterpret_cast<void**>(&g_pVertexBufferResource));

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

#if 0
	// テクスチャ作成
	// リソースバッファを獲得
	hr = g_pGISwapChain->GetBuffer(0, IID_PPV_ARGS(&g_pTextureResource));
	if (showErrorMessage(hr, TEXT("テクスチャー用バッファ獲得失敗")))
	{
		return false;
	}
	D3D12_SHADER_RESOURCE_VIEW_DESC descSRV;
	ZeroMemory(&descSRV, sizeof(descSRV));
	descSRV.Format							= DXGI_FORMAT_R8G8B8A8_UNORM;
	descSRV.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURE2D;
	descSRV.Shader4ComponentMapping			= D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0,0,0,0);
	descSRV.Texture2D.MostDetailedMip		= 0;
	descSRV.Texture2D.MipLevels				= 1;
	descSRV.Texture2D.PlaneSlice			= 0;
	descSRV.Texture2D.ResourceMinLODClamp	= 1.0f;

	g_hTexure = g_pDescripterHeapArray[DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetCPUDescriptorHandleForHeapStart();

	// テクスチャの生データを書き込み
	int x, y, comp;
	stbi_uc* pixels = stbi_load("../resource/Ok-icon.png", &x, &y, &comp, 0);

	D3D12_RESOURCE_DESC descResource;

	descResource.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE_2D;
	descResource.Alignment = 0;
	descResource.Width = x;
	descResource.Height = y;
	descResource.DepthOrArraySize = 2;
	descResource.MipLevels = 0;
	descResource.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	descResource.SampleDesc = {1, 0};
	descResource.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	descResource.MiscFlags = D3D12_RESOURCE_MISC_ALLOW_UNORDERED_ACCESS;

	D3D12_CLEAR_VALUE* clearColor = new D3D12_CLEAR_VALUE[x * y];
	for (int i = 0; i < y; ++i)
	{
		for (int j = 0; j < x; ++j) 
		{
			D3D12_CLEAR_VALUE& color = clearColor[i * y + j];
			color.Color[0] = static_cast<float>(pixels[i * y + j * comp + 0]) / 255.f;
			color.Color[1] = static_cast<float>(pixels[i * y + j * comp + 1]) / 255.f;
			color.Color[2] = static_cast<float>(pixels[i * y + j * comp + 2]) / 255.f;
			color.Color[3] = static_cast<float>(pixels[i * y + j * comp + 3]) / 255.f;
			color.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		}
	}

	g_pDevice->CreateCommittedResource(
		&CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_MISC_SHARED,
		&descResource,
		D3D12_RESOURCE_USAGE_PIXEL_SHADER_RESOURCE,
		nullptr,//clearColor,
		IID_PPV_ARGS(&g_pTextureResource));

	g_pDevice->CreateShaderResourceView(g_pTextureResource, nullptr, g_hTexure);
	g_pTextureResource->SetName(TEXT("テクスチャ"));

	delete [] clearColor;
	stbi_image_free(pixels);

#endif
	return true;
}
#endif

#if defined(RESOURCE_SETUP)
// リソースクリーンアップ
void cleanupResource()
{
	safeRelease(g_pVertexBufferResource);
}
#endif

// リソース設定時のバリア関数
void setResourceBarrier(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter)
{
	D3D12_RESOURCE_BARRIER descBarrier = {};
	ZeroMemory(&descBarrier, sizeof(descBarrier));

	descBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	descBarrier.Transition.pResource = resource;
	descBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	descBarrier.Transition.StateBefore = StateBefore;
	descBarrier.Transition.StateAfter = StateAfter;

	commandList->ResourceBarrier(1, &descBarrier);
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
	pCommand->OMSetRenderTargets(1, &g_hBackBuffer, TRUE, nullptr);

	// 矩形を設定
	D3D12_RECT clearRect;
	clearRect.left = 0;
	clearRect.top = 0;
	clearRect.right = SCREEN_WIDTH;
	clearRect.bottom = SCREEN_HEIGHT;
	pCommand->RSSetViewports(1, &g_viewPort);
	pCommand->RSSetScissorRects(1, &clearRect);

	// レンダーターゲットへリソースを設定
	setResourceBarrier(
		pCommand,
		g_pBackBufferResource,
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	// クリア
	static float count = 0;
	count = fmod(count + 0.01f, 1.0f);
	float clearColor[] = { count, 0.2f, 0.4f, 1.0f };
	pCommand->ClearRenderTargetView(g_hBackBuffer, clearColor, 1, &clearRect);

#if defined(RESOURCE_SETUP)
	// 三角形描画
	pCommand->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommand->SetVertexBuffers(0, &g_VertexBufferView, 1);
	pCommand->DrawInstanced(3, 1, 0, 0);
#endif
	
	// present前の処理
	setResourceBarrier(
		pCommand,
		g_pBackBufferResource,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);

	// 描画コマンドを実行してフリップ
	pCommand->Close();
	ID3D12CommandList* pTemp = pCommand;
	g_pCommandQueue->ExecuteCommandLists(1, &pTemp);
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

#if defined(RESOURCE_SETUP)
	if (setupResource() == false) 
	{
		return 1;
	}
#endif

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
#if defined(RESOURCE_SETUP)
	cleanupResource();
#endif
	cleanupDirectX();

	// 登録したクラスを解除
	UnregisterClass(CLASS_NAME, hInstance);

	return 0;
}