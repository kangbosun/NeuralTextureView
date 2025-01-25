#include "NeuralModel.h"
#include <fstream>
#include "nlohmann/json.hpp"
#include <iostream>
#include "Graphics.h"
#include "d3dx12.h"

NeuralModelPtr NeuralModel::LoadModel(const std::string& modelPath)
{
	auto model = std::make_shared<NeuralModel>();

	using Json = nlohmann::json;
	using namespace std;

	//check file exists
	if (!std::filesystem::exists(modelPath))
	{
		throw std::runtime_error("Model file not found");
	}

	std::ifstream file(modelPath);
	
	Json model_json = Json::parse(file);

	//print hierarchy
	std::cout << "Model hierarchy:\n";

	int32_t num_layers = model_json["num_layers"];


	cout << "Number of layers: " << num_layers << "\n";

	for (int32_t i = 0; i < num_layers; i++)
	{
		string layer_name = "layer" + to_string(i);
		string layer_type = model_json[layer_name]["name"];
		cout << "Layer " << i << " type: " << layer_type << "\n";

		if (layer_type == "Conv2d")
		{
			int32_t in_channels = model_json[layer_name]["in_channels"];
			int32_t out_channels = model_json[layer_name]["out_channels"];

			cout << "--In channels: " << in_channels << "\n";
			cout << "--Out channels: " << out_channels << "\n";

			model->layer_sizes.push_back(in_channels);

			if (i == num_layers - 1)
			{
				model->layer_sizes.push_back(out_channels);
			}


			vector<float> weights = (model_json[layer_name]["weight"]);
			vector<float> bias = (model_json[layer_name]["bias"]);

			model->weights.insert(model->weights.end(), weights.begin(), weights.end());
			model->bias.insert(model->bias.end(), bias.begin(), bias.end());
		}
	}

    return model;
}

void NeuralModel::CreateBuffers(D3D12GraphicsDevice& device)
{
	weightBuffer = std::make_shared<StructuredBuffer>();
	weightBuffer->Initialize(device, weights.data(), sizeof(float), weights.size());

	biasBuffer = std::make_shared<StructuredBuffer>();
	biasBuffer->Initialize(device, bias.data(), sizeof(float), bias.size());
}
