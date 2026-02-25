/*
 * Cassette FIR Analyzer
 * Analyse et calcule le filtre FIR optimal entre signaux source et destination
 * Portable C++17 - Compilation: g++ -std=c++17 -O3 cassette_fir_analyzer.cpp -o cassette_fir_analyzer
 */

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <memory>
#include <filesystem>
#include <cstring>
#include <iomanip>

namespace fs = std::filesystem;

// ============================================================================
// Structure WAV (lecture simplifiée)
// ============================================================================

struct WavHeader {
   char riff[4];           // "RIFF"
   uint32_t fileSize;
   char wave[4];           // "WAVE"
   char fmt[4];            // "fmt "
   uint32_t fmtSize;
   uint16_t audioFormat;
   uint16_t numChannels;
   uint32_t sampleRate;
   uint32_t byteRate;
   uint16_t blockAlign;
   uint16_t bitsPerSample;
   char data[4];           // "data"
   uint32_t dataSize;
};

class WavFile {
public:
   std::vector<float> samples;
   uint32_t sampleRate;
   uint16_t numChannels;

   bool load(const std::string& filename) {
      std::ifstream file(filename, std::ios::binary);
      if (!file.is_open()) {
         std::cerr << "Erreur: impossible d'ouvrir " << filename << std::endl;
         return false;
      }

      WavHeader header;
      file.read(reinterpret_cast<char*>(&header), sizeof(WavHeader));

      // Vérifications basiques
      if (std::strncmp(header.riff, "RIFF", 4) != 0 ||
         std::strncmp(header.wave, "WAVE", 4) != 0) {
         std::cerr << "Erreur: " << filename << " n'est pas un fichier WAV valide" << std::endl;
         return false;
      }

      sampleRate = header.sampleRate;
      numChannels = header.numChannels;

      // Lecture des échantillons
      size_t numSamples = header.dataSize / (header.bitsPerSample / 8) / numChannels;
      samples.resize(numSamples);

      if (header.bitsPerSample == 16) {
         std::vector<int16_t> rawSamples(numSamples * numChannels);
         file.read(reinterpret_cast<char*>(rawSamples.data()), header.dataSize);

         // Conversion en float mono (moyenne des canaux)
         for (size_t i = 0; i < numSamples; ++i) {
            float sum = 0.0f;
            for (uint16_t ch = 0; ch < numChannels; ++ch) {
               sum += rawSamples[i * numChannels + ch] / 32768.0f;
            }
            samples[i] = sum / numChannels;
         }
      }
      else if (header.bitsPerSample == 8) {
         std::vector<uint8_t> rawSamples(numSamples * numChannels);
         file.read(reinterpret_cast<char*>(rawSamples.data()), header.dataSize);

         for (size_t i = 0; i < numSamples; ++i) {
            float sum = 0.0f;
            for (uint16_t ch = 0; ch < numChannels; ++ch) {
               sum += (rawSamples[i * numChannels + ch] - 128) / 128.0f;
            }
            samples[i] = sum / numChannels;
         }
      }
      else {
         std::cerr << "Erreur: format " << header.bitsPerSample << " bits non supporté" << std::endl;
         return false;
      }

      file.close();
      return true;
   }

   void normalize() {
      if (samples.empty()) return;

      float maxAbs = 0.0f;
      for (float s : samples) {
         maxAbs = std::max(maxAbs, std::abs(s));
      }

      if (maxAbs > 1e-6f) {
         for (float& s : samples) {
            s /= maxAbs;
         }
      }
   }
};

// ============================================================================
// Alignement temporel par corrélation croisée
// ============================================================================

class SignalAligner {
public:
   // Trouve le décalage optimal entre deux signaux
   static int findBestOffset(const std::vector<float>& signal1,
      const std::vector<float>& signal2,
      int maxOffset = 10000) {

      size_t minLen = std::min(signal1.size(), signal2.size());
      if (minLen < maxOffset) maxOffset = minLen / 2;

      float bestCorr = -1e9f;
      int bestOffset = 0;

      // Recherche du meilleur alignement
      for (int offset = -maxOffset; offset <= maxOffset; ++offset) {
         float corr = computeCorrelation(signal1, signal2, offset, minLen);
         if (corr > bestCorr) {
            bestCorr = corr;
            bestOffset = offset;
         }
      }

      std::cout << "  Meilleure corrélation: " << bestCorr
         << " à offset=" << bestOffset << " échantillons" << std::endl;

      return bestOffset;
   }

   // Aligne signal2 sur signal1
   static void align(const std::vector<float>& signal1,
      std::vector<float>& signal2,
      int offset) {

      if (offset == 0) return;

      std::vector<float> aligned;
      size_t commonLen = std::min(signal1.size(), signal2.size() - std::abs(offset));
      aligned.resize(commonLen);

      if (offset > 0) {
         // signal2 est en avance
         std::copy(signal2.begin() + offset, signal2.begin() + offset + commonLen,
            aligned.begin());
      }
      else {
         // signal2 est en retard
         std::copy(signal2.begin(), signal2.begin() + commonLen, aligned.begin());
      }

      signal2 = aligned;
   }

private:
   static float computeCorrelation(const std::vector<float>& s1,
      const std::vector<float>& s2,
      int offset,
      size_t len) {
      float corr = 0.0f;
      size_t count = 0;

      size_t testLen = std::min(len, size_t(5000)); // Limite pour la vitesse

      for (size_t i = 0; i < testLen; ++i) {
         int idx1 = i;
         int idx2 = i + offset;

         if (idx1 >= 0 && idx1 < s1.size() && idx2 >= 0 && idx2 < s2.size()) {
            corr += s1[idx1] * s2[idx2];
            count++;
         }
      }

      return count > 0 ? corr / count : -1e9f;
   }
};

// ============================================================================
// Calcul du filtre FIR par méthode des moindres carrés
// ============================================================================

class FIRCalculator {
public:
   struct FIRFilter {
      std::vector<float> coefficients;
      float mse;  // Erreur quadratique moyenne
      float snr;  // Signal-to-Noise Ratio (dB)
   };

   // Calcule le FIR optimal de longueur firLength
   static FIRFilter calculateFIR(const std::vector<float>& input,
      const std::vector<float>& output,
      int firLength = 128) {

      FIRFilter fir;
      fir.coefficients.resize(firLength, 0.0f);

      size_t N = std::min(input.size(), output.size()) - firLength;
      if (N < firLength) {
         std::cerr << "Erreur: signaux trop courts pour FIR de longueur " << firLength << std::endl;
         return fir;
      }

      // Construction de la matrice d'autocorrélation R et vecteur p
      // R * h = p où h est le filtre FIR recherché
      std::vector<std::vector<float>> R(firLength, std::vector<float>(firLength, 0.0f));
      std::vector<float> p(firLength, 0.0f);

      std::cout << "  Calcul de la matrice d'autocorrélation..." << std::endl;

      for (size_t n = 0; n < N; ++n) {
         for (int i = 0; i < firLength; ++i) {
            // Vecteur p = E[x[n-i] * d[n]]
            p[i] += input[n + i] * output[n + firLength - 1];

            // Matrice R = E[x[n-i] * x[n-j]]
            for (int j = 0; j < firLength; ++j) {
               R[i][j] += input[n + i] * input[n + j];
            }
         }

         if ((n + 1) % 10000 == 0) {
            std::cout << "    Progression: " << (n + 1) << "/" << N << std::endl;
         }
      }

      // Normalisation
      for (int i = 0; i < firLength; ++i) {
         p[i] /= N;
         for (int j = 0; j < firLength; ++j) {
            R[i][j] /= N;
         }
      }

      std::cout << "  Résolution du système linéaire..." << std::endl;

      // Résolution par élimination de Gauss (simple mais stable)
      fir.coefficients = solveLinearSystem(R, p);

      // Calcul des métriques d'erreur
      std::cout << "  Calcul des métriques..." << std::endl;
      fir.mse = computeMSE(input, output, fir.coefficients);
      fir.snr = computeSNR(output, input, fir.coefficients);

      return fir;
   }

   // Applique le FIR à un signal
   static std::vector<float> applyFIR(const std::vector<float>& input,
      const std::vector<float>& coeffs) {
      std::vector<float> output(input.size(), 0.0f);
      int firLength = coeffs.size();

      for (size_t n = firLength - 1; n < input.size(); ++n) {
         float sum = 0.0f;
         for (int k = 0; k < firLength; ++k) {
            sum += coeffs[k] * input[n - k];
         }
         output[n] = sum;
      }

      return output;
   }

private:
   // Résolution de R * x = p par élimination de Gauss
   static std::vector<float> solveLinearSystem(std::vector<std::vector<float>> A,
      std::vector<float> b) {
      int n = b.size();

      // Élimination (forward)
      for (int i = 0; i < n; ++i) {
         // Recherche du pivot
         int maxRow = i;
         for (int k = i + 1; k < n; ++k) {
            if (std::abs(A[k][i]) > std::abs(A[maxRow][i])) {
               maxRow = k;
            }
         }

         std::swap(A[i], A[maxRow]);
         std::swap(b[i], b[maxRow]);

         // Élimination
         for (int k = i + 1; k < n; ++k) {
            float factor = A[k][i] / (A[i][i] + 1e-10f);
            b[k] -= factor * b[i];
            for (int j = i; j < n; ++j) {
               A[k][j] -= factor * A[i][j];
            }
         }
      }

      // Substitution arrière (backward)
      std::vector<float> x(n);
      for (int i = n - 1; i >= 0; --i) {
         x[i] = b[i];
         for (int j = i + 1; j < n; ++j) {
            x[i] -= A[i][j] * x[j];
         }
         x[i] /= (A[i][i] + 1e-10f);
      }

      return x;
   }

   static float computeMSE(const std::vector<float>& input,
      const std::vector<float>& target,
      const std::vector<float>& coeffs) {
      auto output = applyFIR(input, coeffs);
      float mse = 0.0f;
      size_t count = 0;

      for (size_t i = coeffs.size(); i < std::min(output.size(), target.size()); ++i) {
         float error = output[i] - target[i];
         mse += error * error;
         count++;
      }

      return count > 0 ? mse / count : 0.0f;
   }

   static float computeSNR(const std::vector<float>& signal,
      const std::vector<float>& input,
      const std::vector<float>& coeffs) {
      auto output = applyFIR(input, coeffs);

      float signalPower = 0.0f;
      float noisePower = 0.0f;
      size_t count = 0;

      for (size_t i = coeffs.size(); i < std::min(output.size(), signal.size()); ++i) {
         signalPower += signal[i] * signal[i];
         float error = output[i] - signal[i];
         noisePower += error * error;
         count++;
      }

      if (count > 0) {
         signalPower /= count;
         noisePower /= count;
      }

      return noisePower > 1e-10f ? 10.0f * std::log10(signalPower / noisePower) : 100.0f;
   }
};

// ============================================================================
// Analyseur principal
// ============================================================================

class CassetteAnalyzer {
public:
   struct Config {
      std::string sourceDir;
      std::string targetDir;
      int firLength = 128;
      bool verbose = true;
   };

   struct AnalysisResult {
      std::string sourceName;
      std::string targetName;
      FIRCalculator::FIRFilter fir;
      bool success;
   };

   CassetteAnalyzer(const Config& cfg) : config(cfg) {}

   bool analyze() {
      // Charger les listes de fichiers
      auto sourceFiles = loadWavFiles(config.sourceDir);
      auto targetFiles = loadWavFiles(config.targetDir);

      if (sourceFiles.empty() || targetFiles.empty()) {
         std::cerr << "Erreur: aucun fichier WAV trouvé" << std::endl;
         return false;
      }

      if (sourceFiles.size() != targetFiles.size()) {
         std::cerr << "Attention: nombre différent de fichiers source ("
            << sourceFiles.size() << ") et target (" << targetFiles.size() << ")" << std::endl;
      }

      std::cout << "\n=== ANALYSE DES PAIRES DE FICHIERS ===" << std::endl;
      std::cout << "Sources: " << sourceFiles.size() << " fichiers" << std::endl;
      std::cout << "Targets: " << targetFiles.size() << " fichiers" << std::endl;
      std::cout << "Longueur FIR: " << config.firLength << " coefficients\n" << std::endl;

      // Analyse paire par paire
      size_t numPairs = std::min(sourceFiles.size(), targetFiles.size());
      std::vector<AnalysisResult> results;

      for (size_t i = 0; i < numPairs; ++i) {
         std::cout << "\n--- Paire " << (i + 1) << "/" << numPairs << " ---" << std::endl;
         AnalysisResult result = analyzePair(sourceFiles[i], targetFiles[i]);
         results.push_back(result);
      }

      // Analyse de cohérence
      std::cout << "\n\n=== ANALYSE DE COHÉRENCE ===" << std::endl;
      analyzeConsistency(results);

      // Sauvegarde du FIR moyen
      saveMeanFIR(results);

      return true;
   }

private:
   Config config;

   std::vector<std::string> loadWavFiles(const std::string& directory) {
      std::vector<std::string> files;

      if (!fs::exists(directory)) {
         std::cerr << "Erreur: répertoire " << directory << " introuvable" << std::endl;
         return files;
      }

      for (const auto& entry : fs::directory_iterator(directory)) {
         if (entry.path().extension() == ".wav" || entry.path().extension() == ".WAV") {
            files.push_back(entry.path().string());
         }
      }

      std::sort(files.begin(), files.end());
      return files;
   }

   AnalysisResult analyzePair(const std::string& sourcePath, const std::string& targetPath) {
      AnalysisResult result;
      result.sourceName = fs::path(sourcePath).filename().string();
      result.targetName = fs::path(targetPath).filename().string();
      result.success = false;

      std::cout << "Source: " << result.sourceName << std::endl;
      std::cout << "Target: " << result.targetName << std::endl;

      // Charger les fichiers
      WavFile source, target;
      if (!source.load(sourcePath) || !target.load(targetPath)) {
         return result;
      }

      std::cout << "Chargé: " << source.samples.size() << " échantillons @ "
         << source.sampleRate << " Hz" << std::endl;

      // Normalisation
      std::cout << "Normalisation..." << std::endl;
      source.normalize();
      target.normalize();

      // Alignement temporel
      std::cout << "Alignement temporel..." << std::endl;
      int offset = SignalAligner::findBestOffset(source.samples, target.samples);
      SignalAligner::align(source.samples, target.samples, offset);

      // Tronquer à la longueur commune
      size_t commonLen = std::min(source.samples.size(), target.samples.size());
      source.samples.resize(commonLen);
      target.samples.resize(commonLen);

      std::cout << "Longueur alignée: " << commonLen << " échantillons" << std::endl;

      // Calcul du FIR
      std::cout << "Calcul du FIR optimal..." << std::endl;
      result.fir = FIRCalculator::calculateFIR(source.samples, target.samples, config.firLength);

      std::cout << "\nRésultats:" << std::endl;
      std::cout << "  MSE: " << result.fir.mse << std::endl;
      std::cout << "  SNR: " << result.fir.snr << " dB" << std::endl;

      result.success = true;
      return result;
   }

   void analyzeConsistency(const std::vector<AnalysisResult>& results) {
      std::vector<FIRCalculator::FIRFilter> validFIRs;

      for (const auto& r : results) {
         if (r.success) {
            validFIRs.push_back(r.fir);
         }
      }

      if (validFIRs.empty()) {
         std::cout << "Aucun FIR valide calculé" << std::endl;
         return;
      }

      int firLen = validFIRs[0].coefficients.size();

      // Calcul des statistiques coefficient par coefficient
      std::vector<float> meanCoeffs(firLen, 0.0f);
      std::vector<float> stdCoeffs(firLen, 0.0f);

      // Moyenne
      for (const auto& fir : validFIRs) {
         for (int i = 0; i < firLen; ++i) {
            meanCoeffs[i] += fir.coefficients[i];
         }
      }
      for (int i = 0; i < firLen; ++i) {
         meanCoeffs[i] /= validFIRs.size();
      }

      // Écart-type
      for (const auto& fir : validFIRs) {
         for (int i = 0; i < firLen; ++i) {
            float diff = fir.coefficients[i] - meanCoeffs[i];
            stdCoeffs[i] += diff * diff;
         }
      }
      for (int i = 0; i < firLen; ++i) {
         stdCoeffs[i] = std::sqrt(stdCoeffs[i] / validFIRs.size());
      }

      // Coefficient de variation moyen
      float meanCV = 0.0f;
      for (int i = 0; i < firLen; ++i) {
         if (std::abs(meanCoeffs[i]) > 1e-6f) {
            meanCV += stdCoeffs[i] / std::abs(meanCoeffs[i]);
         }
      }
      meanCV /= firLen;

      std::cout << "\nNombre de FIRs valides: " << validFIRs.size() << std::endl;
      std::cout << "Coefficient de variation moyen: " << (meanCV * 100.0f) << "%" << std::endl;

      // Statistiques MSE et SNR
      float meanMSE = 0.0f, meanSNR = 0.0f;
      for (const auto& fir : validFIRs) {
         meanMSE += fir.mse;
         meanSNR += fir.snr;
      }
      meanMSE /= validFIRs.size();
      meanSNR /= validFIRs.size();

      std::cout << "\nMSE moyen: " << meanMSE << std::endl;
      std::cout << "SNR moyen: " << meanSNR << " dB" << std::endl;

      // Évaluation de la cohérence
      std::cout << "\n=== ÉVALUATION ===" << std::endl;
      if (meanCV < 0.1f) {
         std::cout << "✓ Excellente cohérence (CV < 10%)" << std::endl;
      }
      else if (meanCV < 0.3f) {
         std::cout << "○ Cohérence acceptable (CV < 30%)" << std::endl;
      }
      else {
         std::cout << "✗ Faible cohérence (CV > 30%)" << std::endl;
         std::cout << "  → Les fichiers source/target sont probablement trop différents" << std::endl;
      }

      if (meanSNR > 20.0f) {
         std::cout << "✓ Excellente qualité de reconstruction (SNR > 20 dB)" << std::endl;
      }
      else if (meanSNR > 10.0f) {
         std::cout << "○ Qualité acceptable (SNR > 10 dB)" << std::endl;
      }
      else {
         std::cout << "✗ Faible qualité (SNR < 10 dB)" << std::endl;
         std::cout << "  → Un FIR simple ne suffit pas, traitement non-linéaire probable" << std::endl;
      }
   }

   void saveMeanFIR(const std::vector<AnalysisResult>& results) {
      std::vector<FIRCalculator::FIRFilter> validFIRs;

      for (const auto& r : results) {
         if (r.success) {
            validFIRs.push_back(r.fir);
         }
      }

      if (validFIRs.empty()) return;

      int firLen = validFIRs[0].coefficients.size();
      std::vector<float> meanCoeffs(firLen, 0.0f);

      for (const auto& fir : validFIRs) {
         for (int i = 0; i < firLen; ++i) {
            meanCoeffs[i] += fir.coefficients[i];
         }
      }
      for (int i = 0; i < firLen; ++i) {
         meanCoeffs[i] /= validFIRs.size();
      }

      // Sauvegarde en fichier texte
      std::ofstream outFile("fir_coefficients.txt");
      outFile << "# FIR Coefficients (moyenne de " << validFIRs.size() << " filtres)" << std::endl;
      outFile << "# Longueur: " << firLen << std::endl;
      outFile << std::scientific << std::setprecision(10);

      for (int i = 0; i < firLen; ++i) {
         outFile << meanCoeffs[i] << std::endl;
      }
      outFile.close();

      std::cout << "\n✓ Coefficients FIR moyens sauvegardés dans fir_coefficients.txt" << std::endl;
   }
};

// ============================================================================
// Main
// ============================================================================

int main(int argc, char* argv[]) {
   std::cout << "==================================================" << std::endl;
   std::cout << "  Cassette FIR Analyzer - Analyse et validation" << std::endl;
   std::cout << "==================================================" << std::endl;

   if (argc < 3) {
      std::cout << "\nUsage: " << argv[0] << " <source_dir> <target_dir> [fir_length]" << std::endl;
      std::cout << "\n  source_dir  : Répertoire contenant les WAV sources" << std::endl;
      std::cout << "  target_dir  : Répertoire contenant les WAV targets" << std::endl;
      std::cout << "  fir_length  : Longueur du FIR (défaut: 128)" << std::endl;
      std::cout << "\nExemple: " << argv[0] << " ./sources ./targets 256" << std::endl;
      return 1;
   }

   CassetteAnalyzer::Config config;
   config.sourceDir = argv[1];
   config.targetDir = argv[2];
   config.firLength = (argc > 3) ? std::atoi(argv[3]) : 128;

   CassetteAnalyzer analyzer(config);

   if (analyzer.analyze()) {
      std::cout << "\n=== ANALYSE TERMINÉE AVEC SUCCÈS ===" << std::endl;
      return 0;
   }
   else {
      std::cerr << "\n=== ERREUR PENDANT L'ANALYSE ===" << std::endl;
      return 1;
   }
}