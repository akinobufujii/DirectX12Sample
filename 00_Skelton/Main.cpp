//==============================================================================
// �C���N���[�h
//==============================================================================
#include <tchar.h>
#include <Windows.h>

#include <memory>

#include <d3d12.h>
#include <Dxgi1_3.h>
#include <D3d12SDKLayers.h>
#include <d3dcompiler.h>

#include <DirectXMath.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../libs/stb-master/stb_image.h"

//==============================================================================
// ���C�u���������N
//==============================================================================
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "D3DCompiler.lib")

//==============================================================================
// ��`
//==============================================================================
// ���̓��C�A�E�g
const D3D12_INPUT_ELEMENT_DESC INPUT_LAYOUT[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_PER_VERTEX_DATA, 0 },
};

// ���_��`
struct UserVertex
{
	DirectX::XMFLOAT3 pos;		// ���_���W
	DirectX::XMFLOAT4 color;	// ���_�J���[
	DirectX::XMFLOAT2 uv;		// UV���W
};


//==============================================================================
// �萔
//==============================================================================
static const int SCREEN_WIDTH = 1280;				// ��ʕ�
static const int SCREEN_HEIGHT = 720;				// ��ʍ���
static const LPTSTR	CLASS_NAME = TEXT("DirectX");	// �E�B���h�E�l�[��

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
ID3D12RootSignature*		g_pRootSignature;									// ���[�g�V�O�j�`��
D3D12_VIEWPORT				g_viewPort;											// �r���[�|�[�g
ID3D12Fence*				g_pFence;											// �t�F���X�I�u�W�F�N�g
HANDLE						g_hFenceEvent;										// �t�F���X�C�x���g�n���h��

ID3D12Resource*				g_pBackBufferResource;								// �o�b�N�o�b�t�@�̃��\�[�X
D3D12_CPU_DESCRIPTOR_HANDLE	g_hBackBuffer;										// �o�b�N�o�b�t�@�n���h��
ID3D12Resource*				g_pDepthStencilResource;							// �f�v�X�X�e���V���̃��\�[�X
D3D12_CPU_DESCRIPTOR_HANDLE	g_hDepthStencil;									// �f�v�X�X�e���V���̃n���h��

LPD3DBLOB					g_pVSBlob;											// ���_�V�F�[�_�u���u
LPD3DBLOB					g_pPSBlob;											// �s�N�Z���V�F�[�_�u���u

ID3D12Resource*				g_pVertexBufferResource;							// ���_�o�b�t�@�̃��\�[�X
D3D12_VERTEX_BUFFER_VIEW	g_VertexBufferView;									// ���_�o�b�t�@�r���[

ID3D12Resource*				g_pTextureResource;									// �e�N�X�`���̃��\�[�X
D3D12_CPU_DESCRIPTOR_HANDLE	g_hTexure;											// �e�N�X�`���n���h��

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

// ���S�ȊJ��
template<typename T>
void safeRelease(T*& object)
{
	if (object)
	{
		object->Release();
		object = nullptr;
	}
}

// �V�F�[�_�R���p�C��
bool compileShaderFlomFile(LPCWSTR pFileName, LPCSTR pEntrypoint, LPCSTR pTarget, ID3DBlob** ppblob)
{
	// �V�F�[�_�R���p�C��
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
			// �G���[�R�[�h�\��
#ifdef _UNICODE
			// Unicode�̎���Unicode�����ɕϊ�����
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

		// �R���p�C�����s
		return false;
	}

	// �R���p�C������
	return true;
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
		IID_PPV_ARGS(&g_pDevice));

	if(showErrorMessage(hr, TEXT("�f�o�C�X�쐬���s")))
	{
		return false;
	}

	// �R�}���h�A���P�[�^�쐬
	hr = g_pDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&g_pCommandAllocator));

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
	commandQueueDesk.NodeMask = 0x00000000;					// �m�[�h�}�X�N

	hr = g_pDevice->CreateCommandQueue(&commandQueueDesk, IID_PPV_ARGS(&g_pCommandQueue));

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
	// �f�o�b�O���[�h�̃t�@�N�g���쐬
	hr = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&g_pGIFactory));
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

	// �f�o�C�X����Ȃ��ăR�}���h�L���[��n��
	// �łȂ��Ǝ��s���G���[���N����
	hr = g_pGIFactory->CreateSwapChain(g_pCommandQueue, &swapChainDesc, &g_pGISwapChain);
	if (showErrorMessage(hr, TEXT("�X���b�v�`�F�[���쐬���s")))
	{
		return false;
	}

	// �`��R�}���h���X�g�쐬
	hr = g_pDevice->CreateCommandList(
		0x00000000,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		g_pCommandAllocator,
		g_pPipelineState,
		IID_PPV_ARGS(&g_pGraphicsCommandList));

	if (showErrorMessage(hr, TEXT("�R�}���h���C�����X�g�쐬���s")))
	{
		return false;
	}
	
	// ���_�V�F�[�_�R���p�C��
	if (compileShaderFlomFile(L"VertexShader.hlsl", "main", "vs_5_1", &g_pVSBlob) == false)
	{
		showErrorMessage(E_FAIL, TEXT("���_�V�F�[�_�R���p�C�����s"));
	}

	// �s�N�Z���V�F�[�_�R���p�C��
	if (compileShaderFlomFile(L"PixelShader.hlsl", "main", "ps_5_1", &g_pPSBlob) == false)
	{
		showErrorMessage(E_FAIL, TEXT("�s�N�Z���V�F�[�_�R���p�C�����s"));
	}

	// ��̃��[�g�V�O�j�`���쐬
	LPD3DBLOB pOutBlob = nullptr;
	D3D12_ROOT_SIGNATURE descRootSignature = D3D12_ROOT_SIGNATURE();
	descRootSignature.Flags = D3D12_ROOT_SIGNATURE_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_V1, &pOutBlob, nullptr);
	hr = g_pDevice->CreateRootSignature(
		0x00000001,
		pOutBlob->GetBufferPointer(),
		pOutBlob->GetBufferSize(),
		IID_PPV_ARGS(&g_pRootSignature));

	safeRelease(pOutBlob);
	if (showErrorMessage(hr, TEXT("���[�g�V�O�j�`���쐬���s")))
	{
		return false;
	}

	// �p�C�v���C���X�e�[�g�I�u�W�F�N�g�쐬
	// ���_�V�F�[�_�ƃs�N�Z���V�F�[�_���Ȃ��ƁA�쐬�Ɏ��s����
	D3D12_GRAPHICS_PIPELINE_STATE_DESC descPSO;
	ZeroMemory(&descPSO, sizeof(descPSO));
	descPSO.InputLayout = { INPUT_LAYOUT, ARRAYSIZE(INPUT_LAYOUT) };										// �C���v�b�g���C�A�E�g�ݒ�
	descPSO.pRootSignature = g_pRootSignature;																// ���[�g�V�O�j�`���ݒ�
	descPSO.VS = { reinterpret_cast<BYTE*>(g_pVSBlob->GetBufferPointer()), g_pVSBlob->GetBufferSize() };	// ���_�V�F�[�_�ݒ�
	descPSO.PS = { reinterpret_cast<BYTE*>(g_pPSBlob->GetBufferPointer()), g_pPSBlob->GetBufferSize() };	// �s�N�Z���V�F�[�_�ݒ�
	descPSO.RasterizerState = CD3D12_RASTERIZER_DESC(D3D12_DEFAULT);										// ���X�^���C�U�ݒ�
	descPSO.BlendState = CD3D12_BLEND_DESC(D3D12_DEFAULT);													// �u�����h�ݒ�
	descPSO.DepthStencilState.DepthEnable = FALSE;															// �[�x�o�b�t�@�L���ݒ�
	descPSO.DepthStencilState.StencilEnable = FALSE;														// �X�e���V���o�b�t�@�L���ݒ�
	descPSO.SampleMask = UINT_MAX;																			// �T���v���}�X�N�ݒ�
	descPSO.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;									// �v���~�e�B�u�^�C�v	
	descPSO.NumRenderTargets = 1;																			// �����_�[�^�[�Q�b�g��
	descPSO.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;														// �����_�[�^�[�Q�b�g�t�H�[�}�b�g
	descPSO.SampleDesc.Count = 1;																			// �T���v���J�E���g

	hr = g_pDevice->CreateGraphicsPipelineState(&descPSO, IID_PPV_ARGS(&g_pPipelineState));

	if (showErrorMessage(hr, TEXT("�p�C�v���C���X�e�[�g�I�u�W�F�N�g�쐬���s")))
	{
		return false;
	}

	// �f�B�X�N���v�^�q�[�v�쐬
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
	ZeroMemory(&heapDesc, sizeof(heapDesc));

	heapDesc.NumDescriptors = 1;

	for (int i = 0; i < DESCRIPTOR_HEAP_TYPE_MAX; ++i)
	{
		heapDesc.Flags = (i == D3D12_RTV_DESCRIPTOR_HEAP) ? D3D12_DESCRIPTOR_HEAP_NONE : D3D12_DESCRIPTOR_HEAP_SHADER_VISIBLE;
		heapDesc.Type = static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i);
		
		hr = g_pDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&g_pDescripterHeapArray[i]));
		if (showErrorMessage(hr, TEXT("�f�B�X�N���v�^�q�[�v�쐬���s")))
		{
			return false;
		}
	}

	// �f�B�X�N���v�^�q�[�v�ƃR�}���h���X�g�̊֘A�Â�
	g_pGraphicsCommandList->SetDescriptorHeaps(g_pDescripterHeapArray, DESCRIPTOR_HEAP_TYPE_SET);

	// �����_�[�^�[�Q�b�g�r���[���쐬
	hr = g_pGISwapChain->GetBuffer(0, IID_PPV_ARGS(&g_pBackBufferResource));
	if (showErrorMessage(hr, TEXT("�����_�[�^�[�Q�b�g�r���[�쐬���s")))
	{
		return false;
	}
	
	g_hBackBuffer = g_pDescripterHeapArray[DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart();
	g_pDevice->CreateRenderTargetView(g_pBackBufferResource, nullptr, g_hBackBuffer);

#if 0
	// �[�x�X�e���V���r���[�쐬
	hr = g_pGISwapChain->GetBuffer(0, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&g_pDepthStencilResource));
	if (showErrorMessage(hr, TEXT("�[�x�X�e���V���r���[�쐬���s")))
	{
		return false;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;			// �t�H�[�}�b�g
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;	// ���\�[�X�A�N�Z�X���@
	depthStencilDesc.Flags = D3D12_DSV_NONE;					// �A�N�Z�X�t���O
	depthStencilDesc.Texture2D.MipSlice = 0;								// �g�p����ŏ��̃~�b�v�}�b�v

	g_hDepthStencil = g_pDescripterHeapArray[DESCRIPTOR_HEAP_TYPE_DSV]->GetCPUDescriptorHandleForHeapStart();
	g_pDevice->CreateDepthStencilView(g_pDepthStencilResource, &depthStencilDesc, g_hDepthStencil);
#endif

	// �r���[�|�[�g�ݒ�
	g_viewPort.TopLeftX = 0;			// X���W
	g_viewPort.TopLeftY = 0;			// Y���W
	g_viewPort.Width = SCREEN_WIDTH;	// ��
	g_viewPort.Height = SCREEN_HEIGHT;	// ����
	g_viewPort.MinDepth = 0.0f;			// �ŏ��[�x
	g_viewPort.MaxDepth = 1.0f;			// �ő�[�x

	// �t�F���X�I�u�W�F�N�g�쐬
	g_pDevice->CreateFence(0, D3D12_FENCE_MISC_NONE, IID_PPV_ARGS(&g_pFence));

	if (showErrorMessage(hr, TEXT("�t�F���X�I�u�W�F�N�g�쐬���s")))
	{
		return false;
	}

	// �t�F���X�C�x���g�n���h���쐬
	g_hFenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	return true;
}

// DirectX12�N���[���A�b�v
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

// ���\�[�X�Z�b�g�A�b�v
bool setupResource() 
{
	HRESULT hr;

	// ���_�o�b�t�@�쐬
	UserVertex vertex[] =
	{
		{ DirectX::XMFLOAT3( 0.0f,  0.5f, 0.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), DirectX::XMFLOAT2(0.0f, 0.0f) },
		{ DirectX::XMFLOAT3( 0.5f, -0.5,  0.0f), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f), DirectX::XMFLOAT2(1.0f, 0.0f) },
		{ DirectX::XMFLOAT3(-0.5f, -0.5f, 0.0f), DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f), DirectX::XMFLOAT2(1.0f, 1.0f) },
	};

	// ���h�L�������g�ɂ͐ÓI�f�[�^���A�b�v���[�h�q�[�v�ɓn���̂͐������Ȃ��Ƃ̂���
	// �@�q�[�v�̏���ʂ���낵���Ȃ��悤�ł�
	hr = g_pDevice->CreateCommittedResource(
		&CD3D12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_MISC_NONE,
		&CD3D12_RESOURCE_DESC::Buffer(ARRAYSIZE(vertex) * sizeof(UserVertex)),
		D3D12_RESOURCE_USAGE_GENERIC_READ,
		nullptr,
		__uuidof(ID3D12Resource),
		reinterpret_cast<void**>(&g_pVertexBufferResource));

	if (showErrorMessage(hr, TEXT("���_�o�b�t�@�쐬���s")))
	{
		return false;
	}

	// ���_�o�b�t�@�ɎO�p�`�����R�s�[����
	UINT8* dataBegin;
	if (SUCCEEDED(g_pVertexBufferResource->Map(0, nullptr, reinterpret_cast<void**>(&dataBegin)))) 
	{
		memcpy(dataBegin, vertex, sizeof(vertex));
		g_pVertexBufferResource->Unmap(0, nullptr);
	}
	else 
	{
		showErrorMessage(E_FAIL, TEXT("���_�o�b�t�@�̃}�b�s���O�Ɏ��s"));
		return false;
	}

	// ���_�o�b�t�@�r���[�ݒ�
	g_VertexBufferView.BufferLocation = g_pVertexBufferResource->GetGPUVirtualAddress();
	g_VertexBufferView.StrideInBytes = sizeof(UserVertex);
	g_VertexBufferView.SizeInBytes = sizeof(vertex);

#if 0
	// �e�N�X�`���쐬
	// ���\�[�X�o�b�t�@���l��
	hr = g_pGISwapChain->GetBuffer(0, IID_PPV_ARGS(&g_pTextureResource));
	if (showErrorMessage(hr, TEXT("�e�N�X�`���[�p�o�b�t�@�l�����s")))
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

	// �e�N�X�`���̐��f�[�^����������
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
	g_pTextureResource->SetName(TEXT("�e�N�X�`��"));

	delete [] clearColor;
	stbi_image_free(pixels);

#endif
	return true;
}

// ���\�[�X�N���[���A�b�v
void cleanupResource()
{
	safeRelease(g_pVertexBufferResource);
}

// ���\�[�X�ݒ莞�̃o���A�֐�
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

// �`��
void Render()
{
	// ���݂̃R�}���h���X�g���l��
	ID3D12GraphicsCommandList* pCommand = g_pGraphicsCommandList;

	// �p�C�v���C���X�e�[�g�I�u�W�F�N�g�ƃ��[�g�V�O�j�`�����Z�b�g
	pCommand->SetPipelineState(g_pPipelineState);
	pCommand->SetGraphicsRootSignature(g_pRootSignature);

	// �����_�[�^�[�Q�b�g�r���[��ݒ�
	pCommand->SetRenderTargets(&g_hBackBuffer, TRUE, 1, nullptr);

	// ��`��ݒ�
	CD3D12_RECT clearRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	pCommand->RSSetViewports(1, &g_viewPort);
	pCommand->RSSetScissorRects(1, &clearRect);

	// �����_�[�^�[�Q�b�g�փ��\�[�X��ݒ�
	setResourceBarrier(
		pCommand,
		g_pBackBufferResource,
		D3D12_RESOURCE_USAGE_PRESENT,
		D3D12_RESOURCE_USAGE_RENDER_TARGET);

	// �N���A
	static float count = 0;
	count = fmod(count + 0.01f, 1.0f);
	float clearColor[] = { count, 0.2f, 0.4f, 1.0f };
	pCommand->ClearRenderTargetView(g_hBackBuffer, clearColor, &clearRect, 1);

	// �O�p�`�`��
	pCommand->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommand->SetVertexBuffers(0, &g_VertexBufferView, 1);
	pCommand->DrawInstanced(3, 1, 0, 0);
	
	// present�O�̏���
	setResourceBarrier(
		pCommand,
		g_pBackBufferResource,
		D3D12_RESOURCE_USAGE_RENDER_TARGET,
		D3D12_RESOURCE_USAGE_PRESENT);

	// �`��R�}���h�����s���ăt���b�v
	pCommand->Close();
	g_pCommandQueue->ExecuteCommandLists(1, CommandListCast(&pCommand));
	g_pGISwapChain->Present(1, 0);

	// �R�}���h�L���[�̏�����҂�
	g_pFence->Signal(0);
	g_pFence->SetEventOnCompletion(1, g_hFenceEvent);
	g_pCommandQueue->Signal(g_pFence, 1);
	WaitForSingleObject(g_hFenceEvent, INFINITE);

	// �R�}���h�A���P�[�^�ƃR�}���h���X�g�����Z�b�g
	g_pCommandAllocator->Reset();
	pCommand->Reset(g_pCommandAllocator, g_pPipelineState);
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
		return 1;
	}

	// DirectX������
	if (initDirectX(hwnd) == false)
	{
		return 1;
	}

	if (setupResource() == false) 
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
	cleanupResource();
	cleanupDirectX();

	// �o�^�����N���X������
	UnregisterClass(CLASS_NAME, hInstance);

	return 0;
}