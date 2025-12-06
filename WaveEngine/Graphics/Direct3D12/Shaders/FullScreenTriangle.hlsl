struct VSOutput {
	noperspective float4 Position : SV_POSITION;
	noperspective float2 UV : TEXCOORD;
};

VSOutput FullScreenTriangleVS(in uint VertexId : SV_VERTEXID) { 
	VSOutput output;

	// TODO: write fullscreen triangle shader code
	output.Position = float4(0, 0, 0, 1);
	return output;
}