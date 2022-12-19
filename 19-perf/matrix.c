double *create_matrix(unsigned dim) {
    double *ret = calloc(dim * dim, sizeof(double));
    if (!ret) die("calloc");
    return ret;
}
double *create_random_matrix(unsigned dim) {
    double * ret = create_matrix(dim);

    for (unsigned i = 0; i < dim * dim; i ++) {
        ret[i] = random() / (double) INT_MAX;
    }

    return ret;
}

void matrixmul_drepper(unsigned N, double *A, double *B, double *C) {
    unsigned CLS = 64;
    unsigned SM  = CLS / sizeof(double);

    int i, i2, j, j2, k, k2;
    double *restrict rres;
    double *restrict rmul1;
    double *restrict rmul2;

    for (i = 0; i < N; i += SM)
        for (j = 0; j < N; j += SM)
            for (k = 0; k < N; k += SM)
                for (i2 = 0, rres = &C[i * N + j], rmul1 = &A[i * N + k]; i2 < SM;
                     ++i2, rres += N, rmul1 += N)
                {
                    _mm_prefetch (&rmul1[8], _MM_HINT_NTA);
                    for (k2 = 0, rmul2 = &B[k * N + j]; k2 < SM; ++k2, rmul2 += N)
                    {
                        __m128d m1d = _mm_load_sd (&rmul1[k2]);
                        m1d = _mm_unpacklo_pd (m1d, m1d);
                        for (j2 = 0; j2 < SM; j2 += 2)
                        {
                            __m128d m2 = _mm_load_pd (&rmul2[j2]);
                            __m128d r2 = _mm_load_pd (&rres[j2]);
                            _mm_store_pd (&rres[j2],
                                          _mm_add_pd (_mm_mul_pd (m2, m1d), r2));
                        }
                    }
                }
}

void matrixmul_naive(unsigned N, double *A, double *B, double *C) {
    unsigned i, j, k;
    for (i = 0; i < N; ++i)
        for (j = 0; j < N; ++j) {
            double sum = 0;
            for (k = 0; k < N; ++k)
                sum += A[i *N + k] * B[k * N + j];
            C[i * N + j] = sum;
        }
}
