#pragma once

#include <string>
#include <vector>
#include <d3d12.h>
#include <wrl\client.h>
#include "StructuredBuffer.h"

typedef std::shared_ptr<class NeuralModel> NeuralModelPtr;

struct float4
{
	union
	{
		struct
		{
			float x, y, z, w;
		};
		float data[4];
	};
};

class NeuralModel
{
public:
	std::vector<float> weights;
	std::vector<float> bias;

	std::vector<int32_t> layer_sizes;

	StructuredBufferPtr weightBuffer;
	StructuredBufferPtr biasBuffer;

public:
	static NeuralModelPtr LoadModel(const std::string& modelPath);

	void CreateBuffers(class D3D12GraphicsDevice& device);
};

