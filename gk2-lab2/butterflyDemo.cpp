#include "butterflyDemo.h"

using namespace mini;
using namespace gk2;
using namespace DirectX;
using namespace std;

#pragma region Constants
const float ButterflyDemo::DODECAHEDRON_R = sqrtf(0.375f + 0.125f * sqrtf(5.0f));
const float ButterflyDemo::DODECAHEDRON_H = 1.0f + 2.0f * DODECAHEDRON_R;
const float ButterflyDemo::DODECAHEDRON_A = XMScalarACos(-0.2f * sqrtf(5.0f));

const float ButterflyDemo::MOEBIUS_R = 1.0f;
const float ButterflyDemo::MOEBIUS_W = 0.1f;
const unsigned int ButterflyDemo::MOEBIUS_N = 128;

const float ButterflyDemo::LAP_TIME = 10.0f;
const float ButterflyDemo::FLAP_TIME = 2.0f;
const float ButterflyDemo::WING_W = 0.15f;
const float ButterflyDemo::WING_H = 0.1f;
const float ButterflyDemo::WING_MAX_A = 8.0f * XM_PIDIV2 / 9.0f; //80 degrees

const unsigned int ButterflyDemo::BS_MASK = 0xffffffff;

const XMFLOAT4 ButterflyDemo::GREEN_LIGHT_POS = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
const XMFLOAT4 ButterflyDemo::BLUE_LIGHT_POS = XMFLOAT4(-1.0f, -1.0f, -1.0f, 1.0f);
const XMFLOAT4 ButterflyDemo::COLORS[] = {
	XMFLOAT4(253.0f / 255.0f, 198.0f / 255.0f, 137.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(255.0f / 255.0f, 247.0f / 255.0f, 153.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(196.0f / 255.0f, 223.0f / 255.0f, 155.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(162.0f / 255.0f, 211.0f / 255.0f, 156.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(130.0f / 255.0f, 202.0f / 255.0f, 156.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(122.0f / 255.0f, 204.0f / 255.0f, 200.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(109.0f / 255.0f, 207.0f / 255.0f, 246.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(125.0f / 255.0f, 167.0f / 255.0f, 216.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(131.0f / 255.0f, 147.0f / 255.0f, 202.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(135.0f / 255.0f, 129.0f / 255.0f, 189.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(161.0f / 255.0f, 134.0f / 255.0f, 190.0f / 255.0f, 100.0f / 255.0f),
	XMFLOAT4(244.0f / 255.0f, 154.0f / 255.0f, 193.0f / 255.0f, 100.0f / 255.0f)
};
#pragma endregion

#pragma region Initalization
ButterflyDemo::ButterflyDemo(HINSTANCE hInstance)
	: Base(hInstance, 1280, 720, L"Motyl"),
	  m_cbWorld(m_device.CreateConstantBuffer<XMFLOAT4X4>()),
	  m_cbView(m_device.CreateConstantBuffer<XMFLOAT4X4, 2>()),
	  m_cbLighting(m_device.CreateConstantBuffer<Lighting>()),
	  m_cbSurfaceColor(m_device.CreateConstantBuffer<XMFLOAT4>())

{
	//Projection matrix
	auto s = m_window.getClientSize();
	auto ar = static_cast<float>(s.cx) / s.cy;
	XMStoreFloat4x4(&m_projMtx, XMMatrixPerspectiveFovLH(XM_PIDIV4, ar, 0.01f, 100.0f));
	m_cbProj = m_device.CreateConstantBuffer<XMFLOAT4X4>();
	UpdateBuffer(m_cbProj, m_projMtx);
	XMFLOAT4X4 cameraMtx;
	XMStoreFloat4x4(&cameraMtx, m_camera.getViewMatrix());
	UpdateCameraCB(cameraMtx);

	//Regular shaders
	auto vsCode = m_device.LoadByteCode(L"vs.cso");
	auto psCode = m_device.LoadByteCode(L"ps.cso");
	m_vs = m_device.CreateVertexShader(vsCode);
	m_ps = m_device.CreatePixelShader(psCode);

	//DONE : 0.3. Change input layout to match new vertex structure
	m_il = m_device.CreateInputLayout(VertexPositionNormal::Layout, vsCode);

	//Billboard shaders
	vsCode = m_device.LoadByteCode(L"vsBillboard.cso");
	psCode = m_device.LoadByteCode(L"psBillboard.cso");
	m_vsBillboard = m_device.CreateVertexShader(vsCode);
	m_psBillboard = m_device.CreatePixelShader(psCode);
	D3D11_INPUT_ELEMENT_DESC elements[1] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
	m_ilBillboard = m_device.CreateInputLayout(elements, vsCode);

	//Render states
	CreateRenderStates();

	//Meshes

	//DONE : 0.2. Create a shaded box model instead of colored one 
	m_box = Mesh::ShadedBox(m_device);

	m_pentagon = Mesh::Pentagon(m_device);
	m_wing = Mesh::DoubleRect(m_device, WING_W, WING_H);
	CreateMoebuisStrip();

	m_bilboard = Mesh::Billboard(m_device, 2.0f);

	//Model matrices
	CreateDodecahadronMtx();

	SetShaders();
	ID3D11Buffer* vsb[] = { m_cbWorld.get(),  m_cbView.get(), m_cbProj.get() };
	m_device.context()->VSSetConstantBuffers(0, 3, vsb);
	ID3D11Buffer* psb[] = { m_cbSurfaceColor.get(), m_cbLighting.get() };
	m_device.context()->PSSetConstantBuffers(0, 2, psb);
}

void ButterflyDemo::CreateRenderStates()
//Setup render states used in various stages of the scene rendering
{
	DepthStencilDescription dssDesc;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; //Disable writing to depth buffer
	m_dssNoDepthWrite = m_device.CreateDepthStencilState(dssDesc); // depth stencil state for billboards

	//TODO : 1.20. Setup depth stencil state for writing to stencil buffer

	m_dssStencilWrite = m_device.CreateDepthStencilState(dssDesc);

	//TODO : 1.36. Setup depth stencil state for stencil test for billboards

	m_dssStencilTestNoDepthWrite = m_device.CreateDepthStencilState(dssDesc);

	//TODO : 1.21. Setup depth stencil state for stencil test for 3D objects

	m_dssStencilTest = m_device.CreateDepthStencilState(dssDesc);

	RasterizerDescription rsDesc;
	//TODO : 1.13. Setup rasterizer state with ccw front faces

	m_rsCCW = m_device.CreateRasterizerState(rsDesc);

	BlendDescription bsDesc;
	//TODO : 1.26. Setup alpha blending state

	m_bsAlpha = m_device.CreateBlendState(bsDesc);

	//TODO : 1.30. Setup additive blending state

	m_bsAdd = m_device.CreateBlendState(bsDesc);
}

void ButterflyDemo::CreateDodecahadronMtx()
//Compute dodecahedronMtx and mirrorMtx
{
    //TODO : 1.01. calculate m_dodecahedronMtx matrices
    XMStoreFloat4x4(&m_dodecahedronMtx[0], XMMatrixRotationX(XM_PIDIV2) * XMMatrixTranslation(0.f, -DODECAHEDRON_H / 2.f, 0.f));
    XMStoreFloat4x4(&m_dodecahedronMtx[1], XMLoadFloat4x4(&m_dodecahedronMtx[0]) * XMMatrixRotationY(XM_PI) * XMMatrixRotationZ(DODECAHEDRON_A - XM_PI));

    for (int i = 2; i <= 5; i++)
    {
        XMStoreFloat4x4(&m_dodecahedronMtx[i], XMLoadFloat4x4(&m_dodecahedronMtx[i - 1]) * XMMatrixRotationY(2.f / 5.f * XM_PI));
    }

    for (int i = 6; i <= 11; i++)
    {
        XMStoreFloat4x4(&m_dodecahedronMtx[i], XMLoadFloat4x4(&m_dodecahedronMtx[i - 6]) * XMMatrixRotationZ(XM_PI));
    }

    //TODO : 1.12. calculate m_mirrorMtx matrices
}

XMFLOAT3 ButterflyDemo::MoebiusStripPos(float t, float s)
//TODO : 1.04. Compute the position of point on the Moebius strip for parameters t and s
{
	return { 
		cosf(t) * (MOEBIUS_R + MOEBIUS_W * s * cosf(0.5f * t)),
		sinf(t) * (MOEBIUS_R + MOEBIUS_W * s * cosf(0.5f * t)),
		MOEBIUS_W * s * sinf(0.5f * t)
	};
}

XMVECTOR ButterflyDemo::MoebiusStripDs(float t, float s)
// TODO: 1.05. Return the s-derivative of point on the Moebius strip for parameters t and s
{
	float dx_ds = MOEBIUS_W * cosf(t) * cosf(0.5f * t);
	float dy_ds = MOEBIUS_W * sinf(t) * cosf(0.5f * t);
	float dz_ds = MOEBIUS_W * sinf(0.5f * t);

	return XMVECTOR{dx_ds, dy_ds, dz_ds, 0.0f};
}

XMVECTOR ButterflyDemo::MoebiusStripDt(float t, float s)
// TODO: 1.06. Compute the t-derivative of point on the Moebius strip for parameters t and s
{
	float dx_dt = -MOEBIUS_R * sinf(t) -0.5f * s * MOEBIUS_W * sinf(0.5f * t) * cosf(t) 
		- MOEBIUS_W * s * cos(0.5f * t) * sinf(t);
	float dy_dt = MOEBIUS_R * cosf(t) - 0.5f * s * MOEBIUS_W * sinf(0.5f * t) * sinf(t)
		+ MOEBIUS_W * s * cos(0.5f * t) * cosf(t);
	float dz_dt = 0.5f * s * MOEBIUS_W * cosf(t);

	return XMVECTOR{ dx_dt, dy_dt, dz_dt, 0.0f };
}

void ButterflyDemo::CreateMoebuisStrip()
{
	std::vector<VertexPositionNormal> vertices;
	std::vector<unsigned short> indices;

	const unsigned int numSegments = MOEBIUS_N;
	const float maxT = 4.0f * XM_PI;
	const float tStep = maxT / numSegments;

	for (unsigned int i = 0; i <= numSegments; ++i)
	{
		float t = i * tStep;
		for (int s_dir = -1; s_dir <= 1; s_dir += 2)
		{
			float s = static_cast<float>(s_dir);
			XMFLOAT3 pos = MoebiusStripPos(t, s);

			XMVECTOR ds = MoebiusStripDs(t, s);
			XMVECTOR dt = MoebiusStripDt(t, s);
			XMVECTOR normal = XMVector3Normalize(XMVector3Cross(ds, dt));

			XMFLOAT3 normalFloat3;
			XMStoreFloat3(&normalFloat3, normal);

			vertices.emplace_back(pos, normalFloat3);
		}
	}

	for (unsigned int i = 0; i < numSegments; ++i)
	{
		unsigned short currentBase = i * 2;
		unsigned short nextBase = (i + 1) * 2;

		indices.push_back(nextBase);
		indices.push_back(currentBase);
		indices.push_back(currentBase + 1);

		indices.push_back(currentBase + 1);
		indices.push_back(nextBase + 1);
		indices.push_back(nextBase);
	}

	m_moebius = Mesh::SimpleTriMesh(m_device, vertices, indices);
}
#pragma endregion

#pragma region Per-Frame Update
void ButterflyDemo::Update(const Clock& c)
{
	Base::Update(c);
	double dt = c.getFrameTime();
	if (HandleCameraInput(dt))
	{
		XMFLOAT4X4 cameraMtx;
		XMStoreFloat4x4(&cameraMtx, m_camera.getViewMatrix());
		UpdateCameraCB(cameraMtx);
	}
	UpdateButterfly(static_cast<float>(dt));
}

void ButterflyDemo::UpdateCameraCB(DirectX::XMFLOAT4X4 cameraMtx)
{
	XMMATRIX mtx = XMLoadFloat4x4(&cameraMtx);
	XMVECTOR det;
	auto invvmtx = XMMatrixInverse(&det, mtx);
	XMFLOAT4X4 view[2] = { cameraMtx };
	XMStoreFloat4x4(view + 1, invvmtx);
	UpdateBuffer(m_cbView, view);
}

void ButterflyDemo::UpdateButterfly(float dtime)
//TODO : 1.10. Compute the matrices for butterfly wings. Position on the strip is determined based on time
{
	using namespace DirectX;

	static float lap = 0.0f;
	lap += dtime;
	while (lap > LAP_TIME)
		lap -= LAP_TIME;
	//Value of the Moebius strip t parameter
	float t = 2 * lap / LAP_TIME;
	//Angle between wing current and vertical position
	float a = t * WING_MAX_A;
	t *= XM_2PI;
	if (a > WING_MAX_A)
		a = 2 * WING_MAX_A - a;
	//Write the rest of code here

	auto b = MoebiusStripDs(t, 0);
	auto t2 = MoebiusStripDt(t, 0);
	auto n = XMVector3Normalize(XMVector3Cross(b, t2));
	auto pos = MoebiusStripPos(t, 0);

	auto matrix = XMMATRIX(XMVector3Normalize(b), XMVector3Normalize(n), XMVector3Normalize(t2), { pos.x, pos.y, pos.z, 1.f });

	XMStoreFloat4x4(&m_wingMtx[0], XMMatrixRotationY(XM_PIDIV2) * XMMatrixRotationZ(a) * matrix);
	XMStoreFloat4x4(&m_wingMtx[1], XMMatrixRotationY(XM_PIDIV2) * XMMatrixRotationZ(-a) * matrix);
}
#pragma endregion

#pragma region Frame Rendering Setup
void ButterflyDemo::SetShaders()
{
	m_device.context()->VSSetShader(m_vs.get(), 0, 0);
	m_device.context()->PSSetShader(m_ps.get(), 0, 0);
	m_device.context()->IASetInputLayout(m_il.get());
	m_device.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void ButterflyDemo::SetBillboardShaders()
{
	m_device.context()->VSSetShader(m_vsBillboard.get(), 0, 0);
	m_device.context()->PSSetShader(m_psBillboard.get(), 0, 0);
	m_device.context()->IASetInputLayout(m_ilBillboard.get());
	m_device.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void ButterflyDemo::Set1Light()
//Setup one positional light at the camera
{
	Lighting l{
		/*.ambientColor = */ XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
		/*.surface = */ XMFLOAT4(0.2f, 0.8f, 0.8f, 200.0f),
		/*.lights =*/ {
			{ /*.position =*/ m_camera.getCameraPosition(), /*.color =*/ XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) }
			//other 2 lights set to 0
		}
	};
	UpdateBuffer(m_cbLighting, l);
}

void ButterflyDemo::Set3Lights()
//Setup one white positional light at the camera
//TODO : 1.28. Setup two additional positional lights, green and blue.
{
	Lighting l{
		/*.ambientColor = */ XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
		/*.surface = */ XMFLOAT4(0.2f, 0.8f, 0.8f, 200.0f),
		/*.lights =*/{
			{ /*.position =*/ m_camera.getCameraPosition(), /*.color =*/ XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f) },
			//Write the rest of the code here

		}
	};

	//comment the following line when structure is filled
	ZeroMemory(&l.lights[1], sizeof(Light) * 2);

	UpdateBuffer(m_cbLighting, l);
}
#pragma endregion

#pragma region Drawing
void ButterflyDemo::DrawBox()
{
	XMFLOAT4X4 worldMtx;
	XMStoreFloat4x4(&worldMtx, XMMatrixIdentity());
	UpdateBuffer(m_cbWorld, worldMtx);
	m_box.Render(m_device.context());
}

void ButterflyDemo::DrawDodecahedron(bool colors)
{
	for (int i = 0; i < 12; i++)
	{
		UpdateBuffer(m_cbWorld, m_dodecahedronMtx[i]);

		if (colors)
			UpdateBuffer(m_cbSurfaceColor, COLORS[i]);
		else
			UpdateBuffer(m_cbSurfaceColor, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));

		m_pentagon.Render(m_device.context());
	}
}


void ButterflyDemo::DrawMoebiusStrip()
{
	XMFLOAT4X4 worldMtx;
	XMStoreFloat4x4(&worldMtx, XMMatrixIdentity());
	UpdateBuffer(m_cbWorld, worldMtx);
	m_moebius.Render(m_device.context());
}

void ButterflyDemo::DrawButterfly()
//TODO : 1.11. Draw the butterfly
{
	XMFLOAT4X4 worldMtx;
	XMStoreFloat4x4(&worldMtx, XMMatrixIdentity());

	for (int i = 0; i < 2; ++i)
	{
		UpdateBuffer(m_cbWorld, m_wingMtx[i]);
		m_wing.Render(m_device.context());
	}
}

void ButterflyDemo::DrawBillboards()
//Setup billboards rendering and draw them
{
	//TODO : 1.33. Setup shaders and blend state

	//TODO : 1.34. Draw both billboards with appropriate colors and transformations

	//TODO : 1.35. Restore rendering state to it's original values

}

void ButterflyDemo::DrawMirroredWorld(unsigned int i)
//Draw the mirrored scene reflected in the i-th dodecahedron face
{
	//TODO : 1.22. Setup render state for writing to the stencil buffer

	//TODO : 1.23. Draw the i-th face

	//TODO : 1.24. Setup depth stencil state for rendering mirrored world

	//TODO : 1.15. Setup rasterizer state and view matrix for rendering the mirrored world

	//TODO : 1.16. Draw 3D objects of the mirrored scene - dodecahedron should be drawn with only one light and no colors and without blending

	//TODO : 1.17. Restore rasterizer state to it's original value

	//TODO : 1.37. Setup depth stencil state for rendering mirrored billboards
	
	//TODO : 1.38. Draw mirrored billboards - they need to be drawn after restoring rasterizer state, but with mirrored view matrix

	//TODO : 1.18. Restore view matrix to its original value

	//TODO : 1.25. Restore depth stencil state to it's original value
}

void ButterflyDemo::Render()
{
	Base::Render();

	//render mirrored worlds
	for (int i = 0; i < 12; ++i)
		DrawMirroredWorld(i);

	//render dodecahedron with one light and alpha blending
	m_device.context()->OMSetBlendState(m_bsAlpha.get(), nullptr, BS_MASK);
	Set1Light();
	//TODO : 1.19. Comment the following line for now
	DrawDodecahedron(true);
	//TODO : 1.27. Uncomment the above line again
	m_device.context()->OMSetBlendState(nullptr, nullptr, BS_MASK);

	//render the rest of the scene with all lights
	Set3Lights();
	UpdateBuffer(m_cbSurfaceColor, XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	//TODO : 1.03. [optional] Comment the following line
	//DrawBox();
	DrawMoebiusStrip();
	DrawButterfly();
	m_device.context()->OMSetDepthStencilState(m_dssNoDepthWrite.get(), 0);
	DrawBillboards();
	m_device.context()->OMSetDepthStencilState(nullptr, 0);
}
#pragma endregion