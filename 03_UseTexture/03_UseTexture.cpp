//==============================================================================
// �C���N���[�h
//==============================================================================
#include "../libs/akilib/include/D3D12Helper.h"

//�h�L�������g�ɂ͂�����ď����Ă���̂ɂȂ��g�����C�u����
// https://msdn.microsoft.com/en-us/library/dn708058(v=vs.85).aspx
//#include <d3dx12.h.>

#include <Dxgi1_4.h>
#include <d3d12sdklayers.h>

#include <DirectXMath.h>

#pragma warning( push )
#pragma warning( disable: 4013 4068 4312 4456 4457 )
#define STB_IMAGE_IMPLEMENTATION
#include "../libs/stb-master/stb_image.h"
#pragma warning( pop )

//==============================================================================
// ��`
//==============================================================================
#define RESOURCE_SETUP	// ���\�[�X�Z�b�g�A�b�v������

// ���̓��C�A�E�g
const D3D12_INPUT_ELEMENT_DESC INPUT_LAYOUT[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
};

// ���_��`
struct UserVertex
{
	DirectX::XMFLOAT3 pos;		// ���_���W
	DirectX::XMFLOAT4 color;	// ���_�J���[
	DirectX::XMFLOAT2 uv;		// UV���W
};

// �R���X�^���g�o�b�t�@
struct cbMatrix
{
	DirectX::XMMATRIX	_WVP;
};

//==============================================================================
// �萔
//==============================================================================
static const int SCREEN_WIDTH = 1280;				// ��ʕ�
static const int SCREEN_HEIGHT = 720;				// ��ʍ���
static const LPTSTR	CLASS_NAME = TEXT("03_UseTexture");	// �E�B���h�E�l�[��
static const UINT BACKBUFFER_COUNT = 2;				// �o�b�N�o�b�t�@��

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

//==============================================================================
// �O���[�o���ϐ�
//==============================================================================
#if _DEBUG
ID3D12Debug*				g_pDebug;											// �f�o�b�O�I�u�W�F�N�g
#endif

ID3D12Device*				g_pDevice;											// �f�o�C�X
ID3D12CommandAllocator*		g_pCommandAllocator;								// �R�}���h�A���P�[�^
ID3D12CommandQueue*			g_pCommandQueue;									// �R�}���h�L���[
IDXGIDevice2*				g_pGIDevice;										// GI�f�o�C�X
IDXGIAdapter*				g_pGIAdapter;										// GI�A�_�v�^�[
IDXGIFactory2*				g_pGIFactory;										// GI�t�@�N�g���[
IDXGISwapChain3*			g_pGISwapChain;										// GI�X���b�v�`�F�[��
ID3D12PipelineState*		g_pPipelineState;									// �p�C�v���C���X�e�[�g
ID3D12DescriptorHeap*		g_pDescripterHeapArray[DESCRIPTOR_HEAP_TYPE_MAX];	// �f�B�X�N���v�^�q�[�v�z��
ID3D12GraphicsCommandList*	g_pGraphicsCommandList;								// �`��R�}���h���X�g
ID3D12RootSignature*		g_pRootSignature;									// ���[�g�V�O�j�`��
D3D12_VIEWPORT				g_viewPort;											// �r���[�|�[�g
ID3D12Fence*				g_pFence;											// �t�F���X�I�u�W�F�N�g
HANDLE						g_hFenceEvent;										// �t�F���X�C�x���g�n���h��
UINT64						g_CurrentFenceIndex = 0;							// ���݂̃t�F���X�C���f�b�N�X

UINT						g_CurrentBuckBufferIndex = 0;						// ���݂̃o�b�N�o�b�t�@
ID3D12Resource*				g_pBackBufferResource[BACKBUFFER_COUNT];			// �o�b�N�o�b�t�@�̃��\�[�X
D3D12_CPU_DESCRIPTOR_HANDLE	g_hBackBuffer[BACKBUFFER_COUNT];					// �o�b�N�o�b�t�@�n���h��

LPD3DBLOB					g_pVSBlob;											// ���_�V�F�[�_�u���u
LPD3DBLOB					g_pPSBlob;											// �s�N�Z���V�F�[�_�u���u

ID3D12Resource*				g_pVertexBufferResource;							// ���_�o�b�t�@�̃��\�[�X
D3D12_VERTEX_BUFFER_VIEW	g_VertexBufferView;									// ���_�o�b�t�@�r���[

ID3D12Heap*					g_pTextureHeap;										// �e�N�X�`���q�[�v
ID3D12Resource*				g_pTextureResource;									// �e�N�X�`�����\�[�X
ID3D12Resource*				g_pTextureUploadHeap;								// �e�N�X�`���A�b�v���[�h�q�[�v

// DirectX������
bool initDirectX(HWND hWnd)
{
	HRESULT hr;
	UINT GIFlag = 0;
#if _DEBUG
	// �f�o�b�O���C���[�쐬
	hr = D3D12GetDebugInterface(IID_PPV_ARGS(&g_pDebug));
	if(showErrorMessage(hr, TEXT("�f�o�b�O���C���[�쐬���s")))
	{
		return false;
	}
	if(g_pDebug)
	{
		g_pDebug->EnableDebugLayer();
	}
	GIFlag = DXGI_CREATE_FACTORY_DEBUG;
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

	// �R�}���h�A���P�[�^�쐬
	hr = g_pDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&g_pCommandAllocator));

	if(showErrorMessage(hr, TEXT("�R�}���h�A���P�[�^�쐬���s")))
	{
		hr = g_pDevice->GetDeviceRemovedReason();
		return false;
	}

	// �R�}���h�L���[�쐬
	// �h�L�������g�ɂ���ȉ��̂悤�ȃ��\�b�h�͂Ȃ�����
	//g_pDevice->GetDefaultCommandQueue(&g_pCommandQueue);
	D3D12_COMMAND_QUEUE_DESC commandQueueDesk;
	commandQueueDesk.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;	// �R�}���h���X�g�^�C�v
	commandQueueDesk.Priority = 0;							// �D��x
	commandQueueDesk.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;	// �t���O
	commandQueueDesk.NodeMask = 0x00000000;					// �m�[�h�}�X�N

	hr = g_pDevice->CreateCommandQueue(&commandQueueDesk, IID_PPV_ARGS(&g_pCommandQueue));

	if(showErrorMessage(hr, TEXT("�R�}���h�L���[�쐬���s")))
	{
		return false;
	}

	// �X���b�v�`�F�[�����쐬
	// ������DirectX11�Ƃ��܂�ς��Ȃ�
	// �Q�lURL:https://msdn.microsoft.com/en-us/library/dn859356(v=vs.85).aspx

	DXGI_SWAP_CHAIN_DESC descSwapChain;
	ZeroMemory(&descSwapChain, sizeof(descSwapChain));
	descSwapChain.BufferCount = BACKBUFFER_COUNT;					// �o�b�N�o�b�t�@��2���ȏ�Ȃ��Ǝ��s����
	descSwapChain.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	descSwapChain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	descSwapChain.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	descSwapChain.OutputWindow = hWnd;
	descSwapChain.SampleDesc.Count = 1;
	descSwapChain.Windowed = TRUE;

	// �f�o�C�X����Ȃ��ăR�}���h�L���[��n��
	// �łȂ��Ǝ��s���G���[���N����
	hr = g_pGIFactory->CreateSwapChain(g_pCommandQueue, &descSwapChain, reinterpret_cast<IDXGISwapChain**>(&g_pGISwapChain));
	if(showErrorMessage(hr, TEXT("�X���b�v�`�F�[���쐬���s")))
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

	if(showErrorMessage(hr, TEXT("�R�}���h���C�����X�g�쐬���s")))
	{
		return false;
	}

	// ���_�V�F�[�_�R���p�C��
	if(compileShaderFlomFile(L"VertexShader.hlsl", "main", "vs_5_0", &g_pVSBlob) == false)
	{
		showErrorMessage(E_FAIL, TEXT("���_�V�F�[�_�R���p�C�����s"));
	}

	// �s�N�Z���V�F�[�_�R���p�C��
	if(compileShaderFlomFile(L"PixelShaderByTexture.hlsl", "main", "ps_5_0", &g_pPSBlob) == false)
	{
		showErrorMessage(E_FAIL, TEXT("�s�N�Z���V�F�[�_�R���p�C�����s"));
	}

	CD3DX12_DESCRIPTOR_RANGE ranges[1];
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER rootParameters[1];
	rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.MipLODBias = 0;
	sampler.MaxAnisotropy = 0;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	Microsoft::WRL::ComPtr<ID3DBlob> signature;
	Microsoft::WRL::ComPtr<ID3DBlob> error;

	hr = D3D12SerializeRootSignature(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&signature,
		&error);

	if(showErrorMessage(hr, TEXT("���[�g�V�O�j�`���V���A���C�Y���s")))
	{
		return false;
	}

	hr = g_pDevice->CreateRootSignature(
		0,
		signature->GetBufferPointer(),
		signature->GetBufferSize(),
		IID_PPV_ARGS(&g_pRootSignature));

	if(showErrorMessage(hr, TEXT("���[�g�V�O�j�`���쐬���s")))
	{
		return false;
	}

	// ���C�X�^���C�U�[�X�e�[�g�ݒ�
	// �f�t�H���g�̐ݒ�͈ȉ��̃y�[�W���Q��(ScissorEnable�͂Ȃ����ǁE�E�E)
	// https://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=JA-JP&k=k(d3d12%2FD3D12_RASTERIZER_DESC);k(D3D12_RASTERIZER_DESC);k(DevLang-C%2B%2B);k(TargetOS-Windows)&rd=true
	D3D12_RASTERIZER_DESC descRasterizer;
	descRasterizer.FillMode = D3D12_FILL_MODE_SOLID;
	descRasterizer.CullMode = D3D12_CULL_MODE_NONE;
	descRasterizer.FrontCounterClockwise = FALSE;
	descRasterizer.DepthBias = 0;
	descRasterizer.SlopeScaledDepthBias = 0.0f;
	descRasterizer.DepthBiasClamp = 0.0f;
	descRasterizer.DepthClipEnable = TRUE;
	descRasterizer.MultisampleEnable = FALSE;
	descRasterizer.AntialiasedLineEnable = FALSE;
	descRasterizer.ForcedSampleCount = 0;
	descRasterizer.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// �u�����h�X�e�[�g�ݒ�
	// https://msdn.microsoft.com/query/dev14.query?appId=Dev14IDEF1&l=JA-JP&k=k(d3d12%2FD3D12_BLEND_DESC);k(D3D12_BLEND_DESC);k(DevLang-C%2B%2B);k(TargetOS-Windows)&rd=true
	D3D12_BLEND_DESC descBlend;
	descBlend.AlphaToCoverageEnable = FALSE;
	descBlend.IndependentBlendEnable = FALSE;
	descBlend.RenderTarget[0].BlendEnable = FALSE;
	descBlend.RenderTarget[0].LogicOpEnable = FALSE;
	descBlend.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	descBlend.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
	descBlend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	descBlend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	descBlend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	descBlend.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	descBlend.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	descBlend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	// �p�C�v���C���X�e�[�g�I�u�W�F�N�g�쐬
	// ���_�V�F�[�_�ƃs�N�Z���V�F�[�_���Ȃ��ƁA�쐬�Ɏ��s����
	D3D12_GRAPHICS_PIPELINE_STATE_DESC descPSO;
	ZeroMemory(&descPSO, sizeof(descPSO));
	descPSO.InputLayout = { INPUT_LAYOUT, ARRAYSIZE(INPUT_LAYOUT) };										// �C���v�b�g���C�A�E�g�ݒ�
	descPSO.pRootSignature = g_pRootSignature;																// ���[�g�V�O�j�`���ݒ�
	descPSO.VS = { reinterpret_cast<BYTE*>(g_pVSBlob->GetBufferPointer()), g_pVSBlob->GetBufferSize() };	// ���_�V�F�[�_�ݒ�
	descPSO.PS = { reinterpret_cast<BYTE*>(g_pPSBlob->GetBufferPointer()), g_pPSBlob->GetBufferSize() };	// �s�N�Z���V�F�[�_�ݒ�
	descPSO.RasterizerState = descRasterizer;																// ���X�^���C�U�ݒ�
	descPSO.BlendState = descBlend;																			// �u�����h�ݒ�
	descPSO.DepthStencilState.DepthEnable = FALSE;															// �[�x�o�b�t�@�L���ݒ�
	descPSO.DepthStencilState.StencilEnable = FALSE;														// �X�e���V���o�b�t�@�L���ݒ�
	descPSO.SampleMask = UINT_MAX;																			// �T���v���}�X�N�ݒ�
	descPSO.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;									// �v���~�e�B�u�^�C�v	
	descPSO.NumRenderTargets = 1;																			// �����_�[�^�[�Q�b�g��
	descPSO.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;														// �����_�[�^�[�Q�b�g�t�H�[�}�b�g
	descPSO.SampleDesc.Count = 1;																			// �T���v���J�E���g

	hr = g_pDevice->CreateGraphicsPipelineState(&descPSO, IID_PPV_ARGS(&g_pPipelineState));

	if(showErrorMessage(hr, TEXT("�p�C�v���C���X�e�[�g�I�u�W�F�N�g�쐬���s")))
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

	return true;
}

// DirectX12�N���[���A�b�v
void cleanupDirectX()
{
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
#if _DEBUG
	safeRelease(g_pDebug);
#endif
}

#if defined(RESOURCE_SETUP)
// ���\�[�X�Z�b�g�A�b�v
bool setupResource()
{
	HRESULT hr;

	// ���_�o�b�t�@�쐬
	UserVertex vertex[] =
	{
		{ DirectX::XMFLOAT3(-1.0f, 1.0f, 0.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), DirectX::XMFLOAT2(0.0f, 0.0f) },	// ����
		{ DirectX::XMFLOAT3(1.0f, 1.0,  0.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), DirectX::XMFLOAT2(1.0f, 0.0f) },	// �E��
		{ DirectX::XMFLOAT3(1.0f, -1.0f, 0.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), DirectX::XMFLOAT2(1.0f, 1.0f) },	// �E��

		{ DirectX::XMFLOAT3(1.0f, -1.0f, 0.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), DirectX::XMFLOAT2(1.0f, 1.0f) },	// �E��
		{ DirectX::XMFLOAT3(-1.0f, -1.0,  0.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), DirectX::XMFLOAT2(0.0f, 1.0f) },// ����
		{ DirectX::XMFLOAT3(-1.0f, 1.0f, 0.0f), DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f), DirectX::XMFLOAT2(0.0f, 0.0f) },	// ����
	};

	// ���h�L�������g�ɂ͐ÓI�f�[�^���A�b�v���[�h�q�[�v�ɓn���̂͐������Ȃ��Ƃ̂���
	// �@�q�[�v�̏���ʂ���낵���Ȃ��悤�ł�
	{
		D3D12_HEAP_PROPERTIES heapPropaty;
		heapPropaty.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapPropaty.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapPropaty.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapPropaty.CreationNodeMask = 0;
		heapPropaty.VisibleNodeMask = 0;

		D3D12_RESOURCE_DESC descResource;
		descResource.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		descResource.Alignment = 0;
		descResource.Width = ARRAYSIZE(vertex) * sizeof(UserVertex);
		descResource.Height = 1;
		descResource.DepthOrArraySize = 1;
		descResource.MipLevels = 1;
		descResource.Format = DXGI_FORMAT_UNKNOWN;
		descResource.SampleDesc.Count = 1;
		descResource.SampleDesc.Quality = 0;
		descResource.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		descResource.Flags = D3D12_RESOURCE_FLAG_NONE;

		hr = g_pDevice->CreateCommittedResource(
			&heapPropaty,
			D3D12_HEAP_FLAG_NONE,
			&descResource,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&g_pVertexBufferResource));

		if(showErrorMessage(hr, TEXT("���_�o�b�t�@�쐬���s")))
		{
			return false;
		}

		// ���_�o�b�t�@�ɎO�p�`�����R�s�[����
		UINT8* dataBegin;
		if(SUCCEEDED(g_pVertexBufferResource->Map(0, nullptr, reinterpret_cast<void**>(&dataBegin))))
		{
			memcpy(dataBegin, vertex, sizeof(vertex));
			g_pVertexBufferResource->Unmap(0, nullptr);
		}
		else
		{
			showErrorMessage(E_FAIL, TEXT("���_�o�b�t�@�̃}�b�s���O�Ɏ��s"));
			return false;
		}
	}

	// ���_�o�b�t�@�r���[�ݒ�
	g_VertexBufferView.BufferLocation = g_pVertexBufferResource->GetGPUVirtualAddress();
	g_VertexBufferView.StrideInBytes = sizeof(UserVertex);
	g_VertexBufferView.SizeInBytes = sizeof(vertex);

	// �e�N�X�`���쐬
	// GPU��Ƀe�N�X�`�������[�h���邽�߂ɃA�b�v���[�h�q�[�v���쐬���܂��B
	// ComPtr�̂́ACPU�̃I�u�W�F�N�g�ł����A���̃q�[�v�́AGPU�̍�Ƃ���������܂ł͈̔͂ɑ؍݂���K�v������܂��B
	// ComPtr���j�󂳂��O�ɁA��X�́A���̃��\�b�h�̍Ō��GPU�Ɠ������܂��B
	{
		// �摜�f�[�^�ǂݍ���
		int width, height, comp;
		stbi_uc* pixels = stbi_load("../resource/���C�ɓ���.png", &width, &height, &comp, 0);

		// �e�N�X�`���f�[�^�쐬
		D3D12_RESOURCE_DESC descResource = {};
		descResource.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		descResource.Alignment = 0;
		descResource.Width = width;
		descResource.Height = height;
		descResource.DepthOrArraySize = 1;
		descResource.MipLevels = 1;
		descResource.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		descResource.SampleDesc.Count = 1;
		descResource.SampleDesc.Quality = 0;
		descResource.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		descResource.Flags = D3D12_RESOURCE_FLAG_NONE;

		hr = g_pDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),//&heapPropaty,
			D3D12_HEAP_FLAG_NONE,
			&descResource,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&g_pTextureResource));

		if(showErrorMessage(hr, TEXT("�e�N�X�`�����\�[�X�쐬���s")))
		{
			return false;
		}

		// GPU�A�b�v���[�h�o�b�t�@���쐬
		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(g_pTextureResource, 0, 1);

		hr = g_pDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&g_pTextureUploadHeap));

		if(showErrorMessage(hr, TEXT("GPU�A�b�v���[�h�o�b�t�@���쐬���s")))
		{
			return false;
		}

		// ���ۂ̃e�N�X�`���f�[�^���쐬
		D3D12_SUBRESOURCE_DATA textureData = {};
		textureData.pData = &pixels[0];
		textureData.RowPitch = width * 4;	// ���~�s�N�Z���̃o�C�g��
		textureData.SlicePitch = textureData.RowPitch * height;

		UpdateSubresources(g_pGraphicsCommandList, g_pTextureResource, g_pTextureUploadHeap, 0, 0, 1, &textureData);
		g_pGraphicsCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(g_pTextureResource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

		// ����Ȃ����̉��
		stbi_image_free(pixels);

		// Describe and create a SRV for the texture.
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = descResource.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		g_pDevice->CreateShaderResourceView(
			g_pTextureResource,
			&srvDesc,
			g_pDescripterHeapArray[DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetCPUDescriptorHandleForHeapStart());
	}

	return true;
}
#endif

#if defined(RESOURCE_SETUP)
// ���\�[�X�N���[���A�b�v
void cleanupResource()
{
	safeRelease(g_pTextureUploadHeap);
	safeRelease(g_pTextureResource);
	safeRelease(g_pVertexBufferResource);
}
#endif

// �`��
void Render()
{
	// ���݂̃R�}���h���X�g���l��
	ID3D12GraphicsCommandList* pCommand = g_pGraphicsCommandList;

	// �p�C�v���C���X�e�[�g�I�u�W�F�N�g�ƃ��[�g�V�O�j�`�����Z�b�g
	pCommand->SetPipelineState(g_pPipelineState);
	pCommand->SetGraphicsRootSignature(g_pRootSignature);

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

	// �e�N�X�`���̐ݒ�
	pCommand->SetDescriptorHeaps(1, &g_pDescripterHeapArray[DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
	pCommand->SetGraphicsRootDescriptorTable(0, g_pDescripterHeapArray[DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetGPUDescriptorHandleForHeapStart());

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

#if defined(RESOURCE_SETUP)

	// �|���S���`��
	pCommand->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommand->IASetVertexBuffers(0, 1, &g_VertexBufferView);
	pCommand->DrawInstanced(6, 2, 0, 0);
#endif

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
	const UINT64 FENCE_INDEX = g_CurrentFenceIndex;
	g_pCommandQueue->Signal(g_pFence, FENCE_INDEX);
	g_CurrentFenceIndex++;

	if(g_pFence->GetCompletedValue() < FENCE_INDEX)
	{
		g_pFence->SetEventOnCompletion(FENCE_INDEX, g_hFenceEvent);
		WaitForSingleObject(g_hFenceEvent, INFINITE);
	}

	// �R�}���h�A���P�[�^�ƃR�}���h���X�g�����Z�b�g
	g_pCommandAllocator->Reset();
	pCommand->Reset(g_pCommandAllocator, g_pPipelineState);
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

#if defined(RESOURCE_SETUP)
	if(setupResource() == false)
	{
		return 1;
	}
#endif

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
#if defined(RESOURCE_SETUP)
	cleanupResource();
#endif
	cleanupDirectX();

	// �o�^�����N���X������
	UnregisterClass(CLASS_NAME, hInstance);

	return 0;
}