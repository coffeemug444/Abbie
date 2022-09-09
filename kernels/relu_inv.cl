kernel void relu_inv(global float* A, global float* B) {
    const int idx = get_global_id(0);
    float scale;
    if (A[idx] < 0.0f ) {
        B[idx] = 0.1f;
    } else {
        B[idx] = 1.0f;
    }
} 