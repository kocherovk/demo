#include "Monkey.h"
#include <Mesh.h>
#include <vector>
#include "headlers\Camera.h"

using namespace std;
using namespace demo;

#define FIELD_WIDTH  150
#define FIELD_HEIGHT 150
#define MONKEY_WIDTH 5
#define MONKEY_HEIGHT 5
#define ROW_COUNT 30
#define COLUMN_COUNT 30

Monkey::Monkey(ID3D11Device* device)
{
	shader = Shader(L"shaders/TextureShader.fx");
	
	vector<D3D11_INPUT_ELEMENT_DESC> layout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 4 * sizeof(float), D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8 * sizeof(float), D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	
	shader.SetInputLayout(layout);
	shader.Compile(device);

	Mesh mesh = loadMeshFromFile_ply("models/slender.ply");

	_verticesCount = mesh.verts_count;
	_indicesCount = mesh.inds_count;

	Vertex* vertices = new Vertex [_verticesCount];
	for (int i = 0; i < _verticesCount; i++)
		vertices[i] = {mesh.coords[i], mesh.normals[i], mesh.uvs[i]};

	_position = XMFLOAT3(0, 0.0f ,0);

	D3D11_BUFFER_DESC bd;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory(&bd, sizeof(bd));
	ZeroMemory(&InitData, sizeof(InitData));

	bd.CPUAccessFlags = 0;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(Vertex) * _verticesCount;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	InitData.pSysMem = vertices;
	device->CreateBuffer(&bd, &InitData, &_vertexBuffer);

	bd.ByteWidth = sizeof(WORD) * _indicesCount;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	InitData.pSysMem = mesh.inds;
	device->CreateBuffer(&bd, &InitData, &_indexBuffer);

	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.CPUAccessFlags = 0;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	device->CreateBuffer(&bd, NULL, &_dataBuffer);

	HRESULT hr = D3DX11CreateShaderResourceViewFromFile(device, L"models/slenderTexture.png", NULL, NULL, &resourceView, NULL);

	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	device->CreateSamplerState(&sampDesc, &linearSampler);

	update();
}

Monkey::Monkey() {};

void Monkey::setLocation(XMFLOAT3 location)
{
	_position = location;
}

void Monkey::update()
{
	long t = GetTickCount();
	float delta = 2e-3;
	float dt = (t - _t);
	_position = XMFLOAT3(0, 0, 0);
	_t = t;
}

void Monkey::setShaders(ShaderMaster sm)
{
	_pixelShader = sm.getPixelShader("Texture");
	_vertexShader = sm.getVertexShader("Texture");
}

void Monkey::drawAnother(ID3D11DeviceContext* deviceContext, const ObservingCamera &camera, int i, int j)
{
	float x = _position.x - i * MONKEY_WIDTH;
	float y = _position.y;
	float z = _position.z + j * MONKEY_HEIGHT;

	z = z > FIELD_HEIGHT / 2 ? z - FIELD_HEIGHT : z;


	ConstantBuffer cb;
	cb.world = XMMatrixTranspose(XMMatrixTranslation(x, y, z));
	cb.view = XMMatrixTranspose(camera.getViewMatrix());
	cb.projection = XMMatrixTranspose(camera.getProjectionMatrix());
	cb.color = XMFLOAT4(0.58f, 0.29f, 0.0f, 1.0f);
	cb.lightPos = camera.getPosition();

	deviceContext->UpdateSubresource(_dataBuffer, 0, NULL, &cb, 0, 0);
	deviceContext->DrawIndexed(_indicesCount, 0, 0);
}

void Monkey::render(ID3D11DeviceContext* deviceContext, const ObservingCamera &camera) 
{
	shader.Apply(deviceContext);

	
	//deviceContext->VSSetShader(_vertexShader, NULL, 0);
	deviceContext->VSSetConstantBuffers(0, 1, &_dataBuffer);
	//deviceContext->PSSetShader(_pixelShader, NULL, 0);
	deviceContext->PSSetConstantBuffers(0, 1, &_dataBuffer);

	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	deviceContext->IASetVertexBuffers(0, 1, &_vertexBuffer, &stride, &offset);
	deviceContext->IASetIndexBuffer(_indexBuffer, DXGI_FORMAT_R16_UINT, 0);
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ConstantBuffer cb;
	cb.world = XMMatrixTranspose(XMMatrixTranslation(_position.x, _position.y, _position.z));
	cb.view = XMMatrixTranspose(camera.getViewMatrix());
	cb.projection = XMMatrixTranspose(camera.getProjectionMatrix());
	cb.color = XMFLOAT4(0.58f, 0.29f, 0.0f, 1.0f);
	cb.lightPos = camera.getPosition();

	deviceContext->UpdateSubresource(_dataBuffer, 0, NULL, &cb, 0, 0);
	deviceContext->PSSetShaderResources(0, 1, &resourceView);
	deviceContext->PSSetSamplers(0, 1, &linearSampler);
	deviceContext->DrawIndexed(_indicesCount, 0, 0);
}