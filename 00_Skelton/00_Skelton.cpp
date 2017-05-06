//==============================================================================
// インクルード
//==============================================================================
#include "../libs/akilib/include/D3D12Helper.h"

#include <Dxgi1_4.h>
#include <D3d12SDKLayers.h>

//==============================================================================
// 定数
//==============================================================================
static const int SCREEN_WIDTH = 1280;					// 画面幅
static const int SCREEN_HEIGHT = 720;					// 画面高さ
static const LPTSTR	CLASS_NAME = TEXT("00_Skelton");	// ウィンドウネーム
static const UINT BACKBUFFER_COUNT = 2;					// バックバッファ数

//==============================================================================
// グローバル変数
//==============================================================================
#if _DEBUG
ID3D12Debug*				g_pDebug;									// デバッグオブジェクト
#endif
IDXGIFactory4*				g_pDXGIFactory;								// GIファクトリー
ID3D12Device*				g_pDevice;									// デバイス
ID3D12CommandQueue*			g_pCommandQueue;							// コマンドキュー
IDXGISwapChain3*			g_pDXGISwapChain;							// GIスワップチェーン
ID3D12DescriptorHeap*		g_pRenderTargetViewHeap;					// レンダーターゲットビュー用ディスクリプタヒープ
ID3D12CommandAllocator*		g_pCommandAllocator;						// コマンドアロケータ
ID3D12Resource*				g_pBackBufferResource[BACKBUFFER_COUNT];	// バックバッファのリソース
ID3D12GraphicsCommandList*	g_pGraphicsCommandList;						// 描画コマンドリスト
ID3D12Fence*				g_pFence;									// フェンスオブジェクト
HANDLE						g_hFenceEvent;								// フェンスイベントハンドル

UINT						g_currentBuckBufferIndex = 0;				// 現在のバックバッファ
UINT						g_renderTargetViewHeapSize = 0;				// レンダーターゲットビューのヒープサイズ
UINT64						g_currentFenceIndex = 0ULL;					// 現在のフェンスインデックス

// フレーム待ち
void waitForPreviousFrame()
{
	const UINT64 FENCE_INDEX = g_currentFenceIndex;
	g_pCommandQueue->Signal(g_pFence, FENCE_INDEX);
	++g_currentFenceIndex;

	if(g_pFence->GetCompletedValue() < FENCE_INDEX)
	{
		g_pFence->SetEventOnCompletion(FENCE_INDEX, g_hFenceEvent);
		WaitForSingleObject(g_hFenceEvent, INFINITE);
	}

	// バックバッファの参照先を変更
	g_currentBuckBufferIndex = g_pDXGISwapChain->GetCurrentBackBufferIndex();
}

// DirectX初期化
bool initDirectX(HWND hWnd)
{
	HRESULT hr;
	UINT dxgiFlag = 0;
#if _DEBUG
	// デバッグレイヤー作成
	hr = D3D12GetDebugInterface(IID_PPV_ARGS(&g_pDebug));
	if(SUCCEEDED(hr) && g_pDebug)
	{
		g_pDebug->EnableDebugLayer();
		dxgiFlag |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	// GIファクトリ獲得
	hr = CreateDXGIFactory2(dxgiFlag, IID_PPV_ARGS(&g_pDXGIFactory));
	if(showErrorMessage(hr, TEXT("GIファクトリ獲得失敗")))
	{
		return false;
	}

	IDXGIAdapter1* pDXGIAdapter = nullptr;
	hr = g_pDXGIFactory->EnumAdapters1(0, &pDXGIAdapter);
	if(showErrorMessage(hr, TEXT("GIアダプター獲得失敗")))
	{
		return false;
	}

	// デバイス作成
	hr = D3D12CreateDevice(
		pDXGIAdapter,
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&g_pDevice));

	safeRelease(pDXGIAdapter);

	if(showErrorMessage(hr, TEXT("デバイス作成失敗")))
	{
		return false;
	}

	// コマンドキュー作成
	D3D12_COMMAND_QUEUE_DESC descCommandQueue = {};
	descCommandQueue.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;	// コマンドリストタイプ
	descCommandQueue.Priority = 0;							// 優先度
	descCommandQueue.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;	// フラグ
	descCommandQueue.NodeMask = 0x00000000;					// ノードマスク

	hr = g_pDevice->CreateCommandQueue(&descCommandQueue, IID_PPV_ARGS(&g_pCommandQueue));
	if(showErrorMessage(hr, TEXT("コマンドキュー作成失敗")))
	{
		return false;
	}

	// スワップチェーンを作成
	DXGI_SWAP_CHAIN_DESC1 descSwapChain = {};
	descSwapChain.BufferCount = BACKBUFFER_COUNT;					// バックバッファは2枚以上ないと失敗する
	descSwapChain.Width = SCREEN_WIDTH;
	descSwapChain.Height = SCREEN_HEIGHT;
	descSwapChain.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	descSwapChain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	descSwapChain.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	descSwapChain.SampleDesc.Count = 1;

	// デバイスじゃなくてコマンドキューを渡す
	// でないと実行時エラーが起こる
	hr = g_pDXGIFactory->CreateSwapChainForHwnd(
		g_pCommandQueue,
		hWnd,
		&descSwapChain,
		nullptr,
		nullptr,
		reinterpret_cast<IDXGISwapChain1**>(&g_pDXGISwapChain));
	if(showErrorMessage(hr, TEXT("スワップチェーン作成失敗")))
	{
		return false;
	}

	// ALT+ENTERでフルスクリーン化しないようにする
	hr = g_pDXGIFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
	if(showErrorMessage(hr, TEXT("DXGIにメッセージキューの監視許可を与えるのに失敗")))
	{
		return false;
	}

	// バックバッファのインデックスを獲得
	g_currentBuckBufferIndex = g_pDXGISwapChain->GetCurrentBackBufferIndex();

	// ディスクリプタヒープ作成
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = BACKBUFFER_COUNT;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = g_pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&g_pRenderTargetViewHeap));
	if(showErrorMessage(hr, TEXT("ディスクリプタヒープ作成失敗")))
	{
		return false;
	}

	g_renderTargetViewHeapSize = g_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// レンダーターゲットビュー(バックバッファ)を作成
	CD3DX12_CPU_DESCRIPTOR_HANDLE hRenderTargetView(g_pRenderTargetViewHeap->GetCPUDescriptorHandleForHeapStart());
	for(UINT i = 0; i < BACKBUFFER_COUNT; ++i)
	{
		hr = g_pDXGISwapChain->GetBuffer(i, IID_PPV_ARGS(&g_pBackBufferResource[i]));
		if(showErrorMessage(hr, TEXT("レンダーターゲットビュー作成失敗")))
		{
			return false;
		}
		g_pDevice->CreateRenderTargetView(g_pBackBufferResource[i], nullptr, hRenderTargetView);
		hRenderTargetView.Offset(1, g_renderTargetViewHeapSize);
	}

	// コマンドアロケータ作成
	hr = g_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_pCommandAllocator));
	if(showErrorMessage(hr, TEXT("コマンドアロケータ作成失敗")))
	{
		hr = g_pDevice->GetDeviceRemovedReason();
		return false;
	}

	// 描画コマンドリスト作成
	hr = g_pDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		g_pCommandAllocator,
		nullptr,
		IID_PPV_ARGS(&g_pGraphicsCommandList));
	if(showErrorMessage(hr, TEXT("コマンドラインリスト作成失敗")))
	{
		return false;
	}

	// コマンドリストは記憶状態で作成されるが、まだコマンド記憶段階ではないので閉じておく
	hr = g_pGraphicsCommandList->Close();
	if(showErrorMessage(hr, TEXT("コマンドラインリストクローズ失敗")))
	{
		return false;
	}

	// フェンスオブジェクト作成
	g_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_pFence));

	if(showErrorMessage(hr, TEXT("フェンスオブジェクト作成失敗")))
	{
		return false;
	}
	g_currentFenceIndex = 1;

	// フェンスイベントハンドル作成
	g_hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	return true;
}

// DirectX12クリーンアップ
void cleanupDirectX()
{
	// コマンドキューの処理を待つ
	waitForPreviousFrame();

	CloseHandle(g_hFenceEvent);
	safeRelease(g_pFence);
	for(UINT i = 0; i < BACKBUFFER_COUNT; ++i)
	{
		safeRelease(g_pBackBufferResource[i]);
	}
	safeRelease(g_pRenderTargetViewHeap);
	safeRelease(g_pDXGISwapChain);
	safeRelease(g_pDXGIFactory);
	safeRelease(g_pCommandQueue);
	safeRelease(g_pGraphicsCommandList);
	safeRelease(g_pCommandAllocator);
	safeRelease(g_pDevice);
#if _DEBUG
	safeRelease(g_pDebug);
#endif
}

// 更新
void updateFrame()
{
}

// 描画
void renderFrame()
{
	// コマンドリストアロケータは、関連付けられたコマンドリストがGPUでの実行を終了したときにのみリセットできる
	// アプリ側はフェンスを使ってGPU実行状況を判断する必要がある
	g_pCommandAllocator->Reset();

	// コマンドリストをリセット
	g_pGraphicsCommandList->Reset(g_pCommandAllocator, nullptr);

	// バックバッファがレンダーターゲットとして使用されるかもしれないので、バリアを張る
	g_pGraphicsCommandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			g_pBackBufferResource[g_currentBuckBufferIndex],
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET)
	);

	// 画面をクリア
	CD3DX12_CPU_DESCRIPTOR_HANDLE hRenderTargetView(
		g_pRenderTargetViewHeap->GetCPUDescriptorHandleForHeapStart(),
		g_currentBuckBufferIndex,
		g_renderTargetViewHeapSize);

	// 矩形情報をnullptrで渡すと指定レンダーターゲットの全画面となる
	static float count = 0;
	count = fmod(count + 0.01f, 1.0f);
	float clearColor[] = {count, 0.2f, 0.4f, 1.0f};
	g_pGraphicsCommandList->ClearRenderTargetView(hRenderTargetView, clearColor, 0, nullptr);

	// 即座にバックバッファに反映
	g_pGraphicsCommandList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			g_pBackBufferResource[g_currentBuckBufferIndex],
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT)
	);

	// コマンド記憶終了
	g_pGraphicsCommandList->Close();

	// 描画コマンドを実行してフリップ
	ID3D12CommandList* pCommandListArray[] = { g_pGraphicsCommandList };
	g_pCommandQueue->ExecuteCommandLists(_countof(pCommandListArray), pCommandListArray);
	g_pDXGISwapChain->Present(1, 0);

	// コマンドキューの処理を待つ
	waitForPreviousFrame();	
}

// ウィンドウプロシージャ
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// メッセージ分岐
	switch(msg)
	{
	case WM_KEYDOWN:	// キーが押された時
		switch(wparam)
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
	if(RegisterClass(&winc) == false)
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

	if(hwnd == NULL)
	{
		return 1;
	}

	// DirectX初期化
	if(initDirectX(hwnd) == false)
	{
		return 1;
	}

	// ウィンドウ表示
	ShowWindow(hwnd, nCmdShow);

	// メッセージループ
	do
	{
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// メイン処理
			updateFrame();
			renderFrame();
		}
	}
	while(msg.message != WM_QUIT);

	// 解放処理
	cleanupDirectX();

	// 登録したクラスを解除
	UnregisterClass(CLASS_NAME, hInstance);

	return 0;
}
