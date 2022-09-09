kernel void matmul( global float* A, global float* B, global float* C, global int* common, global int* B_w) {
    const int idx = get_global_id(0);
    

    float tmp = 0;
    int row = idx / *B_w;
    int col = idx % *B_w;

    for (int w = 0; w < *common; w++) {
        tmp += A[row**common + w]*B[*B_w*w + col];
    }
    C[idx] = tmp;
}
