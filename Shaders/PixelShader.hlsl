

//PixelInput structure
struct PixelInput
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD;
};

#if !USE_NEURAL_TEXTURES
Texture2D AlbeoTexture : register(t0);
SamplerState TextureSampler : register(s0);

//Normal Texture sampler
Texture2D NormalTexture : register(t1);
SamplerState NormalSampler : register(s1);

//AO Texture sampler
Texture2D AOTexture : register(t2);

//Roughness Texture sampler
Texture2D RoughnessTexture : register(t3);
#else
Texture2D FeatureGrid0 : register(t0);
Texture2D FeatureGrid1 : register(t1);
Texture2D FeatureGrid2 : register(t2);
Texture2D FeatureGrid3 : register(t3);

SamplerState TextureSampler : register(s0);
#endif

//cbuffer ConstantBuffer : register(b0)
//{
    float scale = 1.0f;
    float intensity = 1.0f;
    float lightpos = 0.0f;
    float metallic = 0.0f;
//};

float fast_sigmoid(float x)
{
    return x / (1.0f + abs(x));
}

#if USE_NEURAL_TEXTURES
StructuredBuffer<float> nnWeights : register(t4);
StructuredBuffer<float> nnBiases : register(t5);

struct NNOutputs
{
    float outputs[8];
};

#if !LIGHT_WEIGHT_NN
//layer 0 = 14, layer1 = 64, layer2 = 64, layer4 = 8
NNOutputs forward(float input[14])
{
    float layer1[64];
    float layer2[64];
    float output[8];
    

    // Layer 1 계산
    for (int i0 = 0; i0 < 64; i0++)
    {
        layer1[i0] = nnBiases[i0];
        
        float4 l1 = float4(input[0], input[1], input[ 2], input[3]);
        float4 w1 = float4(nnWeights[i0 * 14], nnWeights[i0 * 14 + 1], nnWeights[i0 * 14 + 2], nnWeights[i0 * 14 + 3]);

        float4 l2 = float4(input[4], input[5], input[6], input[7]);
        float4 w2 = float4(nnWeights[i0 * 14 + 4], nnWeights[i0 * 14 + 5], nnWeights[i0 * 14 + 6], nnWeights[i0 * 14 + 7]);

        float4 l3 = float4(input[8], input[9], input[10], input[11]);
        float4 w3 = float4(nnWeights[i0 * 14 + 8], nnWeights[i0 * 14 + 9], nnWeights[i0 * 14 + 10], nnWeights[i0 * 14 + 11]);

        float4 l4 = float4(input[12], input[13], 0.0f, 0.0f);
        float4 w4 = float4(nnWeights[i0 * 14 + 12], nnWeights[i0 * 14 + 13], 0.0f, 0.0f);

        layer1[i0] += dot(w1, l1) + dot(w2, l2) + dot(w3, l3) + dot(w4, l4);
        layer1[i0] = max(0.0f, layer1[i0]); // ReLU 적용
    }  

    // Layer 2 계산
    for (int i1 = 0; i1 < 64; i1++)
    {
        layer2[i1] = nnBiases[64 + i1];
        for (int j = 0; j < 64; j+=4)
        {
            float weightOffset = 64 * 14 + i1 * 64 + j;
            float4 l1 = float4(layer1[j], layer1[j + 1], layer1[j + 2], layer1[j + 3]);
            float4 w = float4(nnWeights[weightOffset], nnWeights[weightOffset + 1], nnWeights[weightOffset + 2], nnWeights[weightOffset + 3]);
            
            layer2[i1] += dot(w, l1);
        }
        layer2[i1] = max(0.0f, layer2[i1]); // ReLU 적용
    }

    // Output Layer 계산 (Sigmoid)
    for (int i2 = 0; i2 < 8; i2++)
    {
        output[i2] = nnBiases[64 + 64 + i2]; // 초기 편향값 추가
        for (int j = 0; j < 64; j+=4)
        {
            float weightOffset = 64 * 14 + 64 * 64 + i2 * 64 + j;
            float4 l1 = float4(layer2[j], layer2[j + 1], layer2[j + 2], layer2[j + 3]);
            float4 w1 = float4(nnWeights[weightOffset], nnWeights[weightOffset + 1], nnWeights[weightOffset + 2], nnWeights[weightOffset + 3]);

            output[i2] += dot(w1, l1);
        }

        // 안정적인 Sigmoid 적용
        output[i2] = 1.0f / (1.0f + exp(-output[i2]));
        //output[i2] = fast_sigmoid(output[i2]);
    }
    
    NNOutputs nnOutputs;
    for (int i = 0; i < 8; i++)
    {
        nnOutputs.outputs[i] = output[i];
    }

    return nnOutputs;
}
#endif

#if LIGHT_WEIGHT_NN
//layer 0 = 14, layer1 = 16, layer2 = 8
NNOutputs forward(float input[14])
{
    float layer1[16];
    float output[8];
    
    // Layer 1 계산
    for (int i0 = 0; i0 < 16; i0++)
    {
        layer1[i0] = nnBiases[i0];
        
        float weightOffset = i0 * 14;
        float4 l1 = float4(input[0], input[1], input[ 2], input[3]);
        float4 w1 = float4(nnWeights[weightOffset], nnWeights[weightOffset + 1], nnWeights[weightOffset + 2], nnWeights[weightOffset + 3]);
        float4 l2 = float4(input[4], input[5], input[6], input[7]);
        float4 w2 = float4(nnWeights[weightOffset + 4], nnWeights[weightOffset + 5], nnWeights[weightOffset + 6], nnWeights[weightOffset + 7]);
        float4 l3 = float4(input[8], input[9], input[10], input[11]);
        float4 w3 = float4(nnWeights[weightOffset + 8], nnWeights[weightOffset + 9], nnWeights[weightOffset + 10], nnWeights[weightOffset + 11]);
        float4 l4 = float4(input[12], input[13], 0.0f, 0.0f);
        float4 w4 = float4(nnWeights[weightOffset + 12], nnWeights[weightOffset + 13], 0.0f, 0.0f);
        layer1[i0] += dot(w1, l1) + dot(w2, l2) + dot(w3, l3) + dot(w4, l4);
        layer1[i0] = max(0.0f, layer1[i0]); // ReLU 적용
    }  
    // Output Layer 계산 (Sigmoid)
    for (int i2 = 0; i2 < 8; i2++)
    {
        output[i2] = nnBiases[16 + i2]; // 초기 편향값 추가
        for (int j = 0; j < 16; j+=4)
        {
            float weightOffset = 16 * 14 + i2 * 16 + j;
            float4 l1 = float4(layer1[j], layer1[j + 1], layer1[j + 2], layer1[j + 3]);
            float4 w1 = float4(nnWeights[weightOffset], nnWeights[weightOffset + 1], nnWeights[weightOffset + 2], nnWeights[weightOffset + 3]);
            output[i2] += dot(w1, l1);
        }
        // 안정적인 Sigmoid 적용
        output[i2] = 1.0f / (1.0f + exp(-output[i2]));

       //output[i2] = fast_sigmoid(output[i2]);
    }

    NNOutputs nnOutputs;
    for (int i = 0; i < 8; i++)
    {
        nnOutputs.outputs[i] = output[i];
    }

    return nnOutputs;
}
#endif // LIGHT_WEIGHT_NN
#endif //USE_NEURAL_TEXTURES

struct MaterialInputs
{
    float3 albedo;
    float3 normal;
    float1 ao;
    float1 roughness;
};

MaterialInputs GetMaterialInputs(float2 tex)
{
    MaterialInputs inputs;
#if USE_NEURAL_TEXTURES
    float3 feature0 = FeatureGrid0.SampleLevel(TextureSampler, tex, 0).rgb;
    float3 feature1 = FeatureGrid1.SampleLevel(TextureSampler, tex, 0).rgb; 
    float3 feature2 = FeatureGrid2.SampleLevel(TextureSampler, tex, 0).rgb;
    float3 feature3 = FeatureGrid3.SampleLevel(TextureSampler, tex, 0).rgb;
    
    inputs.albedo = feature0;
    inputs.normal = feature1;
    inputs.ao = feature2.x;
    inputs.roughness = feature3.x;
    
    float nnInput[14] = { feature0.r, feature0.g, feature0.b, feature1.r, feature1.g, feature1.b, feature2.r, feature2.g, feature2.b, feature3.r, feature3.g, feature3.b, tex.x, tex.y };
    
    //run the neural network
    float nnOutput[8] = forward(nnInput).outputs;
    inputs.albedo = float3(nnOutput[0], nnOutput[1], nnOutput[2]).bgr;
    inputs.normal = float3(nnOutput[3], nnOutput[4], nnOutput[5]).bgr;
    inputs.ao = nnOutput[6];
    inputs.roughness = nnOutput[7];

#else
    inputs.albedo = AlbeoTexture.SampleLevel(TextureSampler, tex, 0).rgb;
    inputs.normal = NormalTexture.SampleLevel(NormalSampler, tex, 0).rgb;
    inputs.ao = AOTexture.SampleLevel(TextureSampler, tex, 0).x;
    inputs.roughness = RoughnessTexture.SampleLevel(TextureSampler, tex, 0).x;
#endif
    return inputs;
}


#define PI 3.14159265359

float3 fresnelSchlick(float3 f0, float cosTheta)
{
    return f0 + (1.0f - f0) * pow(1.0f - cosTheta, 5.0f);
}

float distributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;
    
    return num / denom;
}

float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;
    
    float num = NdotV;
    float denom = NdotV * (1.0f - k) + k;
    
    return num / denom;
}

float geometrySmith(float NdotV, float NdotL, float roughness)
{
    float ggx1 = geometrySchlickGGX(NdotV, roughness);
    float ggx2 = geometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

//Pixel Shader
float4 PSMain(PixelInput Input) : SV_TARGET
{
    //adjust the texture coordinates against the screen size
    float screenratio = 16.0f / 9.0f;
    float texratio = 1.0f;
    
    //scale the texture coordinates pivted on the center
    float2 tex = (Input.tex - 0.5f) * scale + 0.5f;
      
    //adjust y coordinate by width/height ratio
    tex.y = tex.y / (screenratio / texratio);
    
    MaterialInputs material = GetMaterialInputs(tex);
        
    float3 albedo = material.albedo;
    float3 normal = material.normal;
    float1 ao = material.ao;
    float1 roughness = material.roughness;
    
    float3 defaultNormal = float3(0.0f, 0.0f, 1.0f);
    
    //Normal mapping
    normal = normalize(normal * 2.0f - 1.0f);
        
    //Light direction
    float3 lightDir = normalize(float3(lightpos, 0.0f, 1.0f));
    float3 lightColor = float3(1.0f, 1.0f, 1.0f) * intensity;
   
    
    //pbr calculation
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo.rgb, metallic);
    float3 N = normal;
    float3 V = normalize(float3(0.0f, 0.0f, 1.0f));
    float3 L = lightDir;
    float3 H = normalize(V + L);
    
    float NdotL = max(dot(N, L), 0.0f);
    float NdotV = max(dot(N, V), 0.0f);
    float NdotH = max(dot(N, H), 0.0f);
    float LdotH = max(dot(L, H), 0.0f);
    
    //Fresnel
    float3 F = fresnelSchlick(F0, LdotH);
    
    //Distribution
    float D = distributionGGX(NdotH, roughness);
    
    //Geometry
    float G = geometrySmith(NdotV, NdotL, roughness);
    
    //Lamberian Diffuse
    float3 diffuse = (1.0f - F) * (albedo.rgb / PI);
    
    //Cook-Torrance BRDF
    float3 specular = (F * D * G) / (4.0f * max(0.001f, NdotV) * max(0.001f, NdotL));
    
    //Light Intensity
    float3 Lintensity = lightColor * NdotL;
    
    //Combine all including ao
    float4 finalColor = float4((diffuse + specular) * Lintensity * ao, 1.0f);
    
    //return albedo;
    return finalColor;
}
