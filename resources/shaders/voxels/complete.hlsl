AppendStructuredBuffer<int4> complete : register(u0);
[numthreads(1,1,1)]
void CSMain( uint3 id : SV_DispatchThreadID ) {
    complete.Append((int4)0);
}
