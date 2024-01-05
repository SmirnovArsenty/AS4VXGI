struct CompleteData
{
    int data[4];
};

ConsumeStructuredBuffer<CompleteData> complete : register(u0);

[numthreads(1,1,1)]
void CSMain( uint3 id : SV_DispatchThreadID ) {
    complete.Consume();
}