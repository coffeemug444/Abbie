kernel void transpose( global float* A, global float* B, global int* W, global int* H) {
   const int idx = get_global_id(0);
   
   int row = idx / *W;
   int col = idx % *W;
   B[col**H + row] = A[idx];
}