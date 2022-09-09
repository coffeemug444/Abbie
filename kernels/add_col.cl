kernel void add_col( global float* A, global float* B, global int* W, global float* out) {
   const int idx = get_global_id(0);

   int row = idx / (*W);
   out[idx] = A[idx] + B[row];
}