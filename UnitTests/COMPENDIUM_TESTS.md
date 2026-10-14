# Couverture de tests du compendium CRTC

Source : *The Amstrad CPC CRTC Compendium* v1.11 (Logon System, `ACCC1.11-FR.pdf`,
CC BY-NC-ND 4.0). Ce fichier liste chaque section du compendium, l'état de sa
couverture par les tests unitaires de CPCCore et ce qui reste à tester.

Convention :
- un test écrit depuis le compendium qui échoue est préfixé `DISABLED_` : la
  liste des tests désactivés est la liste du travail d'émulation restant
  (`--gtest_also_run_disabled_tests --gtest_filter='Compendium_*'`) ;
- les nouveaux tests sont regroupés par chapitre : `TEST(Compendium_<chap>, ...)`,
  avec le § cité en commentaire ;
- les tests plus anciens (`CRTC_*`, `GateArray_*`, `Z80_IoTiming`) gardent leur nom
  et sont référencés ici.

Statuts : ✅ couvert · 🟡 partiel · ❌ à tester · ➖ non testable (texte didactique,
pas de comportement à vérifier).

## 1-3. Préambule, historique, généralités

| § | Sujet | Statut | Tests / à faire |
|---|---|---|---|
| 1, 2, 3 | Préambule, licence, terminologie | ➖ | |

## 4. CRTC & CPC

| § | Sujet | Statut | Tests / à faire |
|---|---|---|---|
| 4.1 | Table ROM européenne, durée d'un frame | ✅ | `CRTC_RegisterDefaults.MatchTheEuropeanRomTableAfterReset`, `CRTC_FrameTiming.StandardEuropeanFrameIs19968Microseconds` |
| 4.2 | Numérotation des CRTC | ➖ | |
| 4.3 | Vue générale des registres (bits par type) | 🟡 | masques R8/R9 : `CRTC_RegisterMasks.*`. À faire : masques de R0..R7, R12/R13 (bits utiles par type) |
| 4.4.1 | Accès au CRTC (ports) | ✅ | `CRTC_RegisterMasks.R9KeepsOnlyFiveBits` |
| 4.4.2 | Instructions Z80A d'I/O, cycles d'attente du GA | 🟡 | OUT : `Z80_IoTiming.*`. IN A,(C) : voir §7.2 (DISABLED, 3e µs au lieu de 4e). À faire : IN A,(n), INI/IND |
| 4.4.3 | Délais d'accès | 🟡 | `CRTC_BusInterface.*`, `Z80_IoTiming.OutCR8IsOneMicrosecondLaterOnAsicCrtc`. À faire : OUT(n),A (3e µs partout), OUTD, lectures |
| 4.4.4 | Dissection des OUT | ✅ | `CRTC_BusInterface.TState0IsTakenNow`, `Z80_IoTiming.*` |

## 5. Les autres circuits

| § | Sujet | Statut | Tests / à faire |
|---|---|---|---|
| 5.1 | Décodage d'adresse : A14=0, A9-A8 = fonction ; n° de registre et valeur tronqués ; une I/O atteint tous les circuits décodés | ✅ | `Compendium_5.CrtcDecodesA14AndA9A8Only`, `OneOutReachesEveryDecodedChip`, `GateArrayNeedsA14OneOnTheCpcOld` |
| 5.1 (*) | PAL/GATE ARRAY sélectionné avec A14=0 sur les machines à ASIC | ❌ | **DISABLED** : `Compendium_5.GateArraySelectedWithA14ZeroOnTheCpcPlus` (le GA exige A14=1 sur CPC+) |
| 5.2 | CPC+ : séquence de délockage ASIC (17 octets sur &BC00) | ✅ | `Compendium_5.AsicUnlockSequence`, `AsicUnlockWaitsForTheAcknowledgeByte`, `AsicSequenceWithAnotherStateLocks`, `AsicSequenceNeedsANonZeroRq00`, `AsicSequenceBrokenByAWrongByte`, `AsicSequenceIgnoredWithACrtc4` |

## 6. Construction d'un frame

| § | Sujet | Statut | Tests / à faire |
|---|---|---|---|
| 6.1.1 | Comptage des caractères | ✅ | `CRTC_HorizontalCounter.WrapsFromR0BackToZero` |
| 6.1.2 | Synchronisations | ✅ | `CRTC_VerticalSync.AssertedWhenC4ReachesR7`, `CRTC_SyncWidths.*` |
| 6.1.3 | Affichage des caractères (DISPEN) | ✅ | `CRTC_DisplayEnable.BorderAssertedAtR1AndClearedAtNewLine` |
| 6.1.4 | Pointeur vidéo : adresse = R12.5-4 → A15-A14, C9.2-0 → A13-A11, VMA.9-0 → A10-A1 | ❌ | adresse produite pour des valeurs choisies de R12/R13/C9 |
| 6.1.5 | Retards GA : affichage 1 µs après le pointeur CRTC | 🟡 | implicite dans `GateArray_*`. À faire : test explicite du caractère affiché vs C0 |

## 7. Synchronisation

| § | Sujet | Statut | Tests / à faire |
|---|---|---|---|
| 7.1 | Principes | ➖ | |
| 7.2 | VSYNC lue sur le port B du PPI (bit 0 = broche VSYNC du CRTC) ; IN A,(C) met A à jour à sa 4e µs (référence C0vs) | 🟡 | `Compendium_7.PpiPortBBit0IsTheCrtcVSyncPin`. **DISABLED** : `InReadsTheVSyncOnItsFourthMicrosecond` (l'IN émulé lit le port B à sa 3e µs) |
| 7.3 | FAKE VSYNC : port B en sortie, bit 0 force la VSYNC vue par le GA | ❌ | **DISABLED** : `Compendium_7.FakeVSyncIsSeenByTheGateArray` (propagation désactivée, `if (false)` dans `PPI8255::UpdateSignalForPortB`) |

## 8. Affichage, Z80A & Gate Array

| § | Sujet | Statut | Tests / à faire |
|---|---|---|---|
| 8.1 | LD (HL),r8 : écriture RAM vue par le GA selon l'octet | ❌ | **DISABLED** : `Compendium_8.LdHlR8` (le GA lit la RAM 1 µs trop tôt : un octet écrit pendant la µs de sa lecture n'est pas affiché) |
| 8.2 | LD (nn),HL (L à la 4e µs, H à la 5e) | ❌ | **DISABLED** : `Compendium_8.LdAaaaHl` (même cause) |
| 8.3 | PUSH r16 (octet fort à la 3e µs, faible à la 4e) | ❌ | **DISABLED** : `Compendium_8.PushR16` (même cause) |

## 9. Gate Array

| § | Sujet | Statut | Tests / à faire |
|---|---|---|---|
| 9.1 | Pixelisation : ordre des bits par mode (0, 1, 2, 3) | ✅ | `Compendium_9.PixelBitsOfEachMode` |
| 9.1 | Mode 2 en avance d'1 pixel (GA 40007/40008/40010, ASIC 40226 ; pas ASIC 40489) | 🟡 | CPC+ aligné : `Compendium_9.PixelsOfEveryModeAlignedOnTheCpcPlus`. **DISABLED** : `Mode2PixelsAreOnePixelEarlyOnTheGateArray40010` (type de GA non émulé) |
| 9.2.1 | Pixels parasites bord/mode 2 | ➖ | défaut d'alimentation, non émulé |
| 9.2.2 | Instant de prise en compte d'une encre : 2e octet du caractère affiché pendant la µs de l'I/O, quel que soit le mode (CRTC 4 : 1 pixel plus tôt en mode 2) | ❌ | **DISABLED** : `Compendium_9.InkTakenOnTheSecondByteOfTheIoMicrosecond` (1 µs trop tard), `InkTakenOnTheSecondByteOfTheIoMicrosecondOnTheCpcPlus` (1 µs trop tard, dépend du mode), `InkOnePixelEarlierInMode2OnTheAsic40226` |
| 9.3.1 | Changement de mode : pris pendant la HSYNC-GA (≥ 2 µs), registre mis à jour à la 3e µs de l'OUT | ❌ | **DISABLED** : `Compendium_9.ModeChangeNeedsAHSyncOfTwoMicroseconds` (le mode change avec une HSYNC de 1 µs) |
| 9.3.2 | Fenêtre de changement de mode CRTC 0/1/2 (écriture ≤ C0vs 51 avec R2=46, R3=14) | ✅ | `Compendium_9.ModeChangeWindowCrtc012` |
| 9.3.3 | Fenêtre de changement de mode CRTC 3/4 (écriture ≤ C0vs 52) | ✅ | `Compendium_9.ModeChangeWindowCrtc34` |
| 9.3.4 | Mode splitting : pixels « malaxés » (ancien mode sur 1 pixel M2, puis bits réutilisés) | ❌ | Relevé p56-73. **DISABLED** : `Compendium_9.ModeSplittingPixelsCrtc0/1/2` (§9.3.4.3, R2.NJIT, GA 40010 : pixel 36 ancien mode — noir sur CRTC 1 —, pixels 37-40 nouveau mode), `ModeSplittingPixelsCrtc4` (§9.3.4.5, ASIC 40226 : 1 pixel ancien mode puis 6 nouveaux après le noir). Le GA émulé change de mode par bloc entier. Reste : §9.3.4.4 R3.JIT (tableaux p65-68 relevés, test à écrire avec R3 écrit au T-state) et variantes GA 40007/40008 |

## 10. Registre R9

| § | Sujet | Statut | Tests / à faire |
|---|---|---|---|
| 10.1 | R9 sur 5 bits | ✅ | `CRTC_RegisterMasks.R9KeepsOnlyFiveBits` |
| 10.2 | Prise en compte de R9 tant que C0 ≤ R0 | ✅ | `Compendium_10.R9WrittenDuringTheLineEndsTheRow` (5 types), `CRTC_Crtc0Vertical.R9WrittenDuringTheLine`, `CRTC_Crtc1.R9WrittenDuringTheLine` |
| 10.3.1 | Règles de comptage CRTC 0 | ✅ | `CRTC_Crtc0Vertical.*` |
| 10.3.2 | CRTC 1 | ✅ | `CRTC_Crtc1.R9WrittenDuringTheLine` |
| 10.3.3 | CRTC 2 (débordement de C9, R9 lié à la dernière ligne) | 🟡 | débordement : `Compendium_10.R9BelowC9`. À faire : R9 écrit pendant la HSYNC ignoré pour la dernière ligne |
| 10.3.4 | CRTC 3/4 : comparaison C9 ≥ R9, pas de débordement | ✅ | `CRTC_AsicVertical.C9CannotOverflow` |
| 10.3 | Tableaux p79-80 (R9 7→0 selon C9, C4=R4 ou non, R9 0→n) | 🟡 | cas R9 < C9 avec C4≠R4 : `Compendium_10.R9BelowC9`. À faire : cas C4=R4 et R9 0→n |

## 11. Registre R5

| § | Sujet | Statut | Tests / à faire |
|---|---|---|---|
| 11.1 | R5 sur 5 bits, ligne interlace en plus | ✅ | `Compendium_11.R5KeepsFiveBits`, `CRTC_Compendium.InterlaceAddsALineEveryOtherFrame` |
| 11.2.1-11.2.2 | Ajustement CRTC 0 | ✅ | `CRTC_Crtc0Vertical.AdjustmentCountsC9UpToR5`, `R5IsSampledUntilC0Equals2` |
| 11.2.3-11.2.4 | Ajustement CRTC 1 | ✅ | `CRTC_Crtc1.AdjustmentUsesC5`, `R4WrittenOnTheLastLine` |
| 11.2.5 | Ajustement CRTC 2 (C4++ à chaque C9=R9, R4/R9 pris pendant l'additionnel) | 🟡 | `CRTC_Compendium.Crtc2AdjustmentUsesC5`. À faire : écritures R4/R9 pendant l'additionnel |
| 11.2.6 | Ajustement CRTC 3/4 | ✅ | `CRTC_AsicVertical.AdjustmentKeepsC4AtR4` |
| 11.3.1 | R5 modifié pendant l'ajustement, CRTC 0/2 (R5=C5+1 → fin ; R5<C5+1 → débordement) | 🟡 | CRTC 0 : `CRTC_Crtc0Vertical.AdjustmentCountsC9UpToR5`. À faire : CRTC 2, débordement jusqu'à 31 |
| 11.3.2 | CRTC 1 (bug R5=0 pendant l'additionnel) | ✅ | `CRTC_Crtc1.R5ZeroDuringTheAdjustment` |
| 11.3.3 | CRTC 3/4 (fin immédiate) | ✅ | `CRTC_AsicVertical.R5LoweredEndsTheAdjustment` |
| 11.4.1 | R5 avant l'ajustement, CRTC 1/2/3/4 (évalué à chaque C0) | ✅ | `Compendium_11.R5WrittenAtTheEndOfTheLastLine` |
| 11.4.2 | CRTC 0 (R5 écrit après C0=2 ignoré) | ✅ | `CRTC_Crtc0Vertical.R5IsSampledUntilC0Equals2` |
| 11.5.1-11.5.4 | VSYNC en ajustement (C4 atteint en ajustement : R4+1 sur CRTC 0, R4+1/R4+2 sur 1/2, R4 sur 3/4) | ✅ | `Compendium_11.VSyncDuringTheAdjustment`. Restent : CRTC 0 bloquée si R7 écrit à C0<2, CRTC 2 fantôme pendant la HSYNC (couverts par §16.4.1, §15.4.4) |
| 11.6 | RFD (R5 0→≠0 à C0=R0) | ✅ | `CRTC_Crtc1.RuptureForDummies` |
| 11.6.1 | RFD et parité | ✅ | `CRTC_Compendium.Crtc1RfdParity` |
| 11.6.2-11.6.3 | IVM ON/OFF | ✅ | `CRTC_Compendium.Crtc1IvmOnOffFixesTheParity` |
| 11.6.4 | RFD et autres CRTC (sans effet) | ✅ | `Compendium_11.RfdOnlyOnCrtc1` |
| 11.7 | R6 et ajustement (border en ajustement ; CRTC 1 split-border) | ✅ | `Compendium_11.R6BorderInTheAdjustment` (0/1/2), `CRTC_AsicVertical.R6TestedAtTheLineStartOnly` (3/4) |
| 11.8 | Ajustement en interlace (R5 lignes sur chaque frame) | ✅ | `Compendium_11.AdjustmentOnEveryInterlacedFrame` (CRTC 0/1/3/4 ; CRTC 2 a une logique différente, non testée) |
| 11.9 | Ligne d'ajustement interlace (CRTC 2 : R8=0 pendant la ligne → last line annulée) | 🟡 | `CRTC_Compendium.Crtc1InterlaceLineAfterR5`. À faire : CRTC 2 |

## 12. Registre R4

| § | Sujet | Statut | Tests / à faire |
|---|---|---|---|
| 12.1 | C4 sur 7 bits (débordement jusqu'à 127) | ✅ | `Compendium_12.C4OverflowsUpTo127` |
| 12.2 | CRTC 0 | ✅ | `CRTC_Crtc0Vertical.R4WrittenOnC0Equals0Or1OfTheLastLine` |
| 12.2.1 | RLAL CRTC 0 | ✅ | `CRTC_Crtc0Vertical.LineToLineRupture` |
| 12.3 | CRTC 1 (+ RLAL) | ✅ | `CRTC_Crtc1.R4WrittenOnTheLastLine`, `Compendium_12.Crtc1R4EqualsC4BeforeTheLastLineOfTheRow`, `Compendium_12.LineToLineRuptureCrtc134` |
| 12.4 | CRTC 2 | ✅ | voir 12.4.1-12.4.2 |
| 12.4.1 | Concept de dernière ligne (DL, GDL, DLP) | ✅ | `CRTC_Compendium.Crtc2HSyncOnC0ZeroCancelsTheLastLine`, `Crtc2PreviousLastLine` |
| 12.4.2 | RLAL CRTC 2 | ✅ | `CRTC_Compendium.Crtc2LineToLineRupture` |
| 12.5 | CRTC 3/4 (+ exception CRTC 3 : R4=0 avec « ROM select » simultané) | 🟡 | `CRTC_AsicVertical.C4Overflows`, RLAL : `Compendium_12.LineToLineRuptureCrtc134`. À faire : exception CRTC 3 |

## 13. Registre R0

| § | Sujet | Statut | Tests / à faire |
|---|---|---|---|
| 13.1 | C0 de 0 à R0 | ✅ | `CRTC_HorizontalCounter.WrapsFromR0BackToZero` |
| 13.2.1 | CRTC 0 : les 3 premières µs | 🟡 | couvert indirectement par `CRTC_Crtc0Vertical.*`. À faire : un test par position C0=0/1/2 |
| 13.2.2 | Gel de VSYNC | ✅ | `CRTC_Crtc0Vertical.VSyncFrozenByAShortLine` |
| 13.2.3-13.2.4 | Gel de ligne additionnelle, gel de C9 | ✅ | `CRTC_R0Zero.*` |
| 13.2.5 | R0=1 | ✅ | `CRTC_Crtc0Vertical.R0EqualsOneAlternatesC4` |
| 13.2.6 | R0=0 | ✅ | `CRTC_R0Zero.*` |
| 13.2.7 | RVLL CRTC 0 | ✅ | `Compendium_13.Crtc0RvllHiddenTwoMicrosecondLines` (tableau 1 p113), `Crtc0RvllWithR2Zero` (tableau 2). Relevé : chaque écriture de R0 tombe sur C0=0 de la ligne qu'elle dimensionne |
| 13.3 | CRTC 1 : R0 libre (R0=0 normal) ; bug OUTI | ✅ | `Compendium_13.R0ZeroIsANormalOneMicrosecondLine`. Bug OUTI (C0 déborde « dans certains cas ») : ➖ non déterministe |
| 13.3.1 | RVI (tableaux p116-118) | ✅ | `Compendium_13.Crtc1Rvi` (64 transitions), `AsicRvi` (lignes de compatibilité CRTC 3/4). Coquille : ligne C9 1→7 (3/4), R0.1 vaut 50 d'après sa séquence C0, pas 49 |
| 13.4 | CRTC 2 : traitements inhibés pendant la HSYNC, ½ µs de border avant C0=0, R0 libre | 🟡 | `CRTC_Compendium.Crtc2*`. **DISABLED** : `Compendium_13.Crtc2R0ZeroCountsNormally` (avec R0=0, le CRTC 2 émulé reste à C4=C9=0). À faire : R2=0 obligatoire |
| 13.4.1 | RVLL CRTC 2 | 🟡 | `Compendium_13.Crtc2Rvll` (lignes cachées ≥ 2 µs). **DISABLED** : `Crtc2RvllOneMicrosecondLines` (lignes cachées de 1 µs, C9 5-15 : même cause que §13.4) |
| 13.5 | CRTC 3/4 : R0=0 sans effet de bord | ✅ | `Compendium_13.R0ZeroIsANormalOneMicrosecondLine` |
| 13.6.1-13.6.3 | Chronogrammes de mise à jour de R0 | 🟡 | `CRTC_R0Zero.Crtc0KeepsR0ZeroAndC0StaysAtZero`. À faire : chronogrammes p124-125 par type |
| 13.7.1 | CRTC 1 : RFD par R0 prolongé (§13.7.1.2) | ❌ | non modélisé (DISABLED attendu) |
| 13.7.2 | CRTC 0 : R0 agrandi à C0=1 de la dernière ligne | ✅ | `CRTC_Crtc0Vertical.R0EnlargedOnC0Equals1OfALastLine` |
| 13.8.1-13.8.3 | Offset selon C0 (écrans de 4, 2, 1 µs) | ✅ | `Compendium_13.OffsetOnEveryShortLineCrtc134`, `Crtc0R0ZeroNeverTakesTheOffset`, `CRTC_Crtc0Vertical.R0EqualsOneAlternatesC4`. Chronogrammes fins p128-131 non relevés |

## 14. Registre R3

| § | Sujet | Statut | Tests / à faire |
|---|---|---|---|
| 14.1 | R3 : bits par type | ✅ | `CRTC_SyncWidths.VSyncWidthFollowsR3HighNibbleOnCrtc034Only` |
| 14.2 | Longueur VSYNC (R3h, 0=16, réécriture pendant la VSYNC) | ✅ | `CRTC_SyncWidths.VSyncLinesOnThePin`, `Crtc0ShorteningR3hDuringVSync` |
| 14.2 | CRTC 3/4 : C-VSYNC exige une VSYNC CRTC encore active à la 2e HSYNC (R3h=1 casse la synchro) | ❌ | |
| 14.3 | HSYNC : GA vs CRTC (noir, C-HSYNC ≤ 4 µs) | ✅ | `GateArray_HSyncBlack.BlackForTheWholeHSync` |
| 14.4 | HSYNC et position écran (tables C-HSYNC p135) | 🟡 | `GateArray_CHSync.*`. À faire : décalage de l'image par valeur de R3l < 6 |
| 14.5.1 | R3 modifié pendant la HSYNC, CRTC 0/2 (débordement de C3l, 0 à atteindre) | ✅ | `Compendium_14.R3lBelowC3lOverflows`, `CRTC_HSyncReentrancy.WritingR3lZeroDuringHSync` |
| 14.5.2 | CRTC 1 (R3l=0 annule la HSYNC) | ✅ | `CRTC_HSyncReentrancy.WritingR3lZeroDuringHSync`, `CRTC_Jit.R3ZeroDuringTheHSync` |
| 14.5.3 | CRTC 3/4 | 🟡 | débordement : `Compendium_14.R3lBelowC3lOverflows`. À faire : pas de R3.JIT sur 3/4 |
| 14.5.4 | R3.JIT | ✅ | `CRTC_Jit.*`, `GateArray_CHSync.R3JitMovesTheEndByATState` |
| 14.6 | Absence de HSYNC (R3l=0) | ✅ | `CRTC_HSyncReentrancy.NoHSyncEndWithR3lZeroOnCrtc01`, `CRTC_SyncWidths.HSyncWidthOnThePin` |
| 14.7.1 | Démarrage HSYNC CRTC 0/1/2 (R2.JIT) | ✅ | `CRTC_Jit.R2Jit*`, `GateArray_CHSync.R2JitKeepsTheCHSync` |
| 14.7.2 | Démarrage HSYNC CRTC 3/4 (1 µs plus tard) | ✅ | `CRTC_HSyncPin.AsicHSyncIsOneMicrosecondLate` |
| 14.7 | Position du noir HSYNC en pixels M2 par type (CRTC 0 : 5e, 1 : 6e, 2 : 4e, 3 : 17e, 4 : 19e) | ✅ | `GateArray_HSyncBlack.R2ProgrammedBefore` |
| 14.8 | HSYNC et interruptions | ✅ | `Compendium_27.InterruptPositionFollowsR3` |
| 14.9 | Schémas HSYNC : fin du noir VSYNC GA 1 pixel M2 après la fin de HSYNC (CRTC 0/1), simultanée (2/3/4) | ❌ | |

## 15. Registre R2

| § | Sujet | Statut | Tests / à faire |
|---|---|---|---|
| 15.1 | Généralités | ➖ | |
| 15.2.1-15.2.2 | HSYNC avec R2 prédéfini (C0=R2 non géré pendant la HSYNC) | ✅ | `Compendium_15.R2RewrittenDuringTheHSync` |
| 15.3.1-15.3.2 | Réentrance, HSYNC infinie | ✅ | `CRTC_HSyncReentrancy.*`, `CRTC_R0Zero.Crtc0HSyncIsProtected` |
| 15.3.3 | CRTC 0 : R3l modifié à la position de réentrance → nouvelle HSYNC sans remise à 0 de C3l | ❌ | |
| 15.3.4 | CRTC 1/2 (CRTC 1 : 2e C-HSYNC, CRTC 2 : non) | 🟡 | `CRTC_HSyncReentrancy.Crtc1OverflowsWithAnInvisibleRestart`. À faire : C-HSYNC côté GA |
| 15.3.5 | CRTC 3/4 | ✅ | `CRTC_HSyncReentrancy.Crtc234Overflow` |
| 15.4.1-15.4.3 | VSYNC pendant la HSYNC, CRTC 0/1/3/4 (sans problème) | ✅ | `Compendium_15.VSyncDuringTheHSync` |
| 15.4.4 | CRTC 2 : VSYNC fantôme | ✅ | `CRTC_Compendium.Crtc2GhostVSync`, `Crtc2NoGhostVSyncWithR2Zero` |
| 15.5.1 | DISPEN pendant la HSYNC, CRTC 0/1/3/4 | ✅ | `Compendium_15.BorderLiftedDuringTheHSync` |
| 15.5.2 | CRTC 2 | ✅ | `CRTC_Compendium.Crtc2BorderNotLiftedDuringHSync` |
| 15.6 | CRTC 2 et HSYNC (récapitulatif, HSYNC qui déborde → C4 déborde) | 🟡 | `CRTC_Compendium.Crtc2HSyncOnC0ZeroCancelsTheLastLine`. À faire : 2 HSYNC/ligne |
| 15.7.1-15.7.2 | Changer R2 sans perdre la synchro moniteur | ❌ | test moniteur : R2 46↔50 avec R0 temporaire, image stable |

## 16. Registre R7

| § | Sujet | Statut | Tests / à faire |
|---|---|---|---|
| 16.1 | VSYNC quand C4 atteint R7 | ✅ | `CRTC_VerticalSync.AssertedWhenC4ReachesR7` |
| 16.2.1 | Affichage de la zone VSYNC (noir V26) | ✅ | `GateArray_VSyncBlack.TwentySixHSyncWhateverTheCrtcVSync` |
| 16.2.1 | Début du noir VSYNC par type (CRTC 0/2 : 5e pixel, 1 : 6e, 4 : 2e) | ❌ | |
| 16.2.2 | Signal C-SYNC moniteur (V26=2 → C-VSYNC on, 6 → off) | 🟡 | `GateArray_CHSync.LengthFollowsR3l` (H06). À faire : C-VSYNC |
| 16.2.3 | Gestion de C-SYNC (XNOR, nouvelle VSYNC pendant V26 → remise à 0) | 🟡 | `GateArray_VSyncBlack.*`. À faire : remise à 0 de V26 |
| 16.2.4 | Tolérances du moniteur | ❌ | |
| 16.2.5 | Interactions CRTC/GA (HSYNC débordant sur la ligne C4=R7 → V26=1 immédiat) | ❌ | |
| 16.3 | Protection de VSYNC (1) et (2) | ✅ | `CRTC_VerticalSync.RewritingR7DoesNotRestartTheVSync`, `CRTC_AsicVertical.InfiniteVSyncWithoutProtection` |
| 16.4.1 | Conditions CRTC 0 (VSYNC bloquée, gel C3h) | ✅ | `CRTC_Crtc0Vertical.R7WrittenDuringALine`, `VSyncCounterFrozenWithR0Zero`, `VSyncFrozenByAShortLine` |
| 16.4.2 | CRTC 1 | ✅ | `CRTC_Compendium.Crtc1VSyncStartedDuringALine` |
| 16.4.3 | CRTC 2 (évaluée à toutes les valeurs de C0/C9) | ✅ | `Compendium_16.Crtc2R7WrittenDuringALine` |
| 16.4.4 | CRTC 3/4 | ✅ | `CRTC_AsicVertical.R7WrittenWithC4StartsNoVSync` |
| 16.5.1-16.5.4 | VSYNC différée (interlace) | ✅ | `CRTC_Compendium.InterlaceMidVSync`, `IvmVSyncDelayedOnOddC4` |
| 16.6 | VSYNC sans limites (scroll vertical par la 2e HSYNC) | ❌ | test moniteur |
| 16.7 | Le bon moment | ➖ | |

## 17. Registre R1

| § | Sujet | Statut | Tests / à faire |
|---|---|---|---|
| 17.1 | DISPEN on à C0=0, off à C0=R1 ; pas après un débordement de C0 | ✅ | `CRTC_DisplayEnable.*`, `CRTC_Compendium.Crtc0NoDisplayAfterC0Overflow` |
| 17.2.1 | R1 ≤ R0 | ✅ | `CRTC_DisplayEnable.BorderAssertedAtR1AndClearedAtNewLine` |
| 17.2.2 | R1 > R0 : lignes répétées | ✅ | `CRTC_Compendium.R1GreaterThanR0RepeatsTheRows` |
| 17.3 | Mise à jour dynamique de R1 (2e égalité C0=R1 recharge VMA') | ✅ | `Compendium_17.SecondC0EqualsR1LoadsVmaPrimeAgain` (0/1/3/4) |
| 17.4.1 | VMA'/VMA quand C4=0, CRTC 0/3/4 | ✅ | `Compendium_17.FirstRowAtR12R13AndR1AboveR0` |
| 17.4.2 | CRTC 1 | ✅ | `CRTC_Crtc1.OffsetOnEachLineOfTheFirstCharacter` |
| 17.4.3 | CRTC 2 (+ bug AND VMA' avec R1=0) | 🟡 | `CRTC_Compendium.Crtc2OffsetTakenAtR1OfTheLastLine`. À faire : bug AND (non modélisé) |
| 17.5.1-17.5.2 | Prise en compte de R1=0 | ❌ | chronogrammes p187 |
| 17.6.1 | R1=R0 et C0=R0 : 1 µs de border | ✅ | `Compendium_17.R1EqualsR0GivesOneBorderMicrosecond`, `CRTC_SkewDispTmg.R1EqualsR0` |
| 17.6.2 | R1>R0 et C0=R0 : octet de border CRTC 0/2 | ✅ | `CRTC_Compendium.BorderByteWhenR1GreaterThanR0`, `Crtc0R0ZeroAlternatesBytes` |

## 18. Registre R6

| § | Sujet | Statut | Tests / à faire |
|---|---|---|---|
| 18.1 | Border quand C4=R6, prioritaire sur R1 | ✅ | `Compendium_18.R6BorderBeforeC0EqualsR1` |
| 18.2.2 | Délais CRTC 0/2 (immédiat, définitif) | ✅ | `CRTC_Compendium.Crtc02R6BorderIsDefinitive` |
| 18.2.3 | CRTC 1 | ✅ | `CRTC_Crtc1.R6ZeroBorder` |
| 18.2.4 | CRTC 3/4 (testé au début de la ligne) | ✅ | `CRTC_AsicVertical.R6TestedAtTheLineStartOnly` |
| 18.3.2 | Conflits R6=0, CRTC 0/2 | ✅ | `CRTC_Compendium.R6ZeroOnTheFirstLine`, `R6ZeroConflictResolvedOnC0EqualsR1` |
| 18.3.3 | CRTC 1 (exception sur le dernier caractère du frame) | 🟡 | `CRTC_Crtc1.R6ZeroBorder`. À faire : exception du dernier caractère |
| 18.3.4 | CRTC 3/4 (R6=0 sans traitement spécial) | ✅ | `Compendium_18.AsicR6Zero` |

## 19. Registre R8

| § | Sujet | Statut | Tests / à faire |
|---|---|---|---|
| 19.1 | Bits de R8 par type | ✅ | `CRTC_RegisterMasks.R8*` |
| 19.2.1 | BORDER ON | ✅ | `CRTC_SkewDispTmg.DelaysTheBorderOnCrtc034` |
| 19.2.2 | BORDER OFF | ✅ | `Compendium_19.BorderOffDuringALine` |
| 19.2.3 | Délai +1/+2 | ✅ | `CRTC_SkewDispTmg.DelaysTheBorderOnCrtc034`, `R1EqualsR0`, `NoSkewOnCrtc12` |
| 19.2.4 | Absence de condition C0=R1 (R1>R0) avec skew | ❌ | |
| 19.2.5 | Désintégration du border CRTC 0 | ✅ | `CRTC_SkewDispTmg.Crtc0BorderDisintegration` |
| 19.3.1-19.3.2 | Interlace : IS / IVM | ✅ | `CRTC_Compendium.InterlaceAddsALineEveryOtherFrame`, `InterlaceVideoModeAlternatesTheLines` |
| 19.3.3-19.3.4 | Restrictions, fonction mal aimée (BORDER ON + interlace) | ❌ | |
| 19.4.1-19.4.4 | Programmation verticale en interlace par type | 🟡 | `CRTC_Compendium.InterlaceVideoModeAlternatesTheLines`. À faire : R9=N-2 (0, 3/4), N-1 (1, 2), R9=0 en IVM |
| 19.5.2 | Parité CRTC 0 | ✅ | `CRTC_Compendium.InterlaceParityFrozenByR6`, `IvmVSyncDelayedOnOddC4` |
| 19.5.3 | Parité CRTC 1 (tables p211-212) | 🟡 | `CRTC_Compendium.Crtc1IvmOnOffFixesTheParity`. À faire : tables complètes |
| 19.5.4 | Parité CRTC 2 (SPLITC9) | ✅ | `Compendium_19.Crtc2SplitC9` |
| 19.5.5 | Parité CRTC 3/4 | ✅ | `CRTC_Compendium.IvmVSyncDelayedOnOddC4` |
| 19.6.1 | Ligne additionnelle interlace CRTC 0 | ✅ | `CRTC_Compendium.InterlaceParityFrozenByR6` |
| 19.6.2 | CRTC 1 | ✅ | `CRTC_Compendium.Crtc1InterlaceLineAfterR5` |
| 19.6.3 | CRTC 2 (+ bug IVM coupé pendant la ligne additionnelle) | 🟡 | `CRTC_Compendium.InterlaceParityFrozenByR6`. À faire : bug (non modélisé) |
| 19.6.4 | CRTC 3/4 | ✅ | `CRTC_Compendium.InterlaceAdditionalLineCounters` |
| 19.7 | MID-VSYNC (ordre parité/VSYNC avec R7=0 : 0/1/2 vs 3/4) | 🟡 | `CRTC_Compendium.InterlaceMidVSync`. À faire : cas R7=0 |
| 19.8.1 | Comptage IVM CRTC 0 | ✅ | `CRTC_Compendium.Crtc0Ivm*` |
| 19.8.2 | Comptage IVM CRTC 1 | ✅ | `Compendium_19.Crtc1IvmCounting` (R9 impair) |
| 19.8.3 | Comptage IVM CRTC 2 | ✅ | `CRTC_Compendium.Crtc2IvmCounter` |
| 19.8.4 | Comptage IVM CRTC 3/4 (tables p237-240) | ✅ | `Compendium_19.AsicIvmCounting` (R9 pair ; tables complètes non relevées) |

## 20. Registres R12/R13

| § | Sujet | Statut | Tests / à faire |
|---|---|---|---|
| 20.1-20.2 | Calcul du pointeur vidéo | ❌ | voir §6.1.4 (test machine à écrire) |
| 20.3.1 | Mise à jour CRTC 0 (C4=C9=C0=0) | ✅ | `Compendium_20.OffsetTakenAtTheFrameStart` |
| 20.3.2 | CRTC 1 (à chaque C0=0 tant que C4=0) | ✅ | `CRTC_Crtc1.OffsetOnEachLineOfTheFirstCharacter` |
| 20.3.3 | CRTC 2 (à C0=R1 de la dernière ligne) | ✅ | `CRTC_Compendium.Crtc2OffsetTakenAtR1OfTheLastLine` |
| 20.3.4 | CRTC 3/4 | ✅ | `Compendium_20.OffsetTakenAtTheFrameStart` |
| 20.4 | Délais de prise en compte (8 µs entre R13 et R12, §24.1) | ❌ | |
| 20.5 | Overscan bits (R12 bits 3-2 = 11 → retenue sur la page) | ✅ | `Compendium_20.OverscanBitsCarryIntoThePage` |

## 21. Registres en lecture

| § | Sujet | Statut | Tests / à faire |
|---|---|---|---|
| 21.2.1-21.2.3 | Lecture du contenu des registres | ✅ | `CRTC_RegisterRead.Crtc0/Crtc1/Crtc2/Crtc34UseAThreeBitTable` |
| 21.3.2 | Status CRTC 0/2 (haute impédance) | 🟡 | `CRTC_RegisterRead.StatusPort` |
| 21.3.3 | Status CRTC 1 (bit 5) | ✅ | `CRTC_Crtc1.StatusBorderR6` |
| 21.3.4 | Status CRTC 3/4 | ✅ | `CRTC_Compendium.AsicStatus1`, `AsicStatus2` |
| 21.4 | Pas de dummy register sur CPC | ❌ | |

## 22-25. Autres registres, fullscreen, trucs et astuces, temps fixe

| § | Sujet | Statut | Tests / à faire |
|---|---|---|---|
| 22 | Curseur, light pen | ➖ | non câblés sur CPC |
| 23 | Fullscreen et centrage (48 × 272 caractères visibles sur CTM) | ❌ | test moniteur : zone visible |
| 24.1-24.10 | Trucs et astuces | ➖ | techniques de programmation (24.5 couvert par §4.4) |
| 25 | Temps fixe | ➖ | |

## 26. Durées des instructions Z80A sur CPC

| § | Sujet | Statut | Tests / à faire |
|---|---|---|---|
| 26 | Durée en µs de chaque instruction (alignement 4T par le GA) | ✅ | `Compendium_26.InstructionDurations` : chaque ligne du tableau p281-282 (une ou plusieurs variantes d'encodage, branches prises/non prises, répétitions BC/B = 2/1). Colonne I/O (position de l'I/O dans l'instruction) : voir §4.4.3. Coquille du tableau : LD HX/LX,r fait 2 octets, pas 3 |

## 27. Interruptions

| § | Sujet | Statut | Tests / à faire |
|---|---|---|---|
| 27.1 | R52 compte les fins de HSYNC (même R3=1), 6 interruptions par frame | ✅ | `Compendium_27.R52CountsEveryHSyncEndEvenWithR3One`, `SixInterruptsPerFrame` |
| 27.2 | Remise à 0 : dépassement de 51, RMR bit 4, 2e HSYNC après la VSYNC ; effacement du bit 5 à l'acquittement | 🟡 | `Compendium_27.VSyncResetsR52AtTheEndOfTheSecondHSync`, `AcknowledgeClearsBit5OfR52`. **DISABLED** : `R52WrapsDuringTheVSyncWindow` (R52 = 52 non testé pendant les 2 HSYNC de la VSYNC : interruption 1 ligne trop tard) |
| 27.2 | RMR bit 4 à C0=R2+R3-1 prioritaire sur l'incrément ; à R2+R3-2 : remise à 0 puis incrément | 🟡 | `Compendium_27.RmrResetOneMicrosecondBeforeTheLastOneOfTheHSync`. **DISABLED** : `RmrResetOnTheLastMicrosecondOfTheHSyncWins` (écritures GA sans T-state) |
| 27.3.1 | Armement quand R52 reboucle à 0 ; une seule interruption en attente | ✅ | `Compendium_27.OnlyOneInterruptPending` |
| 27.3.2 | Armement pendant la VSYNC (seulement si bit 5 = 1) | ✅ | `Compendium_27.VSyncRequestsAnInterruptOnlyIfBit5IsSet` |
| 27.3.3 | Z80 : EI retarde d'une instruction, HALT | ✅ | `Compendium_27.EiDefersTheInterruptByOneInstruction`, `HaltIsInterruptedLikeANopSled` |
| 27.4 | IM 1 : appel en #38 = 5 µs (RST #38 = 4 µs) | ✅ | `Compendium_27.Im1CallLastsFiveMicroseconds` |
| 27.5 | IM 2 : 7 µs | ❌ | **DISABLED** : `Compendium_27.Im2CallLastsSevenMicroseconds` (8 µs dans l'émulation) |
| 27.6.1-27.6.5 | Instant de l'interruption par type (R3=14 : 15 µs après C0vs=R2 sur 0/1/2, 16 sur 3/4 ; R3=0 : aucune sur 0/1, 17 sur 2, 18 sur 3/4) | ✅ | `Compendium_27.InterruptPositionFollowsR3` |
| 27.7.1 | Course incrément de R52 / effacement du bit 5 | ➖ | non déterministe sur le matériel (dépend de la durée réelle de l'instruction qui suit EI) |
| 27.7.2 | Fiabilité : fin de HSYNC sur le dernier T d'une instruction → interruption après l'instruction suivante | ❌ | à concevoir : placer la fin de HSYNC au T-state près (R3.JIT), NOP vs ADD HL,DE |

## 28. Identification CRTC

Chaque méthode est un test de synthèse : elle doit distinguer les types comme sur le vrai matériel.

| § | Sujet | Statut | Tests / à faire |
|---|---|---|---|
| 28.1.1 | Débordement de C4/C9 (R4=36, R9=7, R5=16 : VSYNC perdue au-delà de R7=36/37/38) | ✅ | `Compendium_11.VSyncDuringTheAdjustment` |
| 28.1.2 | VSYNC pendant la HSYNC | ✅ | `CRTC_Compendium.Crtc2GhostVSync` |
| 28.1.3 | Prise en compte de la VSYNC (R7=C4 avec C0>0) | 🟡 | tests §16.4. À faire : test de synthèse par type |
| 28.1.4 | Longueur de la VSYNC | ✅ | `CRTC_SyncWidths.VSyncLinesOnThePin` |
| 28.1.5 | Longueur de la HSYNC (R3=0) | ✅ | `CRTC_SyncWidths.HSyncWidthOnThePin` |
| 28.1.6 | Border, visuellement | ✅ | tests §15.5, §17.6, §18.3, §19.2 |
| 28.1.7 | Mode interlace (R9=6/7 → VSYNC 2× plus vite sur 0/1) | ✅ | `Compendium_28.IvmVSyncTwiceAsEarly` |
| 28.1.8 | Status &BE00 | ✅ | `CRTC_RegisterRead.StatusPort`, `CRTC_Crtc1.StatusBorderR6` |
| 28.1.9 | Lecture &BF00 | ✅ | `CRTC_RegisterRead.*` |
| 28.1.10 | Status R10/R11 (bit 3 de STATUS 2 : CRTC 3 vs 4) | 🟡 | `CRTC_Compendium.AsicStatus2` |

## 29. Identification CPC

| § | Sujet | Statut | Tests / à faire |
|---|---|---|---|
| 29.1.1 | Fonctions étendues : délockage sur CPC+ seulement | ✅ | voir §5.2 |
| 29.1.2 | Bug PPI port C : le registre de commande ne remet pas le port C à 0 sur CPC+ | ✅ | `Compendium_29.PpiControlWordResetsPortCExceptOnTheCpcPlus` |
| 29.1.3 | Bug PPI port B : port B en sortie non relisible sur le CPC « low-cost » | 🟡 | CPC « old » relisible : `Compendium_29.PpiPortBOutputReadsBackOnTheCpcOld`. Low-cost : ➖ (non confirmé par le compendium, pas de modèle de machine low-cost) |
