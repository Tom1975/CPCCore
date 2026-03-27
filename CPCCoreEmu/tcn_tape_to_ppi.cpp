#include "tcn_tape_to_ppi.h"
#include "tcn_weights.h"

#include <cstring>   // memset
#include <algorithm> // std::fill
#include <vector>

// ============================================================
//  conv1d
//
//  Applique une convolution 1D avec zero-padding.
//
//  in  : [in_ch × N]  (in_ch lignes de N floats, layout [ch][pos])
//  out : [out_ch × N]
//  weight : [out_ch × in_ch × kernel_size]  (layout PyTorch)
//  bias   : [out_ch]
// ============================================================
void TcnTapeToPPI::conv1d(
    const float* in,  int in_ch,
    float*       out, int out_ch,
    const float* weight, const float* bias,
    int N, int kernel_size, int padding)
{
    const int half = padding; // padding == (kernel_size - 1) / 2

    for (int oc = 0; oc < out_ch; ++oc) {
        float* o = out + oc * N;
        // Initialiser avec le biais
        for (int p = 0; p < N; ++p)
            o[p] = bias[oc];

        for (int ic = 0; ic < in_ch; ++ic) {
            const float* inp = in + ic * N;
            // Poids pour ce (oc, ic) : weight[(oc * in_ch + ic) * kernel_size]
            const float* w = weight + (oc * in_ch + ic) * kernel_size;

            for (int p = 0; p < N; ++p) {
                float acc = 0.0f;
                for (int k = 0; k < kernel_size; ++k) {
                    const int ip = p + k - half;
                    if (ip >= 0 && ip < N)
                        acc += w[k] * inp[ip];
                }
                o[p] += acc;
            }
        }
    }
}

void TcnTapeToPPI::relu(float* data, int size)
{
    for (int i = 0; i < size; ++i)
        if (data[i] < 0.0f) data[i] = 0.0f;
}

// ============================================================
//  Filtrer
//
//  Pipeline d'inférence :
//    input  [1 × N]   float
//    → Conv0 + ReLU → [16 × N]   (buf_a)
//    → Conv2 + ReLU → [32 × N]   (buf_b)
//    → Conv4 + ReLU → [16 × N]   (buf_a, réutilisé)
//    → Linear par position → N logits
//    → signe du logit → -1.0 / +1.0 (in-place dans samples)
// ============================================================
void TcnTapeToPPI::Filtrer(std::vector<double>& samples) const
{
    const int N = static_cast<int>(samples.size());
    if (N == 0) return;

    // --- Conversion de l'entrée en float [1 × N] ---
    std::vector<float> buf_a(N);
    for (int i = 0; i < N; ++i)
        buf_a[i] = static_cast<float>(samples[i]);

    // --- Couche 1 : Conv1d(1→16, k=9, pad=4) + ReLU ---
    // buf_a : [1 × N] → buf_16 : [16 × N]
    std::vector<float> buf_16(16 * N);
    conv1d(buf_a.data(),  1,
           buf_16.data(), 16,
           TcnWeights::conv0_weight, TcnWeights::conv0_bias,
           N, 9, 4);
    relu(buf_16.data(), 16 * N);

    // --- Couche 2 : Conv1d(16→32, k=9, pad=4) + ReLU ---
    // buf_16 : [16 × N] → buf_32 : [32 × N]
    std::vector<float> buf_32(32 * N);
    conv1d(buf_16.data(), 16,
           buf_32.data(), 32,
           TcnWeights::conv2_weight, TcnWeights::conv2_bias,
           N, 9, 4);
    relu(buf_32.data(), 32 * N);
    buf_16.clear();
    buf_16.shrink_to_fit(); // libérer [16 × N] avant d'allouer le suivant

    // --- Couche 3 : Conv1d(32→16, k=9, pad=4) + ReLU ---
    // buf_32 : [32 × N] → buf_out : [16 × N]
    std::vector<float> buf_out(16 * N);
    conv1d(buf_32.data(), 32,
           buf_out.data(), 16,
           TcnWeights::conv4_weight, TcnWeights::conv4_bias,
           N, 9, 4);
    relu(buf_out.data(), 16 * N);
    buf_32.clear();
    buf_32.shrink_to_fit(); // libérer [32 × N]

    // --- Couche linéaire + décision ---
    // Pour chaque position p :
    //   logit = sum_{f=0}^{15} head_weight[f] * buf_out[f * N + p] + head_bias[0]
    //   out[p] = (logit > 0) ? +1.0 : -1.0
    const float  head_bias   = TcnWeights::head_bias[0];
    const float* head_weight = TcnWeights::head_weight; // [1 × 16], layout [f]

    for (int p = 0; p < N; ++p) {
        float logit = head_bias;
        for (int f = 0; f < 16; ++f)
            logit += head_weight[f] * buf_out[f * N + p];
        samples[p] = (logit > 0.0f) ? 1.0 : -1.0;
    }
}
