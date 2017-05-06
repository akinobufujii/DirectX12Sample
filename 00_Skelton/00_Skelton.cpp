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

// �f�B�X�N���v�^�q�[�v�^�C�v
enum DESCRIPTOR_HEAP_TYPE
{
	DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,	// UAV�ȂǗp
	DESCRIPTOR_HEAP_TYPE_SAMPLER,		// �T���v���p
	DESCRIPTOR_HEAP_TYPE_RTV,			// �����_�[�^�[�Q�b�g�p	
	DESCRIPTOR_HEAP_TYPE_DSV,			// �f�v�X�X�e���V���p	
	DESCRIPTOR_HEAP_TYPE_MAX,			// �f�B�X�N���v�^�q�[�v��
	DESCRIPTOR_HEAP_TYPE_SET = DESCRIPTOR_HEAP_TYPE_SAMPLER + 1,
};

//static const UINT	COMMAND_ALLOCATOR_MAX = 2;	// �R�}���h�A���P�[�^��

//==============================================================================
// �O���[�o���ϐ�
//==============================================================================
#if _DEBUG
ID3D12Debug*				g_pDebug;											// �f�o�b�O�I�u�W�F�N�g
#endif

ID3D12Device*				g_pDevice;											// �f�o�C�X
ID3D12CommandAllocator*		g_pCommandAllocator;								// �R�}���h�A���P�[�^
ID3D12CommandQueue*			g_pCommandQueue;									// �R�}���h�L���[
IDXGIFactory2*				g_pGIFactory;										// GI�t�@�N�g���[
IDXGISwapChain3*			g_pGISwapChain;										// GI�X���b�v�`�F�[��
ID3D12DescriptorHeap*		g_pDescripterHeapArray[DESCRIPTOR_HEAP_TYPE_MAX];	// �f�B�X�N���v�^�q�[�v�z��
ID3D12GraphicsCommandList*	g_pGraphicsCommandList;								// �`��R�}���h���X�g
D3D12_VIEWPORT				g_viewPort;											// �r���[�|�[�g
ID3D12Fence*				g_pFence;											// �t�F���X�I�u�W�F�N�g
HANDLE						g_hFenceEvent;										// �t�F���X�C�x���g�n���h��
UINT64						g_CurrentFenceIndex = 0;							// ���݂̃t�F���X�C���f�b�N�X
UINT						g_CurrentBuckBufferIndex = 0;						// ���݂̃o�b�N�o�b�t�@
ID3D12Resource*				g_pBackBufferResource[BACKBUFFER_COUNT];			// �o�b�N�o�b�t�@�̃��\�[�X
D3D12_CPU_DESCRIPTOR_HANDLE	g_hBackBuffer[BACKBUFFER_COUNT];					// �o�b�N�o�b�t�@�n���h��

// �t���[���҂�
void waitForPreviousFrame()
{
	const UINT64 FENCE_INDEX = g_CurrentFenceIndex;
	g_pCommandQueue->Signal(g_pFence, FENCE_INDEX);
	g_CurrentFenceIndex++;

	if(g_pFence->GetCompletedValue() < FENCE_INDEX)
	{
		g_pFence->SetEventOnCompletion(FENCE_INDEX, g_hFenceEvent);
		WaitForSingleObject(g_hFenceEvent, INFINITE);
	}
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
	UINT GIFlag = 0;
#if _DEBUG
	// �f�o�b�O���C���[�쐬
	hr = D3D12GetDebugInterface(IID_PPV_ARGS(&g_pDebug));
	if(SUCCEEDED(hr) && g_pDebug)
	{
		g_pDebug->EnableDebugLayer();
		GIFlag = DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	// GI�t�@�N�g���l��
	// �f�o�b�O���[�h�̃t�@�N�g���쐬
	hr = CreateDXGIFactory2(GIFlag, IID_PPV_ARGS(&g_pGIFactory));
	if(showErrorMessage(hr, TEXT("GI�t�@�N�g���l�����s")))
	{
		return false;
	}

	IDXGIAdapter* pGIAdapter = nullptr;
	hr = g_pGIFactory->EnumAdapters(0, &pGIAdapter);
	if(showErrorMessage(hr, TEXT("GI�A�_�v�^�[�l�����s")))
	{
		return false;
	}

	// �f�o�C�X�쐬
	hr = D3D12CreateDevice(
		pGIAdapter,
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&g_pDevice));

	safeRelease(pGIAdapter);

	if(showErrorMessage(hr, TEXT("�f�o�C�X�쐬���s")))
	{
		return false;
	}

	// �R�}���h�L���[�쐬
	// �h�L�������g�ɂ���ȉ��̂悤�ȃ��\�b�h�͂Ȃ�����
	//g_pDevice->GetDefaultCommandQueue(&g_pCommandQueue);
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
	// ������DirectX11�Ƃ��܂�ς��Ȃ�
	// �Q�lURL:https://msdn.microsoft.com/en-us/library/dn859356(v=vs.85).aspx
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
	hr = g_pGIFactory->CreateSwapChainForHwnd(
		g_pCommandQueue,
		hWnd,
		&descSwapChain,
		nullptr,
		nullptr,
		reinterpret_cast<IDXGISwapChain1**>(&g_pGISwapChain));
	if(showErrorMessage(hr, TEXT("�X���b�v�`�F�[���쐬���s")))
	{
		return false;
	}

	// ALT+ENTER�Ńt���X�N���[�������Ȃ��悤�ɂ���
	hr = g_pGIFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);
	if(showErrorMessage(hr, TEXT("DXGI�Ƀ��b�Z�[�W�L���[�̊Ď�����^����̂Ɏ��s")))
	{
		return false;
	}

	// �o�b�N�o�b�t�@�̃C���f�b�N�X���l��
	g_CurrentBuckBufferIndex = g_pGISwapChain->GetCurrentBackBufferIndex();

	// �R�}���h�A���P�[�^�쐬
	hr = g_pDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&g_pCommandAllocator));

	if(showErrorMessage(hr, TEXT("�R�}���h�A���P�[�^�쐬���s")))
	{
		hr = g_pDevice->GetDeviceRemovedReason();
		return false;
	}

	// �`��R�}���h���X�g�쐬
	hr = g_pDevice->CreateCommandList(
		0x00000000,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		g_pCommandAllocator,
		nullptr,
		IID_PPV_ARGS(&g_pGraphicsCommandList));

	if(showErrorMessage(hr, TEXT("�R�}���h���C�����X�g�쐬���s")))
	{
		return false;
	}

	// �f�B�X�N���v�^�q�[�v�쐬
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	ZeroMemory(&heapDesc, sizeof(heapDesc));

	heapDesc.NumDescriptors = BACKBUFFER_COUNT;

	for(int i = 0; i < DESCRIPTOR_HEAP_TYPE_MAX; ++i)
	{
		heapDesc.Flags = (i == D3D12_DESCRIPTOR_HEAP_TYPE_RTV || i == D3D12_DESCRIPTOR_HEAP_TYPE_DSV) ? D3D12_DESCRIPTOR_HEAP_FLAG_NONE : D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.Type = static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i);

		hr = g_pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&g_pDescripterHeapArray[i]));
		if(showErrorMessage(hr, TEXT("�f�B�X�N���v�^�q�[�v�쐬���s")))
		{
			return false;
		}
	}

	// �f�B�X�N���v�^�q�[�v�ƃR�}���h���X�g�̊֘A�Â�
	g_pGraphicsCommandList->SetDescriptorHeaps(DESCRIPTOR_HEAP_TYPE_SET, g_pDescripterHeapArray);

	// �����_�[�^�[�Q�b�g�r���[(�o�b�N�o�b�t�@)���쐬
	UINT strideHandleBytes = g_pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	for(UINT i = 0; i < BACKBUFFER_COUNT; ++i)
	{
		hr = g_pGISwapChain->GetBuffer(i, IID_PPV_ARGS(&g_pBackBufferResource[i]));
		if(showErrorMessage(hr, TEXT("�����_�[�^�[�Q�b�g�r���[�쐬���s")))
		{
			return false;
		}
		g_hBackBuffer[i] = g_pDescripterHeapArray[DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart();
		g_hBackBuffer[i].ptr += i * strideHandleBytes;	// �����_�[�^�[�Q�b�g�̃I�t�Z�b�g���|�C���^�����炷
		g_pDevice->CreateRenderTargetView(g_pBackBufferResource[i], nullptr, g_hBackBuffer[i]);
	}

	// �r���[�|�[�g�ݒ�
	g_viewPort.TopLeftX = 0;			// X���W
	g_viewPort.TopLeftY = 0;			// Y���W
	g_viewPort.Width = SCREEN_WIDTH;	// ��
	g_viewPort.Height = SCREEN_HEIGHT;	// ����
	g_viewPort.MinDepth = 0.0f;			// �ŏ��[�x
	g_viewPort.MaxDepth = 1.0f;			// �ő�[�x

	// �t�F���X�I�u�W�F�N�g�쐬
	g_pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_pFence));

	if(showErrorMessage(hr, TEXT("�t�F���X�I�u�W�F�N�g�쐬���s")))
	{
		return false;
	}

	// �t�F���X�C�x���g�n���h���쐬
	g_hFenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

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
	for(int i = 0; i < DESCRIPTOR_HEAP_TYPE_MAX; ++i)
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

// �`��
void Render()
{
	// ���݂̃R�}���h���X�g���l��
	ID3D12GraphicsCommandList* pCommand = g_pGraphicsCommandList;

	// ��`��ݒ�
	D3D12_RECT clearRect;
	clearRect.left = 0;
	clearRect.top = 0;
	clearRect.right = SCREEN_WIDTH;
	clearRect.bottom = SCREEN_HEIGHT;

	// �o�b�N�o�b�t�@�̎Q�Ɛ��ύX
	g_CurrentBuckBufferIndex = g_pGISwapChain->GetCurrentBackBufferIndex();

	// �����_�[�^�[�Q�b�g�r���[��ݒ�
	pCommand->OMSetRenderTargets(1, &g_hBackBuffer[g_CurrentBuckBufferIndex], TRUE, nullptr);

	// �����_�[�^�[�Q�b�g�փ��\�[�X��ݒ�
	setResourceBarrier(
		pCommand,
		g_pBackBufferResource[g_CurrentBuckBufferIndex],
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);

	// �N���A
	static float count = 0;
	count = fmod(count + 0.01f, 1.0f);
	float clearColor[] = { count, 0.2f, 0.4f, 1.0f };
	pCommand->RSSetViewports(1, &g_viewPort);
	pCommand->RSSetScissorRects(1, &clearRect);
	pCommand->ClearRenderTargetView(g_hBackBuffer[g_CurrentBuckBufferIndex], clearColor, 1, &clearRect);

	// present�O�̏���
	setResourceBarrier(
		pCommand,
		g_pBackBufferResource[g_CurrentBuckBufferIndex],
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT);

	// �`��R�}���h�����s���ăt���b�v
	pCommand->Close();
	ID3D12CommandList* pTemp = pCommand;
	g_pCommandQueue->ExecuteCommandLists(1, &pTemp);
	g_pGISwapChain->Present(1, 0);

	// �R�}���h�L���[�̏�����҂�
	waitForPreviousFrame();

	// �R�}���h�A���P�[�^�ƃR�}���h���X�g�����Z�b�g
	g_pCommandAllocator->Reset();
	pCommand->Reset(g_pCommandAllocator, nullptr);
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
	do {
		if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// ���C��
			Render();
		}
	} while(msg.message != WM_QUIT);

	// �������
	cleanupDirectX();

	// �o�^�����N���X������
	UnregisterClass(CLASS_NAME, hInstance);

	return 0;
}