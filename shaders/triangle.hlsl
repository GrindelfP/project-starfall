// Clip-space position and linear vertex color for the initial DX11 triangle.
struct VertexInput { float2 position : POSITION; float3 color : COLOR; };
struct VertexOutput { float4 position : SV_POSITION; float3 color : COLOR; };
VertexOutput VSMain(VertexInput input) {
    VertexOutput output;
    output.position = float4(input.position, 0.0, 1.0);
    output.color = input.color;
    return output;
}
float4 PSMain(VertexOutput input) : SV_TARGET { return float4(input.color, 1.0); }
