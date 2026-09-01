#pragma once

// ============================================================
//  TcnTapeToPPI — inférence TCN cassette → PPI
//
//  Modèle : TinyCpaTCN entraîné sur paires cassette/PPI CPC
//    - 3 × Conv1d (k=9, padding=4) + ReLU
//    - 1 × Linear (16 → 1)
//    - Fenêtre de contexte : 128 samples (~3 ms @ 44100 Hz)
//    - Poids embarqués dans tcn_weights.h (9 441 floats, ~37 KB)
//
//  Interface :
//    TcnTapeToPPI conv;
//    conv.Filtrer(samples);  // in-place, [-1.0, +1.0] → [-1.0, +1.0]
//
//  Entrée : signal cassette normalisé dans [-1.0, +1.0]
//  Sortie : signal PPI bipolaire, -1.0 ou +1.0 (in-place)
//
//  Aucune dépendance externe — C++11, <vector> uniquement.
//  Mémoire temporaire : ~192 × N bytes (N = nb samples de l'appel).
//  Pour limiter l'empreinte mémoire, appeler Filtrer bloc par bloc.
// ============================================================

#include <vector>

class TcnTapeToPPI {
public:
    TcnTapeToPPI() = default;

    // Convertit le signal cassette en signal PPI bipolaire (-1.0 / +1.0)
    // in-place. Les samples doivent être normalisés dans [-1.0, +1.0].
    void Filtrer(std::vector<double>& samples) const;

private:
    // Conv1d : in[in_ch × N] → out[out_ch × N]
    // Layout mémoire : out_ch × N (out_ch lignes de N floats)
    static void conv1d(
        const float* in,  int in_ch,
        float*       out, int out_ch,
        const float* weight, const float* bias,
        int N, int kernel_size, int padding);

    static void relu(float* data, int size);
};
