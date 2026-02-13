//게임 객체의 정보를 위한 상수 버퍼를 선언한다.
cbuffer cbGameObjectInfo : register(b0) {
    matrix gmtxWorld : packoffset(c0); // 월드 행렬
};

//카메라의 정보를 위한 상수 버퍼를 선언한다.
cbuffer cbCameraInfo : register(b1) {
    matrix gmtxView : packoffset(c0);           // 뷰 행렬
    matrix gmtxProjection : packoffset(c4);     // 프로젝션 행렬
    float3 gvCameraPosition : packoffset(c8);   // 카메라 위치
};

//조명 정보를 위한 상수 버퍼
cbuffer cbLightInfo : register(b2) {
    float3 gvLightPosition;     // 라이트 위치
    float gfLightRange;         // 라이트 범위
    float3 gvLightColor;        // 라이트 색상
    float gfLightIntensity;     // 라이트 강도
};

//=============================================================================
// 텍스처 리소스와 샘플러
//=============================================================================
Texture2D g_txAlbedo : register(t0);    // alb - Albedo/Diffuse
Texture2D g_txAO : register(t1);        // ao - Ambient Occlusion
Texture2D g_txMetallic : register(t2);  // mtl - Metallic
Texture2D g_txNormal : register(t3);    // nrm - Normal Map
Texture2D g_txRGN : register(t4);       // rgn - Region Map
Texture2D g_txWLM_R : register(t5);     // wlm_r - Wrinkle Map Red
Texture2D g_txWLM_G : register(t6);     // wlm_g - Wrinkle Map Green
Texture2D g_txWLM_B : register(t7);     // wlm_b - Wrinkle Map Blue

SamplerState g_sampler : register(s0);

//=============================================================================
// 포인트 라이트 계산 함수
//=============================================================================
float3 CalculatePointLight(float3 worldPos, float3 normal, float3 viewDir, float3 albedo, float metallic) {
    float3 lightPos = float3(0.0f, 200.0f, 0.0f); // 라이트 위치
    float3 lightColor = float3(1.0f, 1.0f, 1.0f); // 라이트 색상: 흰색
    float lightRange = 500.0f;                    // 라이트 범위
    float lightIntensity = 1.5f;                  // 라이트 강도
    
    // 라이트 방향 및 거리
    float3 lightVec = lightPos - worldPos;
    float distance = length(lightVec);
    float3 lightDir = normalize(lightVec);
    
    // 감쇠 (Attenuation)
    float attenuation = saturate(1.0f - (distance / lightRange));
    attenuation *= attenuation; // 제곱 감쇠
    
    // Diffuse
    float NdotL = max(dot(normal, lightDir), 0.0f);
    float3 diffuse = NdotL * lightColor * lightIntensity * attenuation;
    
    // Specular (Blinn-Phong)
    float3 halfVector = normalize(lightDir + viewDir);
    float NdotH = max(dot(normal, halfVector), 0.0f);
    float specularPower = 64.0f;
    float specular = pow(NdotH, specularPower) * attenuation * lightIntensity;
    
    // 메탈릭 적용
    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);
    float3 diffuseColor = albedo * (1.0f - metallic) * diffuse;
    float3 specularColor = F0 * specular * lightColor;
    
    return diffuseColor + specularColor;
}

//=============================================================================
// Diffused 정점 셰이더 (기존 - 색상만 사용)
//=============================================================================
struct VS_DIFFUSED_INPUT {
    float3 position : POSITION;
    float4 color : COLOR;
};

struct VS_DIFFUSED_OUTPUT {
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

VS_DIFFUSED_OUTPUT VSDiffused(VS_DIFFUSED_INPUT input) {
    VS_DIFFUSED_OUTPUT output;
    output.position = mul(mul(mul(float4(input.position, 1.0f), gmtxWorld), gmtxView), gmtxProjection);
    output.color = input.color;
    return (output);
}

float4 PSDiffused(VS_DIFFUSED_OUTPUT input) : SV_TARGET {
    return (input.color);
}

//=============================================================================
// Textured 정점 셰이더 (텍스처 사용)
//=============================================================================
struct VS_TEXTURED_INPUT {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
};

struct VS_TEXTURED_OUTPUT {
    float4 position : SV_POSITION;
    float3 worldPosition : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
};

VS_TEXTURED_OUTPUT VSTextured(VS_TEXTURED_INPUT input) {
    VS_TEXTURED_OUTPUT output;
    
    float4 worldPosition = mul(float4(input.position, 1.0f), gmtxWorld);
    output.worldPosition = worldPosition.xyz;
    output.position = mul(mul(worldPosition, gmtxView), gmtxProjection);
    output.normal = normalize(mul(input.normal, (float3x3) gmtxWorld));
    output.texcoord = input.texcoord;
    
    return output;
}

//=============================================================================
// Textured 픽셀 셰이더 (Albedo만 사용 - 기본)
//=============================================================================
float4 PSTextured(VS_TEXTURED_OUTPUT input) : SV_TARGET {
    float4 albedo = g_txAlbedo.Sample(g_sampler, input.texcoord);
    
    // 간단한 조명 계산
    float3 lightDir = normalize(float3(1.0f, 1.0f, -1.0f));
    float NdotL = max(dot(input.normal, lightDir), 0.0f);
    float3 ambient = 0.3f;
    float3 lighting = ambient + NdotL * 0.7f;
    
    float4 finalColor = albedo;
    finalColor.rgb *= lighting;
    
    return finalColor;
}

//=============================================================================
// Textured 픽셀 셰이더 (AO 적용)
//=============================================================================
float4 PSTexturedAO(VS_TEXTURED_OUTPUT input) : SV_TARGET {
    float4 albedo = g_txAlbedo.Sample(g_sampler, input.texcoord);
    float ao = g_txAO.Sample(g_sampler, input.texcoord).r;
    
    // 간단한 조명 계산
    float3 lightDir = normalize(float3(1.0f, 1.0f, -1.0f));
    float NdotL = max(dot(input.normal, lightDir), 0.0f);
    float3 ambient = 0.3f;
    float3 lighting = ambient + NdotL * 0.7f;
    
    float4 finalColor = albedo;
    finalColor.rgb *= lighting * ao; // AO 적용
    
    return finalColor;
}

//=============================================================================
// Textured 픽셀 셰이더 (전체 텍스처 사용 - PBR 스타일)
//=============================================================================
float4 PSTexturedFull(VS_TEXTURED_OUTPUT input) : SV_TARGET {
    // 텍스처 샘플링
    float4 albedo = g_txAlbedo.Sample(g_sampler, input.texcoord);
    float ao = g_txAO.Sample(g_sampler, input.texcoord).r;
    float metallic = g_txMetallic.Sample(g_sampler, input.texcoord).r;
    float3 normalMap = g_txNormal.Sample(g_sampler, input.texcoord).rgb;
    
    // 노멀 맵 변환 (0~1 -> -1~1)
    normalMap = normalMap * 2.0f - 1.0f;
    float3 normal = normalize(input.normal + normalMap * 0.5f);
    
    // 카메라/뷰 방향 계산
    float3 viewDir = normalize(gvCameraPosition - input.worldPosition);
    
    // 앰비언트 라이팅
    float3 ambient = albedo.rgb * 0.2f * ao;
    
    // 포인트 라이트 (y=200 위치)
    float3 pointLight = CalculatePointLight(input.worldPosition, normal, viewDir, albedo.rgb, metallic);
    
    // 최종 색상
    float3 finalColor = ambient + pointLight * ao;
    
    return float4(finalColor, albedo.a);
}