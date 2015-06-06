//==============================================================================
// インクルード
//==============================================================================
#include "../libs/akilib/include/D3D12Helper.h"

/*
ドキュメントにはあるって書いているのにない拡張ライブラリ
https://msdn.microsoft.com/en-us/library/dn708058(v=vs.85).aspx
#include <d3dx12.h>
*/
#include <Dxgi1_4.h>
#include <D3d12SDKLayers.h>

//==============================================================================
// 定数
//==============================================================================
static const int SCREEN_WIDTH = 1280;					// 画面幅
static const int SCREEN_HEIGHT = 720;					// 画面高さ
static const LPTSTR	CLASS_NAME = TEXT("00_Skelton");	// ウィンドウネーム
static const UINT BACKBUFFER_COUNT = 2;					// バックバッファ数

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
#if _DEBUG
ID3D12Debug*				g_pDebug;											// デバッグオブジェクト
#endif

ID3D12Device*				g_pDevice;											// デバイス
ID3D12CommandAllocator*		g_pCommandAllocator;								// コマンドアロケータ
ID3D12CommandQueue*			g_pCommandQueue;									// コマンドキュー
IDXGIFactory2*				g_pGIFactory;										// GIファクトリー
IDXGISwapChain3*			g_pGISwapChain;										// GIスワップチェーン
ID3D12DescriptorHeap*		g_pDescripterHeapArray[DESCRIPTOR_HEAP_TYPE_MAX];	// ディスクリプタヒープ配列
ID3D12GraphicsCommandList*	g_pGraphicsCommandList;								// 描画コマンドリスト
D3D12_VIEWPORT				g_viewPort;											// ビューポート
ID3D12Fence*				g_pFence;											// フェンスオブジェクト
HANDLE						g_hFenceEvent;										// フェンスイベントハンドル
UINT64						g_CurrentFenceIndex = 0;							// 現在のフェンスインデックス
UINT						g_CurrentBuckBufferIndex = 0;						// 現在のバックバッファ
ID3D12Resource*				g_pBackBufferResource[BACKBUFFER_COUNT];			// バックバッファのリソース
D3D12_CPU_DESCRIPTOR_HANDLE	g_hBackBuffer[BACKBUFFER_COUNT];					// バックバッファハンドル

// DirectX初期化
bool initDirectX(HWND hWnd)
{
	HRESULT hr;

#if _DEBUG
	// デバッグレイヤー作成
	hr = D3D12GetDebugInterface(IID_PPV_ARGS(&g_pDebug));
	if (showErrorMessage(hr, TEXT("デバッグレイヤー作成失敗")))
	{
		return false;
	}
	if (g_pDebug)
	{
		g_pDebug->EnableDebugLayer();
	}
#endif

	// GIファクトリ獲得
	// デバッグモードのファクトリ作成
	hr = CreateDXGIFactory2((_DEBUG)? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(&g_pGIFactory));
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

	safeRelease(pGIAdapter);

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
	descSwapChain.BufferCount = BACKBUFFER_COUNT;					// バックバッファは2枚以上ないと失敗する
	descSwapChain.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	descSwapChain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	descSwapChain.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	descSwapChain.OutputWindow = hWnd;
	descSwapChain.SampleDesc.Count = 1;
	descSwapChain.Windowed = TRUE;

	// デバイスじゃなくてコマンドキューを渡す
	// でないと実行時エラーが起こる
	hr = g_pGIFactory->CreateSwapChain(g_pCommandQueue, &descSwapChain, reinterpret_cast<IDXGISwapChain**>(&g_pGISwapChain));
	if (showErrorMessage(hr, TEXT("スワップチェーン作成失敗")))
	{
		return false;
	}

	// 描画コマンドリスト作成
	hr = g_pDevice->CreateCommandList(
		0x00000000,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		g_pCommandAllocator,
		nullptr,
		IID_PPV_ARGS(&g_pGraphicsCommandList));

	if (showErrorMessage(hr, TEXT("コマンドラインリスト作成失敗")))
	{
		return false;
	}

	// ディスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	ZeroMemory(&heapDesc, sizeof(heapDesc));

	heapDesc.NumDescriptors = BACKBUFFER_COUNT;

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

	// レンダーターゲットビュー(バックバッファ)を作成
	UINT strideHandleBytes = g_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for (UINT i = 0; i < BACKBUFFER_COUNT; ++i) 
	{
		hr = g_pGISwapChain->GetBuffer(i, IID_PPV_ARGS(&g_pBackBufferResource[i]));
		if (showErrorMessage(hr, TEXT("レンダーターゲットビュー作成失敗")))
		{
			return false;
		}
		g_hBackBuffer[i] = g_pDescripterHeapArray[DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart();
		g_hBackBuffer[i].ptr += i * strideHandleBytes;	// レンダーターゲットのオフセット分ポインタをずらす
		g_pDevice->CreateRenderTargetView(g_pBackBufferResource[i], nullptr, g_hBackBuffer[i]);
	}

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
	for (UINT i = 0; i < BACKBUFFER_COUNT; ++i)
	{
		safeRelease(g_pBackBufferResource[i]);
	}
	for (int i = 0; i < DESCRIPTOR_HEAP_TYPE_MAX; ++i)
	{
		safeRelease(g_pDescripterHeapArray[i]);
	}	
	safeRelease(g_pGISwapChain);
	safeRelease(g_pGIFactory);
	safeRelease(g_pCommandQueue);
	safeRelease(g_pGraphicsCommandList);
	safeRelease(g_pCommandAllocator);
	safeRelease(g_pDevice);
#if _DEBUG
	safeRelease(g_pDebug);
#endif
}

// 描画
void Render()
{
	// 現在のコマンドリストを獲得
	ID3D12GraphicsCommandList* pCommand = g_pGraphicsCommandList;

	// 矩形を設定
	D3D12_RECT clearRect;
	clearRect.left = 0;
	clearRect.top = 0;
	clearRect.right = SCREEN_WIDTH;
	clearRect.bottom = SCREEN_HEIGHT;

	// バックバッファの参照先を変更
	g_CurrentBuckBufferIndex = g_pGISwapChain->GetCurrentBackBufferIndex();

	// レンダーターゲットビューを設定
	pCommand->OMSetRenderTargets(1, &g_hBackBuffer[g_CurrentBuckBufferIndex], TRUE, nullptr);

	// レンダーターゲットへリソースを設定
	setResourceBarrier(
		pCommand,
		g_pBackBufferResource[g_CurrentBuckBufferIndex],
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	// クリア
	static float count = 0;
	count = fmod(count + 0.01f, 1.0f);
	float clearColor[] = { count, 0.2f, 0.4f, 1.0f };
	pCommand->RSSetViewports(1, &g_viewPort);
	pCommand->RSSetScissorRects(1, &clearRect);
	pCommand->ClearRenderTargetView(g_hBackBuffer[g_CurrentBuckBufferIndex], clearColor, 1, &clearRect);
	
	// present前の処理
	setResourceBarrier(
		pCommand,
		g_pBackBufferResource[g_CurrentBuckBufferIndex],
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);

	// 描画コマンドを実行してフリップ
	pCommand->Close();
	ID3D12CommandList* pTemp = pCommand;
	g_pCommandQueue->ExecuteCommandLists(1, &pTemp);
	g_pGISwapChain->Present(1, 0);

	// コマンドキューの処理を待つ
	const UINT64 FENCE_INDEX = g_CurrentFenceIndex;
	g_pCommandQueue->Signal(g_pFence, FENCE_INDEX);
	g_CurrentFenceIndex++;

	if (g_pFence->GetCompletedValue() < FENCE_INDEX)
	{
		g_pFence->SetEventOnCompletion(FENCE_INDEX, g_hFenceEvent);
		WaitForSingleObject(g_hFenceEvent, INFINITE);
	}

	// コマンドアロケータとコマンドリストをリセット
	g_pCommandAllocator->Reset();
	pCommand->Reset(g_pCommandAllocator, nullptr);
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