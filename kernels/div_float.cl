kernel void div_float( global float* A, global float* B, global float* div) {
   const int idx = get_global_id(0);
   
   B[idx] = A[idx] / *div;
}