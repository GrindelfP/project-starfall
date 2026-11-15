cbuffer ObjectConstants : register(b0) {
    row_major float4x4 worldViewProjection;
    float4 objectColor;
};
struct VertexInput { float3 position : POSITION; };
struct VertexOutput { float4 position : SV_POSITION; float4 color : COLOR; float3 localPosition : TEXCOORD0; };
VertexOutput VSMain(VertexInput input) {
    VertexOutput output;
    output.position = mul(float4(input.position, 1.0), worldViewProjection);
    output.color = objectColor;
    output.localPosition = input.position;
    return output;
}
float4 PSMain(VertexOutput input) : SV_TARGET {
    float3 face = abs(normalize(cross(ddx(input.localPosition), ddy(input.localPosition))));
    float shade = dot(face, float3(0.65, 1.0, 0.8));
    return float4(input.color.rgb * shade, input.color.a);
}
