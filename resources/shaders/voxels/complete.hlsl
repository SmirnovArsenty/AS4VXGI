struct CompleteData
{
    int data[4];
};

AppendStructuredBuffer<CompleteData> complete : register(u0);

[numthreads(1,1,1)]
void CSMain( uint3 id : SV_DispatchThreadID ) {
    CompleteData data = (CompleteData)0;
    complete.Append(data);
}
