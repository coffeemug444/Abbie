kernel void mul_float( global float* A, global float* B, global float* mul) {
   const int idx = get_global_id(0);
   
   B[idx] = A[idx] * *mul;
}