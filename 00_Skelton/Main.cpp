// �C���N���[�h
#include <tchar.h>
#include <Windows.h>

#include <d3d12.h>
#include <dxgi1_2.h>
#include <Dxgi1_3.h>
#include <D3d12SDKLayers.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "DXGI.lib")

// �萔
static const int SCREEN_WIDTH = 1280;
static const int SCREEN_HEIGHT = 720;
static const LPTSTR	CLASS_NAME = TEXT("DirectX");

enum DESCRIPTOR_HEAP_TYPE 
{
	DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,	// UAV�ȂǗp
	DESCRIPTOR_HEAP_TYPE_SAMPLER,		// �T���v���p
	DESCRIPTOR_HEAP_TYPE_RTV,			// �����_�[�^�[�Q�b�g�p	
	DESCRIPTOR_HEAP_TYPE_DSV,			// �f�v�X�X�e���V���p	
	DESCRIPTOR_HEAP_TYPE_MAX,
	DESCRIPTOR_HEAP_TYPE_SET = DESCRIPTOR_HEAP_TYPE_SAMPLER + 1,
};

// �O���[�o���ϐ�
ID3D12Device*				g_pDevice;											// �f�o�C�X
ID3D12CommandAllocator*		g_pCommandAllocator;								// �R�}���h�A���P�[�^
ID3D12CommandQueue*			g_pCommandQueue;									// �R�}���h�L���[
IDXGIDevice2*				g_pGIDevice;										// GI�f�o�C�X
IDXGIAdapter*				g_pGIAdapter;										// GI�A�_�v�^�[
IDXGIFactory2*				g_pGIFactory;										// GI�t�@�N�g���[
IDXGISwapChain*				g_pGISwapChain;										// GI�X���b�v�`�F�[��
ID3D12PipelineState*		g_pPipelineState;									// �p�C�v���C���X�e�[�g
ID3D12DescriptorHeap*		g_pDescripterHeapArray[DESCRIPTOR_HEAP_TYPE_MAX];	// �f�B�X�N���v�^�q�[�v�z��
ID3D12GraphicsCommandList*	g_pGraphicsCommandList;								// �`��R�}���h���X�g

ID3D12Resource*				g_pBackBufferResource;								// �o�b�N�o�b�t�@�̃��\�[�X
D3D12_CPU_DESCRIPTOR_HANDLE g_hBackBufferRTV;									// �o�b�N�o�b�t�@�����_�[�^�[�Q�b�g�r���[
ID3D12Resource*				g_pDepthStencilResource;							// �f�v�X�X�e���V���̃��\�[�X
D3D12_CPU_DESCRIPTOR_HANDLE g_hDepthStencilView;								// �f�v�X�X�e���V���r���[

// �G���[���b�Z�[�W
// �G���[���N��������true��Ԃ��悤�ɂ���
bool showErrorMessage(HRESULT hr, const LPTSTR text)
{
	if (FAILED(hr)) 
	{
		MessageBox(nullptr, text, TEXT("Error"), MB_OK);
		return true;
	}

	return false;
}

template<typename T>
void safeRelease(T*& object)
{
	if (object)
	{
		object->Release();
		object = nullptr;
	}
}

// DirectX������
bool initDirectX(HWND hWnd)
{
	HRESULT hr;
	
	// �f�o�C�X�쐬
	hr = D3D12CreateDevice(
		nullptr, 
		D3D_DRIVER_TYPE_WARP,
		D3D12_CREATE_DEVICE_DEBUG,	// �f�o�b�O�f�o�C�X
		D3D_FEATURE_LEVEL_11_1,
		D3D12_SDK_VERSION, 
		__uuidof(ID3D12Device), 
		reinterpret_cast<void**>(&g_pDevice));

	if(showErrorMessage(hr, TEXT("�f�o�C�X�쐬���s")))
	{
		return false;
	}

	// �R�}���h�A���P�[�^�쐬
	hr = g_pDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		__uuidof(ID3D12CommandAllocator),
		reinterpret_cast<void**>(&g_pCommandAllocator));

	if (showErrorMessage(hr, TEXT("�R�}���h�A���P�[�^�쐬���s")))
	{
		return false;
	}

	// �R�}���h�L���[�쐬
	// �h�L�������g�ɂ���ȉ��̂悤�ȃ��\�b�h�͂Ȃ�����
	//g_pDevice->GetDefaultCommandQueue(&g_pCommandQueue);
	D3D12_COMMAND_QUEUE_DESC commandQueueDesk;
	commandQueueDesk.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;	// �R�}���h���X�g�^�C�v
	commandQueueDesk.Priority = 0;							// �D��x
	commandQueueDesk.Flags = D3D12_COMMAND_QUEUE_NONE;		// �t���O
	commandQueueDesk.NodeMask = 0x00000001;					// �m�[�h�}�X�N

	hr = g_pDevice->CreateCommandQueue(
		&commandQueueDesk,
		__uuidof(ID3D12CommandQueue),
		reinterpret_cast<void**>(&g_pCommandQueue));

	if (showErrorMessage(hr, TEXT("�R�}���h�L���[�쐬���s")))
	{
		return false;
	}

	// �h�L�������g�̈ȉ��̂悤�Ȏ擾���@����
	// GI�f�o�C�X�쐬�ŃG���[���o��̂ŕύX
#if 0
	// GI�f�o�C�X�쐬
	hr = g_pDevice->QueryInterface(__uuidof(IDXGIDevice2), reinterpret_cast<void**>(&g_pGIDevice));
	CreateDXGIFactory2();
	if (showErrorMessage(hr, TEXT("GI�f�o�C�X�쐬���s"));)
	{
		return false;
	}

	// GI�A�_�v�^�l��
	hr = g_pGIDevice->GetAdapter(&g_pGIAdapter);
	if (showErrorMessage(hr, TEXT("GI�A�_�v�^�l�����s")))
	{
		return false;
	}

	// GI�t�@�N�g���l��
	hr = g_pGIAdapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&g_pGIFactory));
	if (showErrorMessage(hr, TEXT("GI�t�@�N�g���l�����s"));)
	{
		return false;
	}
#else
	// GI�t�@�N�g���l��
	hr = CreateDXGIFactory2(
		DXGI_CREATE_FACTORY_DEBUG,					// �f�o�b�O���[�h�̃t�@�N�g���쐬
		__uuidof(IDXGIFactory2),
		reinterpret_cast<void**>(&g_pGIFactory));
	if (showErrorMessage(hr, TEXT("GI�t�@�N�g���l�����s")))
	{
		return false;
	}
#endif

	// �X���b�v�`�F�[�����쐬
	// ������DirectX11�Ƃ��܂�ς��Ȃ�
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
	if (showErrorMessage(hr, TEXT("�X���b�v�`�F�[���쐬���s")))
	{
		return false;
	}

#if 0
	// �p�C�v���C���X�e�[�g�쐬
	// �̑O�ɂȂ񂩂��낢��p�ӂ��Ȃ��Ƃ����Ȃ��E�E�E

	// ���[�g�V�O�j�`���̍쐬


	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(psoDesc));

	psoDesc.pRootSignature;	// ���[�g�V�O�j�`��
	psoDesc.NumRenderTargets = 1;

	hr = g_pDevice->CreateGraphicsPipelineState(
		&psoDesc,
		__uuidof(ID3D12PipelineState),
		reinterpret_cast<void**>(&g_pPipelineState));
	if (showErrorMessage(hr, TEXT("�p�C�v���C���X�e�[�g�쐬���s")))
	{
		return false;
	}

#endif

	// �`��R�}���h���X�g�쐬
	hr = g_pDevice->CreateCommandList(
		commandQueueDesk.NodeMask,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		g_pCommandAllocator,
		g_pPipelineState,
		__uuidof(ID3D12GraphicsCommandList),
		reinterpret_cast<void**>(&g_pGraphicsCommandList));

	if (showErrorMessage(hr, TEXT("�R�}���h���C�����X�g�쐬���s")))
	{
		return false;
	}

	// �f�B�X�N���v�^�q�[�v�쐬
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
		if (showErrorMessage(hr, TEXT("�L�q�q�q�[�v�쐬���s")))
		{
			return false;
		}
	}

	// �f�B�X�N���v�^�q�[�v�ƃR�}���h���X�g�̊֘A�Â�
	g_pGraphicsCommandList->SetDescriptorHeaps(g_pDescripterHeapArray, DESCRIPTOR_HEAP_TYPE_SET);

	// �����_�[�^�[�Q�b�g�r���[���쐬
	hr = g_pGISwapChain->GetBuffer(0, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&g_pBackBufferResource));
	if (showErrorMessage(hr, TEXT("�����_�[�^�[�Q�b�g�r���[�쐬���s")))
	{
		return false;
	}
	g_hBackBufferRTV = g_pDescripterHeapArray[DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart();
	g_pDevice->CreateRenderTargetView(g_pBackBufferResource, nullptr, g_hBackBufferRTV);

#if 0
	// �[�x�X�e���V���r���[�쐬
	hr = g_pGISwapChain->GetBuffer(0, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&g_pDepthStencilResource));
	if (showErrorMessage(hr, TEXT("�[�x�X�e���V���r���[�쐬���s")))
	{
		return false;
	}
	g_hDepthStencilView = g_pDescripterHeapArray[DESCRIPTOR_HEAP_TYPE_DSV]->GetCPUDescriptorHandleForHeapStart();


	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

	depthStencilDesc.Format				= DXGI_FORMAT_D32_FLOAT;			// �t�H�[�}�b�g
	depthStencilDesc.ViewDimension		= D3D12_DSV_DIMENSION_TEXTURE2D;	// ���\�[�X�A�N�Z�X���@
	depthStencilDesc.Flags				= D3D12_DSV_NONE;					// �A�N�Z�X�t���O
	depthStencilDesc.Texture2D.MipSlice = 0;								// �g�p����ŏ��̃~�b�v�}�b�v

	g_pDevice->CreateDepthStencilView(g_pDepthStencilResource, &depthStencilDesc, g_hDepthStencilView);
#endif

	// �����_�[�^�[�Q�b�g�r���[��ݒ�
	//g_pGraphicsCommandList->SetRenderTargets(&g_hBackBufferRTV, TRUE, 1, &g_hDepthStencilView);
	g_pGraphicsCommandList->SetRenderTargets(&g_hBackBufferRTV, TRUE, 1, nullptr);

	// �u�����h�X�e�[�g�ݒ�
	D3D12_BLEND_DESC blendDesc;
	blendDesc.RenderTarget[0].BlendEnable = true;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//g_pDevice->GetCopyableLayout

	return true;
}

// DirectX12�N���[���A�b�v
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


// �`��
void Render()
{
#if 1
	// �r���[�|�[�g�ݒ�
	D3D12_VIEWPORT vp;
	vp.TopLeftX = 0;			// X���W
	vp.TopLeftY = 0;			// Y���W
	vp.Width = SCREEN_WIDTH;	// ��
	vp.Height = SCREEN_HEIGHT;	// ����
	vp.MinDepth = 0.0f;			// �ŏ��[�x
	vp.MaxDepth = 1.0f;			// �ő�[�x
	g_pGraphicsCommandList->RSSetViewports(1, &vp);

	// Indicate that this resource will be in use as a render target.
	//setResourceBarrier(mCommandList, mRenderTarget, D3D12_RESOURCE_USAGE_PRESENT, D3D12_RESOURCE_USAGE_RENDER_TARGET);

	// �N���A.
	float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	CD3D12_RECT clearRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	// �����_�[�^�[�Q�b�g�r���[��ݒ�
	g_pGraphicsCommandList->SetRenderTargets(&g_hBackBufferRTV, TRUE, 1, nullptr);
	g_pGraphicsCommandList->ClearRenderTargetView(g_hBackBufferRTV, clearColor, &clearRect, 1);

	// Indicate that the render target will now be used to present when the command list is done executing.
	setResourceBarrier(g_pGraphicsCommandList, g_pBackBufferResource, D3D12_RESOURCE_USAGE_RENDER_TARGET, D3D12_RESOURCE_USAGE_PRESENT);

	// Execute the command list.
	g_pGraphicsCommandList->Close();

	ID3D12CommandList* pTemp = g_pGraphicsCommandList;
	g_pCommandQueue->ExecuteCommandLists(1, &pTemp);

	// �t���b�v
	g_pGISwapChain->Present(1, 0);
	
	g_pCommandAllocator->Reset();
	g_pGraphicsCommandList->Reset(g_pCommandAllocator, g_pPipelineState);
#endif
}

// �E�B���h�E�v���V�[�W��
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// ���b�Z�[�W����
	switch (msg)
	{
	case WM_KEYDOWN:	// �L�[�������ꂽ��
		switch (wparam)
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
	if (RegisterClass(&winc) == false)
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

	if (hwnd == NULL)
	{
		return 0;
	}

	// DirectX������
	if (initDirectX(hwnd) == false)
	{
		return 1;
	}

	// �E�B���h�E�\��
	ShowWindow(hwnd, nCmdShow);

	// ���b�Z�[�W���[�v
	do {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// ���C��
			Render();
		}
	} while (msg.message != WM_QUIT);

	// �������
	cleanupDirectX();

	// �o�^�����N���X������
	UnregisterClass(CLASS_NAME, hInstance);

	return msg.wParam;
}