//==============================================================================
// �C���N���[�h
//==============================================================================
#include "../libs/akilib/include/D3D12Helper.h"

#include <Dxgi1_4.h>
#include <D3d12SDKLayers.h>

//==============================================================================
// �萔
//==============================================================================
static const int SCREEN_WIDTH = 1280;					// ��ʕ�
static const int SCREEN_HEIGHT = 720;					// ��ʍ���
static const LPTSTR	CLASS_NAME = TEXT("00_Skelton");	// �E�B���h�E�l�[��
static const UINT BACKBUFFER_COUNT = 2;					// �o�b�N�o�b�t�@��

//static const UINT	COMMAND_ALLOCATOR_MAX = 2;	// �R�}���h�A���P�[�^��

//==============================================================================
// �O���[�o���ϐ�
//==============================================================================
#if _DEBUG
ID3D12Debug*				g_pDebug;											// �f�o�b�O�I�u�W�F�N�g
#endif
IDXGIFactory4*				g_pDXGIFactory;										// GI�t�@�N�g���[
ID3D12Device*				g_pDevice;											// �f�o�C�X
ID3D12CommandQueue*			g_pCommandQueue;									// �R�}���h�L���[
IDXGISwapChain3*			g_pDXGISwapChain;									// GI�X���b�v�`�F�[��
ID3D12DescriptorHeap*		g_pRenderTargetViewHeap;							// �����_�[�^�[�Q�b�g�r���[�p�f�B�X�N���v�^�q�[�v
ID3D12CommandAllocator*		g_pCommandAllocator;								// �R�}���h�A���P�[�^
ID3D12Resource*				g_pBackBufferResource[BACKBUFFER_COUNT];			// �o�b�N�o�b�t�@�̃��\�[�X
ID3D12GraphicsCommandList*	g_pGraphicsCommandList;								// �`��R�}���h���X�g
ID3D12Fence*				g_pFence;											// �t�F���X�I�u�W�F�N�g
HANDLE						g_hFenceEvent;										// �t�F���X�C�x���g�n���h��
D3D12_VIEWPORT				g_viewPort;											// �r���[�|�[�g

UINT						g_currentBuckBufferIndex = 0;						// ���݂̃o�b�N�o�b�t�@
UINT						g_renderTargetViewHeapSize = 0;						// �����_�[�^�[�Q�b�g�r���[�̃q�[�v�T�C�Y
UINT64						g_currentFenceIndex = 0ULL;							// ���݂̃t�F���X�C���f�b�N�X

// �t���[���҂�
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

	// �o�b�N�o�b�t�@�̎Q�Ɛ��ύX
	g_currentBuckBufferIndex = g_pDXGISwapChain->GetCurrentBackBufferIndex();
}

// �p�C�v���C���̏�����
bool initPipeline()
{
	return true;
}

// DirectX������
bool initDirectX(HWND hWnd)
{
	HRESULT hr;
	UINT dxgiFlag = 0;
#if _DEBUG
	// �f�o�b�O���C���[�쐬
	hr = D3D12GetDebugInterface(IID_PPV_ARGS(&g_pDebug));
	if(SUCCEEDED(hr) && g_pDebug)
	{
		g_pDebug->EnableDebugLayer();
		dxgiFlag |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	// GI�t�@�N�g���l��
	hr = CreateDXGIFactory2(dxgiFlag, IID_PPV_ARGS(&g_pDXGIFactory));
	if(showErrorMessage(hr, TEXT("GI�t�@�N�g���l�����s")))
	{
		return false;
	}

	IDXGIAdapter1* pDXGIAdapter = nullptr;
	hr = g_pDXGIFactory->EnumAdapters1(0, &pDXGIAdapter);
	if(showErrorMessage(hr, TEXT("GI�A�_�v�^�[�l�����s")))
	{
		return false;
	}

	// �f�o�C�X�쐬
	hr = D3D12CreateDevice(
		pDXGIAdapter,
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&g_pDevice));

	safeRelease(pDXGIAdapter);

	if(showErrorMessage(hr, TEXT("�f�o�C�X�쐬���s")))
	{
		return false;
	}

	// �R�}���h�L���[�쐬
	D3D12_COMMAND_QUEUE_DESC descCommandQueue = {};
	descCommandQueue.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;	// �R�}���h���X�g�^�C�v
	descCommandQueue.Priority = 0;							// �D��x
	descCommandQueue.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;	// �t���O
	descCommandQueue.NodeMask = 0x00000000;					// �m�[�h�}�X�N

	hr = g_pDevice->CreateCommandQueue(&descCommandQueue, IID_PPV_ARGS(&g_pCommandQueue));
	if(showErrorMessage(hr, TEXT("�R�}���h�L���[�쐬���s")))
	{
		return false;
	}

	// �X���b�v�`�F�[�����쐬
	DXGI_SWAP_CHAIN_DESC1 descSwapChain = {};
	descSwapChain.BufferCount = BACKBUFFER_COUNT;					// �o�b�N�o�b�t�@��2���ȏ�Ȃ��Ǝ��s����
	descSwapChain.Width = SCREEN_WIDTH;
	descSwapChain.Height = SCREEN_HEIGHT;
	descSwapChain.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	descSwapChain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	descSwapChain.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	descSwapChain.SampleDesc.Count = 1;

	// �f�o�C�X����Ȃ��ăR�}���h�L���[��n��
	// �łȂ��Ǝ��s���G���[���N����
	hr = g_pDXGIFactory->CreateSwapChainForHwnd(
		g_pCommandQueue,
		hWnd,
		&descSwapChain,
		nullptr,
		nullptr,
		reinterpret_cast<IDXGISwapChain1**>(&g_pDXGISwapChain));
	if(showErrorMessage(hr, TEXT("�X���b�v�`�F�[���쐬���s")))
	{
		return false;
	}

	// ALT+ENTER�Ńt���X�N���[�������Ȃ��悤�ɂ���
	hr = g_pDXGIFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
	if(showErrorMessage(hr, TEXT("DXGI�Ƀ��b�Z�[�W�L���[�̊Ď�����^����̂Ɏ��s")))
	{
		return false;
	}

	// �o�b�N�o�b�t�@�̃C���f�b�N�X���l��
	g_currentBuckBufferIndex = g_pDXGISwapChain->GetCurrentBackBufferIndex();

	// �f�B�X�N���v�^�q�[�v�쐬
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = BACKBUFFER_COUNT;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = g_pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&g_pRenderTargetViewHeap));
	if(showErrorMessage(hr, TEXT("�f�B�X�N���v�^�q�[�v�쐬���s")))
	{
		return false;
	}

	g_renderTargetViewHeapSize = g_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// �����_�[�^�[�Q�b�g�r���[(�o�b�N�o�b�t�@)���쐬
	CD3DX12_CPU_DESCRIPTOR_HANDLE hRenderTargetView(g_pRenderTargetViewHeap->GetCPUDescriptorHandleForHeapStart());
	for(UINT i = 0; i < BACKBUFFER_COUNT; ++i)
	{
		hr = g_pDXGISwapChain->GetBuffer(i, IID_PPV_ARGS(&g_pBackBufferResource[i]));
		if(showErrorMessage(hr, TEXT("�����_�[�^�[�Q�b�g�r���[�쐬���s")))
		{
			return false;
		}
		g_pDevice->CreateRenderTargetView(g_pBackBufferResource[i], nullptr, hRenderTargetView);
		hRenderTargetView.Offset(1, g_renderTargetViewHeapSize);
	}

	// �R�}���h�A���P�[�^�쐬
	hr = g_pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_pCommandAllocator));
	if(showErrorMessage(hr, TEXT("�R�}���h�A���P�[�^�쐬���s")))
	{
		hr = g_pDevice->GetDeviceRemovedReason();
		return false;
	}

	// �`��R�}���h���X�g�쐬
	hr = g_pDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		g_pCommandAllocator,
		nullptr,
		IID_PPV_ARGS(&g_pGraphicsCommandList));
	if(showErrorMessage(hr, TEXT("�R�}���h���C�����X�g�쐬���s")))
	{
		return false;
	}

	// �R�}���h���X�g�͋L����Ԃō쐬����邪�A�܂��R�}���h�L���i�K�ł͂Ȃ��̂ŕ��Ă���
	hr = g_pGraphicsCommandList->Close();
	if(showErrorMessage(hr, TEXT("�R�}���h���C�����X�g�N���[�Y���s")))
	{
		return false;
	}

	// �t�F���X�I�u�W�F�N�g�쐬
	g_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_pFence));

	if(showErrorMessage(hr, TEXT("�t�F���X�I�u�W�F�N�g�쐬���s")))
	{
		return false;
	}
	g_currentFenceIndex = 1;

	// �t�F���X�C�x���g�n���h���쐬
	g_hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	// �r���[�|�[�g�ݒ�
	g_viewPort.TopLeftX = 0;			// X���W
	g_viewPort.TopLeftY = 0;			// Y���W
	g_viewPort.Width = SCREEN_WIDTH;	// ��
	g_viewPort.Height = SCREEN_HEIGHT;	// ����
	g_viewPort.MinDepth = 0.0f;			// �ŏ��[�x
	g_viewPort.MaxDepth = 1.0f;			// �ő�[�x

	// �R�}���h�L���[�̏�����҂�
	waitForPreviousFrame();

	return true;
}

// DirectX12�N���[���A�b�v
void cleanupDirectX()
{
	// �R�}���h�L���[�̏�����҂�
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

// �X�V
void updateFrame()
{
}

// �`��
void renderFrame()
{
	// �R�}���h���X�g�A���P�[�^�́A�֘A�t����ꂽ�R�}���h���X�g��GPU�ł̎��s���I�������Ƃ��ɂ̂݃��Z�b�g�ł���
	// �A�v�����̓t�F���X���g����GPU���s�󋵂𔻒f����K�v������
	g_pCommandAllocator->Reset();

	// ���݂̃R�}���h���X�g���l��
	ID3D12GraphicsCommandList* pCommand = g_pGraphicsCommandList;

	// �R�}���h���X�g�����Z�b�g
	pCommand->Reset(g_pCommandAllocator, nullptr);

	// �o�b�N�o�b�t�@�������_�[�^�[�Q�b�g�Ƃ��Ďg�p����邩������Ȃ��̂ŁA�o���A�𒣂�
	pCommand->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			g_pBackBufferResource[g_currentBuckBufferIndex],
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET)
	);

	// ��ʂ��N���A
	CD3DX12_CPU_DESCRIPTOR_HANDLE hRenderTargetView(
		g_pRenderTargetViewHeap->GetCPUDescriptorHandleForHeapStart(),
		g_currentBuckBufferIndex,
		g_renderTargetViewHeapSize);

	// ��`����nullptr�œn���Ǝw�背���_�[�^�[�Q�b�g�̑S��ʂƂȂ�
	static float count = 0;
	count = fmod(count + 0.01f, 1.0f);
	float clearColor[] = {count, 0.2f, 0.4f, 1.0f};
	pCommand->ClearRenderTargetView(hRenderTargetView, clearColor, 0, nullptr);

	// Indicate that the back buffer will now be used to present.
	pCommand->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			g_pBackBufferResource[g_currentBuckBufferIndex],
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT));

	// �R�}���h�L���I��
	pCommand->Close();

	// �`��R�}���h�����s���ăt���b�v
	ID3D12CommandList* pCommandListArray[] = { pCommand };
	g_pCommandQueue->ExecuteCommandLists(_countof(pCommandListArray), pCommandListArray);
	g_pDXGISwapChain->Present(1, 0);

	// �R�}���h�L���[�̏�����҂�
	waitForPreviousFrame();	
}

// �E�B���h�E�v���V�[�W��
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// ���b�Z�[�W����
	switch(msg)
	{
	case WM_KEYDOWN:	// �L�[�������ꂽ��
		switch(wparam)
		{
		case VK_ESCAPE:
			DestroyWindow(hwnd);
			break;
		}
		break;

	case WM_DESTROY:	// �E�B���h�E�j��
		PostQuitMessage(0);
		break;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

// �G���g���[�|�C���g
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	HWND hwnd;
	MSG msg;
	WNDCLASS winc;

	winc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	winc.lpfnWndProc = WndProc;					// �E�B���h�E�v���V�[�W��
	winc.cbClsExtra = 0;
	winc.cbWndExtra = 0;
	winc.hInstance = hInstance;
	winc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	winc.hCursor = LoadCursor(NULL, IDC_ARROW);
	winc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	winc.lpszMenuName = NULL;
	winc.lpszClassName = CLASS_NAME;

	// �E�B���h�E�N���X�o�^
	if(RegisterClass(&winc) == false)
	{
		return 1;
	}

	// �E�B���h�E�쐬
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

	// DirectX������
	if(initDirectX(hwnd) == false)
	{
		return 1;
	}

	// �E�B���h�E�\��
	ShowWindow(hwnd, nCmdShow);

	// ���b�Z�[�W���[�v
	do
	{
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// ���C������
			updateFrame();
			renderFrame();
		}
	}
	while(msg.message != WM_QUIT);

	// �������
	cleanupDirectX();

	// �o�^�����N���X������
	UnregisterClass(CLASS_NAME, hInstance);

	return 0;
}
