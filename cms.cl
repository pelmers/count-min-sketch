__kernel void cms(
   __constant unsigned int* params,
   __global unsigned int* hashes,
   __global unsigned int* counts)
{
   int i = get_global_id(0);
// Do 8 hashes, increment corresponding positions in counts
// See atomic_add(pointr, incr_value)
// 2^22 - 1
unsigned m = 4194303;
int k;
for (k = 0; k < 8; k++) {
       unsigned int a = params[k*2];
       unsigned int b = params[k*2+1];
       unsigned int idx = (a*hashes[i] + b) % m;

       atomic_add(counts + idx + (k << 22), 1);
       }

}
