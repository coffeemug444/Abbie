kernel void sub_mat( global float* A, global float* B, global float* out) {
   const int idx = get_global_id(0);
   
   out[idx] = A[idx] - B[idx];
}