#include "Mesh.h"

CMesh::CMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) {}

CMesh::~CMesh() {
	if (m_pd3dVertexBuffer) m_pd3dVertexBuffer->Release();
	if (m_pd3dVertexUploadBuffer) m_pd3dVertexUploadBuffer->Release();
	if (m_pd3dIndexBuffer) m_pd3dIndexBuffer->Release();
	if (m_pd3dIndexUploadBuffer) m_pd3dIndexUploadBuffer->Release();
}

void CMesh::ReleaseUploadBuffers() {
	//정점 버퍼를 위한 업로드 버퍼를 소멸시킨다.
	if (m_pd3dVertexUploadBuffer) m_pd3dVertexUploadBuffer->Release();
	m_pd3dVertexUploadBuffer = NULL;
	if (m_pd3dIndexUploadBuffer) m_pd3dIndexUploadBuffer->Release();
	m_pd3dIndexUploadBuffer = NULL;
};

void CMesh::Render(ID3D12GraphicsCommandList* pd3dCommandList) {
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);
	pd3dCommandList->IASetVertexBuffers(m_nSlot, 1, &m_d3dVertexBufferView);
	if (m_pd3dIndexBuffer) {
		pd3dCommandList->IASetIndexBuffer(&m_d3dIndexBufferView);
		pd3dCommandList->DrawIndexedInstanced(m_nIndices, 1, 0, 0, 0);
		//인덱스 버퍼가 있으면 인덱스 버퍼를 파이프라인(IA: 입력 조립기)에 연결하고 인덱스를 사용하여 렌더링한다.
	}
	else {
		pd3dCommandList->DrawInstanced(m_nVertices, 1, m_nOffset, 0);
	}
}

void CMesh::Render(ID3D12GraphicsCommandList* pd3dCommandList, int nSubMeshIndex) {
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);
	pd3dCommandList->IASetVertexBuffers(m_nSlot, 1, &m_d3dVertexBufferView);
	
	if (m_pd3dIndexBuffer && nSubMeshIndex < (int)m_vSubMeshes.size()) {
		pd3dCommandList->IASetIndexBuffer(&m_d3dIndexBufferView);
		SubMeshInfo& subMesh = m_vSubMeshes[nSubMeshIndex];
		pd3dCommandList->DrawIndexedInstanced(subMesh.nIndexCount, 1, subMesh.nStartIndex, 0, 0);
	}
}


// 삼각형 메쉬 생성
CTriangleMesh::CTriangleMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) : CMesh(pd3dDevice, pd3dCommandList) {
	//삼각형 메쉬를 정의한다.
	m_nVertices = 3;
	m_nStride = sizeof(CDiffusedVertex);
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	/*정점(삼각형의 꼭지점)의 색상은 시계방향 순서대로 빨간색, 녹색, 파란색으로 지정한다. RGBA(Red, Green, Blue,
	Alpha) 4개의 파라메터를 사용하여 색상을 표현한다. 각 파라메터는 0.0~1.0 사이의 실수값을 가진다.*/
	CDiffusedVertex pVertices[3];
	pVertices[0] = CDiffusedVertex(XMFLOAT3(0.0f, 0.5f, 0.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f));
	pVertices[1] = CDiffusedVertex(XMFLOAT3(0.5f, -0.5f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));
	pVertices[2] = CDiffusedVertex(XMFLOAT3(-0.5f, -0.5f, 0.0f), XMFLOAT4(Colors::Blue));
	//삼각형 메쉬를 리소스(정점 버퍼)로 생성한다.
	m_pd3dVertexBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pVertices, m_nStride * m_nVertices, 
		D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dVertexUploadBuffer);
	//정점 버퍼 뷰를 생성한다.
	m_d3dVertexBufferView.BufferLocation = m_pd3dVertexBuffer->GetGPUVirtualAddress();
	m_d3dVertexBufferView.StrideInBytes = m_nStride;
	m_d3dVertexBufferView.SizeInBytes = m_nStride * m_nVertices;
}


// 직육면체 메쉬 생성
CCubeMeshDiffused::CCubeMeshDiffused(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
	float fWidth, float fHeight, float fDepth) : CMesh(pd3dDevice, pd3dCommandList) {
	//직육면체는 꼭지점(정점)이 8개이다.
	m_nVertices = 8;
	m_nStride = sizeof(CDiffusedVertex);
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	float fx = fWidth * 0.5f, fy = fHeight * 0.5f, fz = fDepth * 0.5f;
	//정점 버퍼는 직육면체의 꼭지점 8개에 대한 정점 데이터를 가진다.
	CDiffusedVertex pVertices[8];
	pVertices[0] = CDiffusedVertex(XMFLOAT3(-fx, +fy, -fz), RANDOM_COLOR);
	pVertices[1] = CDiffusedVertex(XMFLOAT3(+fx, +fy, -fz), RANDOM_COLOR);
	pVertices[2] = CDiffusedVertex(XMFLOAT3(+fx, +fy, +fz), RANDOM_COLOR);
	pVertices[3] = CDiffusedVertex(XMFLOAT3(-fx, +fy, +fz), RANDOM_COLOR);
	pVertices[4] = CDiffusedVertex(XMFLOAT3(-fx, -fy, -fz), RANDOM_COLOR);
	pVertices[5] = CDiffusedVertex(XMFLOAT3(+fx, -fy, -fz), RANDOM_COLOR);
	pVertices[6] = CDiffusedVertex(XMFLOAT3(+fx, -fy, +fz), RANDOM_COLOR);
	pVertices[7] = CDiffusedVertex(XMFLOAT3(-fx, -fy, +fz), RANDOM_COLOR);
	m_pd3dVertexBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pVertices, m_nStride * m_nVertices, 
		D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dVertexUploadBuffer);
	m_d3dVertexBufferView.BufferLocation = m_pd3dVertexBuffer->GetGPUVirtualAddress();
	m_d3dVertexBufferView.StrideInBytes = m_nStride;
	m_d3dVertexBufferView.SizeInBytes = m_nStride * m_nVertices;
	/*인덱스 버퍼는 직육면체의 6개의 면(사각형)에 대한 기하 정보를 갖는다. 삼각형 리스트로 직육면체를 표현할 것이
	므로 각 면은 2개의 삼각형을 가지고 각 삼각형은 3개의 정점이 필요하다. 즉, 인덱스 버퍼는 전체 36(=6*2*3)개의 인
	덱스를 가져야 한다.*/
	m_nIndices = 36;
	UINT pnIndices[36];
	//ⓐ 앞면(Front) 사각형의 위쪽 삼각형
	pnIndices[0] = 3; pnIndices[1] = 1; pnIndices[2] = 0;
	//ⓑ 앞면(Front) 사각형의 아래쪽 삼각형
	pnIndices[3] = 2; pnIndices[4] = 1; pnIndices[5] = 3;
	//ⓒ 윗면(Top) 사각형의 위쪽 삼각형
	pnIndices[6] = 0; pnIndices[7] = 5; pnIndices[8] = 4;
	//ⓓ 윗면(Top) 사각형의 아래쪽 삼각형
	pnIndices[9] = 1; pnIndices[10] = 5; pnIndices[11] = 0;
	//ⓔ 뒷면(Back) 사각형의 위쪽 삼각형
	pnIndices[12] = 3; pnIndices[13] = 4; pnIndices[14] = 7;
	//ⓕ 뒷면(Back) 사각형의 아래쪽 삼각형
	pnIndices[15] = 0; pnIndices[16] = 4; pnIndices[17] = 3;
	//ⓖ 아래면(Bottom) 사각형의 위쪽 삼각형
	pnIndices[18] = 1; pnIndices[19] = 6; pnIndices[20] = 5;
	//ⓗ 아래면(Bottom) 사각형의 아래쪽 삼각형
	pnIndices[21] = 2; pnIndices[22] = 6; pnIndices[23] = 1;
	//ⓘ 옆면(Left) 사각형의 위쪽 삼각형
	pnIndices[24] = 2; pnIndices[25] = 7; pnIndices[26] = 6;
	//ⓙ 옆면(Left) 사각형의 아래쪽 삼각형
	pnIndices[27] = 3; pnIndices[28] = 7; pnIndices[29] = 2;
	//ⓚ 옆면(Right) 사각형의 위쪽 삼각형
	pnIndices[30] = 6; pnIndices[31] = 4; pnIndices[32] = 5;
	//ⓛ 옆면(Right) 사각형의 아래쪽 삼각형
	pnIndices[33] = 7; pnIndices[34] = 4; pnIndices[35] = 6;
	//인덱스 버퍼를 생성한다.
	m_pd3dIndexBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pnIndices, sizeof(UINT) * m_nIndices, 
		D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, &m_pd3dIndexUploadBuffer);
	//인덱스 버퍼 뷰를 생성한다.
	m_d3dIndexBufferView.BufferLocation = m_pd3dIndexBuffer->GetGPUVirtualAddress();
	m_d3dIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_d3dIndexBufferView.SizeInBytes = sizeof(UINT) * m_nIndices;
}

CCubeMeshDiffused::~CCubeMeshDiffused() {}


// 비행기 메쉬 생성
CAirplaneMeshDiffused::CAirplaneMeshDiffused(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
	float fWidth, float fHeight, float fDepth, XMFLOAT4 xmf4Color) : CMesh(pd3dDevice, pd3dCommandList) {
	m_nVertices = 24 * 3;
	m_nStride = sizeof(CDiffusedVertex);
	m_nOffset = 0;
	m_nSlot = 0;
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	float fx = fWidth * 0.5f, fy = fHeight * 0.5f, fz = fDepth * 0.5f;
	//위의 그림과 같은 비행기 메쉬를 표현하기 위한 정점 데이터이다.
	CDiffusedVertex pVertices[24 * 3];
	float x1 = fx * 0.2f, y1 = fy * 0.2f, x2 = fx * 0.1f, y3 = fy * 0.3f, y2 = ((y1 - (fy - y3)) / x1) * x2 + (fy - y3);
	int i = 0;
	//비행기 메쉬의 위쪽 면
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(0.0f, +(fy + y3), -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(+x1, -y1, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(0.0f, 0.0f, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(0.0f, +(fy + y3), -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(0.0f, 0.0f, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(-x1, -y1, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(+x2, +y2, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(+fx, -y3, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(+x1, -y1, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(-x2, +y2, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(-x1, -y1, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(-fx, -y3, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	//비행기 메쉬의 아래쪽 면
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(0.0f, +(fy + y3), +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(0.0f, 0.0f, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(+x1, -y1, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(0.0f, +(fy + y3), +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(-x1, -y1, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(0.0f, 0.0f, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(+x2, +y2, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(+x1, -y1, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(+fx, -y3, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(-x2, +y2, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(-fx, -y3, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(-x1, -y1, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	//비행기 메쉬의 오른쪽 면
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(0.0f, +(fy + y3), -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(0.0f, +(fy + y3), +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(+x2, +y2, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(+x2, +y2, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(0.0f, +(fy + y3), +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(+x2, +y2, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(+x2, +y2, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(+x2, +y2, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(+fx, -y3, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(+fx, -y3, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(+x2, +y2, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(+fx, -y3, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	//비행기 메쉬의 뒤쪽/오른쪽 면
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(+x1, -y1, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(+fx, -y3, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(+fx, -y3, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(+x1, -y1, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(+fx, -y3, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(+x1, -y1, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(0.0f, 0.0f, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(+x1, -y1, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(+x1, -y1, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(0.0f, 0.0f, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(+x1, -y1, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(0.0f, 0.0f, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	//비행기 메쉬의 왼쪽 면
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(0.0f, +(fy + y3), +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(0.0f, +(fy + y3), -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(-x2, +y2, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(0.0f, +(fy + y3), +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(-x2, +y2, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(-x2, +y2, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(-x2, +y2, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(-x2, +y2, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(-fx, -y3, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(-x2, +y2, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(-fx, -y3, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(-fx, -y3, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	//비행기 메쉬의 뒤쪽/왼쪽 면
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(0.0f, 0.0f, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(0.0f, 0.0f, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR)); 
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(-x1, -y1, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(0.0f, 0.0f, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(-x1, -y1, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(-x1, -y1, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(-x1, -y1, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(-x1, -y1, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(-fx, -y3, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(-x1, -y1, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(-fx, -y3, +fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	pVertices[i++] = CDiffusedVertex(XMFLOAT3(-fx, -y3, -fz), Vector4::Add(xmf4Color, RANDOM_COLOR));
	m_pd3dVertexBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pVertices, m_nStride * m_nVertices,
		D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dVertexUploadBuffer);
	m_d3dVertexBufferView.BufferLocation = m_pd3dVertexBuffer->GetGPUVirtualAddress();
	m_d3dVertexBufferView.StrideInBytes = m_nStride;
	m_d3dVertexBufferView.SizeInBytes = m_nStride * m_nVertices;
}

CAirplaneMeshDiffused::~CAirplaneMeshDiffused() {}


//=============================================================================
// FBX SDK를 사용한 애니메이션 지원 FBX 메쉬 생성
//=============================================================================
CFBXMeshDiffused::CFBXMeshDiffused(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
	const char* pstrFileName, float fScale, XMFLOAT4 xmf4Color) : CMesh(pd3dDevice, pd3dCommandList) {

	m_pd3dDevice = pd3dDevice;
	m_pd3dCommandList = pd3dCommandList;
	m_fScale = fScale;

	OutputDebugStringA("=== CFBXMeshDiffused Constructor START (FBX SDK with Animation) ===\n");
	OutputDebugStringA("File: ");
	OutputDebugStringA(pstrFileName);
	OutputDebugStringA("\n");

	// FBX SDK Manager 생성
	FbxManager* pFbxManager = FbxManager::Create();
	if (!pFbxManager) {
		OutputDebugStringA("Error: Unable to create FBX Manager!\n");
		return;
	}

	// IOSettings 생성
	FbxIOSettings* pIOSettings = FbxIOSettings::Create(pFbxManager, IOSROOT);
	pFbxManager->SetIOSettings(pIOSettings);

	// Importer 생성
	FbxImporter* pImporter = FbxImporter::Create(pFbxManager, "");
	if (!pImporter->Initialize(pstrFileName, -1, pFbxManager->GetIOSettings())) {
		OutputDebugStringA("Error: FBX Importer Initialize failed!\n");
		OutputDebugStringA(pImporter->GetStatus().GetErrorString());
		OutputDebugStringA("\n");
		pFbxManager->Destroy();
		return;
	}

	// Scene 생성 및 Import
	FbxScene* pScene = FbxScene::Create(pFbxManager, "myScene");
	pImporter->Import(pScene);
	pImporter->Destroy();

	OutputDebugStringA("FBX Scene loaded successfully\n");

	// 삼각형화 (Triangulate)
	FbxGeometryConverter geometryConverter(pFbxManager);
	geometryConverter.Triangulate(pScene, true);

	// 메쉬 처리
	std::vector<CDiffusedVertex> vertices;
	std::vector<UINT> indices;

	FbxNode* pRootNode = pScene->GetRootNode(); // <--- FIX: pRootNode 정의 추가

	if (pRootNode) {
		for (int i = 0; i < pRootNode->GetChildCount(); i++) {
			ProcessNode(pRootNode->GetChild(i), vertices, indices, fScale, xmf4Color);
		}
	}
	// FBX Manager 정리
	pFbxManager->Destroy();

	if (vertices.empty()) {
		return;
	}

	m_nVertices = (UINT)vertices.size();
	m_nStride = sizeof(CDiffusedVertex);
	m_nOffset = 0;
	m_nSlot = 0;
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// 정점 버퍼 생성
	m_pd3dVertexBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, vertices.data(),
		m_nStride * m_nVertices, D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dVertexUploadBuffer);

	m_d3dVertexBufferView.BufferLocation = m_pd3dVertexBuffer->GetGPUVirtualAddress();
	m_d3dVertexBufferView.StrideInBytes = m_nStride;
	m_d3dVertexBufferView.SizeInBytes = m_nStride * m_nVertices;

	// 인덱스 버퍼 생성
	if (!indices.empty()) {
		m_nIndices = (UINT)indices.size();
		m_pd3dIndexBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, indices.data(),
			sizeof(UINT) * m_nIndices, D3D12_HEAP_TYPE_DEFAULT,
			D3D12_RESOURCE_STATE_INDEX_BUFFER, &m_pd3dIndexUploadBuffer);

		m_d3dIndexBufferView.BufferLocation = m_pd3dIndexBuffer->GetGPUVirtualAddress();
		m_d3dIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
		m_d3dIndexBufferView.SizeInBytes = sizeof(UINT) * m_nIndices;
	}
}

CFBXMeshDiffused::~CFBXMeshDiffused() {}

void CFBXMeshDiffused::ProcessNode(FbxNode* pNode, std::vector<CDiffusedVertex>& vertices, 
	std::vector<UINT>& indices, float fScale, XMFLOAT4& xmf4Color) {
	
	FbxMesh* pMesh = pNode->GetMesh();
	if (pMesh) {
		ProcessMesh(pMesh, vertices, indices, fScale, xmf4Color);
	}
	
	for (int i = 0; i < pNode->GetChildCount(); i++) {
		ProcessNode(pNode->GetChild(i), vertices, indices, fScale, xmf4Color);
	}
}

void CFBXMeshDiffused::ProcessMesh(FbxMesh* pMesh, std::vector<CDiffusedVertex>& vertices, 
	std::vector<UINT>& indices, float fScale, XMFLOAT4& xmf4Color) {
	
	UINT baseVertex = (UINT)vertices.size();
	
	FbxVector4* pControlPoints = pMesh->GetControlPoints();
	FbxGeometryElementVertexColor* pVertexColor = pMesh->GetElementVertexColor();
	
	int nPolygonCount = pMesh->GetPolygonCount();
	int vertexCounter = 0;
	
	for (int i = 0; i < nPolygonCount; i++) {
		int nPolygonSize = pMesh->GetPolygonSize(i);
		
		for (int j = 0; j < nPolygonSize; j++) {
			int nControlPointIndex = pMesh->GetPolygonVertex(i, j);
			
			XMFLOAT3 position;
			position.x = (float)pControlPoints[nControlPointIndex][0] * fScale;
			position.y = (float)pControlPoints[nControlPointIndex][1] * fScale;
			position.z = (float)pControlPoints[nControlPointIndex][2] * fScale;
			
			XMFLOAT4 color = xmf4Color;
			if (pVertexColor) {
				FbxColor fbxColor;
				switch (pVertexColor->GetMappingMode()) {
				case FbxGeometryElement::eByControlPoint:
					if (pVertexColor->GetReferenceMode() == FbxGeometryElement::eDirect) {
						fbxColor = pVertexColor->GetDirectArray().GetAt(nControlPointIndex);
					}
					else if (pVertexColor->GetReferenceMode() == FbxGeometryElement::eIndexToDirect) {
						int id = pVertexColor->GetIndexArray().GetAt(nControlPointIndex);
						fbxColor = pVertexColor->GetDirectArray().GetAt(id);
					}
					break;
				case FbxGeometryElement::eByPolygonVertex:
					if (pVertexColor->GetReferenceMode() == FbxGeometryElement::eDirect) {
						fbxColor = pVertexColor->GetDirectArray().GetAt(vertexCounter);
					}
					else if (pVertexColor->GetReferenceMode() == FbxGeometryElement::eIndexToDirect) {
						int id = pVertexColor->GetIndexArray().GetAt(vertexCounter);
						fbxColor = pVertexColor->GetDirectArray().GetAt(id);
					}
					break;
				}
				color.x = (float)fbxColor.mRed;
				color.y = (float)fbxColor.mGreen;
				color.z = (float)fbxColor.mBlue;
				color.w = (float)fbxColor.mAlpha;
			}
			
			vertices.push_back(CDiffusedVertex(position, color));
			indices.push_back(baseVertex + vertexCounter);
			vertexCounter++;
		}
	}
}
//=============================================================================
// FBX SDK를 사용한 UV 좌표 지원 FBX 메쉬 생성
//=============================================================================
CFBXMeshTextured::CFBXMeshTextured(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
	const char* pstrFileName, float fScale) : CMesh(pd3dDevice, pd3dCommandList) {

	m_pd3dDevice = pd3dDevice;
	m_pd3dCommandList = pd3dCommandList;
	m_fScale = fScale;

	OutputDebugStringA("=== CFBXMeshTextured Constructor START ===\n");
	OutputDebugStringA("File: ");
	OutputDebugStringA(pstrFileName);
	OutputDebugStringA("\n");

	FbxManager* pFbxManager = FbxManager::Create();
	if (!pFbxManager) {
		OutputDebugStringA("Error: Unable to create FBX Manager!\n");
		return;
	}

	FbxIOSettings* pIOSettings = FbxIOSettings::Create(pFbxManager, IOSROOT);
	pFbxManager->SetIOSettings(pIOSettings);

	FbxImporter* pImporter = FbxImporter::Create(pFbxManager, "");
	if (!pImporter->Initialize(pstrFileName, -1, pFbxManager->GetIOSettings())) {
		OutputDebugStringA("Error: FBX Importer Initialize failed!\n");
		OutputDebugStringA(pImporter->GetStatus().GetErrorString());
		OutputDebugStringA("\n");
		pFbxManager->Destroy();
		return;
	}

	FbxScene* pScene = FbxScene::Create(pFbxManager, "myScene");
	pImporter->Import(pScene);
	pImporter->Destroy();

	OutputDebugStringA("FBX Scene loaded successfully\n");

	FbxGeometryConverter geometryConverter(pFbxManager);
	geometryConverter.Triangulate(pScene, true);

	std::vector<CTexturedNormalVertex> vertices;
	std::vector<UINT> indices;

	FbxNode* pRootNode = pScene->GetRootNode();
	if (pRootNode) {
		for (int i = 0; i < pRootNode->GetChildCount(); i++) {
			ProcessNode(pRootNode->GetChild(i), vertices, indices, fScale);
		}
	}

	pFbxManager->Destroy();

	if (vertices.empty()) {
		OutputDebugStringA("Warning: No vertices loaded from FBX!\n");
		return;
	}

	char buffer[256];
	sprintf_s(buffer, "Loaded %zu vertices, %zu indices, %zu submeshes\n", 
		vertices.size(), indices.size(), m_vSubMeshes.size());
	OutputDebugStringA(buffer);

	// 서브메쉬 정보 출력
	for (size_t i = 0; i < m_vSubMeshes.size(); i++) {
		sprintf_s(buffer, "  SubMesh[%zu]: %s (start=%u, count=%u)\n", 
			i, m_vSubMeshes[i].strName.c_str(), m_vSubMeshes[i].nStartIndex, m_vSubMeshes[i].nIndexCount);
		OutputDebugStringA(buffer);
	}

	m_nVertices = (UINT)vertices.size();
	m_nStride = sizeof(CTexturedNormalVertex);
	m_nOffset = 0;
	m_nSlot = 0;
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	m_pd3dVertexBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, vertices.data(),
		m_nStride * m_nVertices, D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dVertexUploadBuffer);

	m_d3dVertexBufferView.BufferLocation = m_pd3dVertexBuffer->GetGPUVirtualAddress();
	m_d3dVertexBufferView.StrideInBytes = m_nStride;
	m_d3dVertexBufferView.SizeInBytes = m_nStride * m_nVertices;

	if (!indices.empty()) {
		m_nIndices = (UINT)indices.size();
		m_pd3dIndexBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, indices.data(),
			sizeof(UINT) * m_nIndices, D3D12_HEAP_TYPE_DEFAULT,
			D3D12_RESOURCE_STATE_INDEX_BUFFER, &m_pd3dIndexUploadBuffer);

		m_d3dIndexBufferView.BufferLocation = m_pd3dIndexBuffer->GetGPUVirtualAddress();
		m_d3dIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
		m_d3dIndexBufferView.SizeInBytes = sizeof(UINT) * m_nIndices;
	}

	OutputDebugStringA("=== CFBXMeshTextured Constructor END ===\n");
}

CFBXMeshTextured::~CFBXMeshTextured() {}

void CFBXMeshTextured::ProcessNode(FbxNode* pNode, std::vector<CTexturedNormalVertex>& vertices,
	std::vector<UINT>& indices, float fScale) {

	FbxMesh* pMesh = pNode->GetMesh();
	if (pMesh) {
		ProcessMesh(pNode, pMesh, vertices, indices, fScale);
	}

	for (int i = 0; i < pNode->GetChildCount(); i++) {
		ProcessNode(pNode->GetChild(i), vertices, indices, fScale);
	}
}

void CFBXMeshTextured::ProcessMesh(FbxNode* pNode, FbxMesh* pMesh, std::vector<CTexturedNormalVertex>& vertices,
	std::vector<UINT>& indices, float fScale) {

	// 서브메쉬 정보 생성
	SubMeshInfo subMesh;
	subMesh.strName = pNode->GetName();
	subMesh.nStartIndex = (UINT)indices.size();
	subMesh.nMaterialIndex = -1;

	char buffer[256];
	sprintf_s(buffer, "Processing mesh: %s\n", subMesh.strName.c_str());
	OutputDebugStringA(buffer);

	UINT baseVertex = (UINT)vertices.size();
	FbxVector4* pControlPoints = pMesh->GetControlPoints();

	int nPolygonCount = pMesh->GetPolygonCount();
	int vertexCounter = 0;

	for (int i = 0; i < nPolygonCount; i++) {
		int nPolygonSize = pMesh->GetPolygonSize(i);

		for (int j = 0; j < nPolygonSize; j++) {
			int nControlPointIndex = pMesh->GetPolygonVertex(i, j);

			// 위치 추출
			XMFLOAT3 position;
			position.x = (float)pControlPoints[nControlPointIndex][0] * fScale;
			position.y = (float)pControlPoints[nControlPointIndex][1] * fScale;
			position.z = (float)pControlPoints[nControlPointIndex][2] * fScale;

			// 노멀 추출
			XMFLOAT3 normal = GetNormal(pMesh, nControlPointIndex, vertexCounter);

			// UV 좌표 추출
			XMFLOAT2 texCoord = GetUV(pMesh, nControlPointIndex, vertexCounter, 0);

			vertices.push_back(CTexturedNormalVertex(position, normal, texCoord));
			indices.push_back(baseVertex + vertexCounter);
			vertexCounter++;
		}
	}

	subMesh.nIndexCount = (UINT)indices.size() - subMesh.nStartIndex;
	m_vSubMeshes.push_back(subMesh);

	sprintf_s(buffer, "  -> %d polygons, %u indices\n", nPolygonCount, subMesh.nIndexCount);
	OutputDebugStringA(buffer);
}

XMFLOAT2 CFBXMeshTextured::GetUV(FbxMesh* pMesh, int nControlPointIndex, int nVertexCounter, int nUVIndex) {
	XMFLOAT2 texCoord = XMFLOAT2(0.0f, 0.0f);

	if (pMesh->GetElementUVCount() <= nUVIndex) {
		return texCoord;
	}

	FbxGeometryElementUV* pUV = pMesh->GetElementUV(nUVIndex);
	if (!pUV) {
		return texCoord;
	}

	FbxVector2 fbxUV;

	switch (pUV->GetMappingMode()) {
	case FbxGeometryElement::eByControlPoint:
		switch (pUV->GetReferenceMode()) {
		case FbxGeometryElement::eDirect:
			fbxUV = pUV->GetDirectArray().GetAt(nControlPointIndex);
			break;
		case FbxGeometryElement::eIndexToDirect:
			{
				int id = pUV->GetIndexArray().GetAt(nControlPointIndex);
				fbxUV = pUV->GetDirectArray().GetAt(id);
			}
			break;
		default:
			break;
		}
		break;

	case FbxGeometryElement::eByPolygonVertex:
		switch (pUV->GetReferenceMode()) {
		case FbxGeometryElement::eDirect:
			fbxUV = pUV->GetDirectArray().GetAt(nVertexCounter);
			break;
		case FbxGeometryElement::eIndexToDirect:
			{
				int id = pUV->GetIndexArray().GetAt(nVertexCounter);
				fbxUV = pUV->GetDirectArray().GetAt(id);
			}
			break;
		default:
			break;
		}
		break;

	default:
		break;
	}

	texCoord.x = (float)fbxUV[0];
	texCoord.y = 1.0f - (float)fbxUV[1];  // DirectX는 V좌표가 반전됨

	return texCoord;
}

XMFLOAT3 CFBXMeshTextured::GetNormal(FbxMesh* pMesh, int nControlPointIndex, int nVertexCounter) {
	XMFLOAT3 normal = XMFLOAT3(0.0f, 1.0f, 0.0f);

	if (pMesh->GetElementNormalCount() < 1) {
		return normal;
	}

	FbxGeometryElementNormal* pNormal = pMesh->GetElementNormal(0);
	if (!pNormal) {
		return normal;
	}

	FbxVector4 fbxNormal;

	switch (pNormal->GetMappingMode()) {
	case FbxGeometryElement::eByControlPoint:
		switch (pNormal->GetReferenceMode()) {
		case FbxGeometryElement::eDirect:
			fbxNormal = pNormal->GetDirectArray().GetAt(nControlPointIndex);
			break;
		case FbxGeometryElement::eIndexToDirect:
			{
				int id = pNormal->GetIndexArray().GetAt(nControlPointIndex);
				fbxNormal = pNormal->GetDirectArray().GetAt(id);
			}
			break;
		default:
			break;
		}
		break;

	case FbxGeometryElement::eByPolygonVertex:
		switch (pNormal->GetReferenceMode()) {
		case FbxGeometryElement::eDirect:
			fbxNormal = pNormal->GetDirectArray().GetAt(nVertexCounter);
			break;
		case FbxGeometryElement::eIndexToDirect:
			{
				int id = pNormal->GetIndexArray().GetAt(nVertexCounter);
				fbxNormal = pNormal->GetDirectArray().GetAt(id);
			}
			break;
		default:
			break;
		}
		break;

	default:
		break;
	}

	normal.x = (float)fbxNormal[0];
	normal.y = (float)fbxNormal[1];
	normal.z = (float)fbxNormal[2];

	return normal;
}