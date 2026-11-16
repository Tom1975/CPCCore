# Étude de l'AFC du moniteur CTM644 (IC401 = Sanyo LA7800)

But : reconstruire un modèle électronique de la synchro ligne du moniteur à partir du schéma
(manuel de service CPC6128 / CTM644, page 26) et des photos « Real Hardware » de SHAKER. Les valeurs
que ni le schéma ni la fiche du LA7800 ne donnent sont tirées des photos.

État au 2026-11-16 : modèle ajusté et **porté** dans `CPCCoreEmu/Monitor.cpp` (modèle provisoire, entrée P2 de
`UnitTests/RealCpc/README.md`, partie 1 bis).

## Modèle

| Élément | Source | Modèle |
|---|---|---|
| Broche 16, dent de scie | R406 6,2 k, C403 180 pF, C404 39 nF | impulsion de retour intégrée (montée 192 px, descente 832 px) + créneau 0,23 par C403 / C404 |
| Comparateur de phase | interne au LA7800 | paire différentielle commutée par la C-HSYNC : courant = tanh((dent de scie − delta) / eps), côté source = rho × côté puits |
| Broche 1, filtre | C413 10 nF, R418 8,2 k + C414 1 µF, R417 56 k | valeurs du schéma, sans autre composant |
| Broche 2, oscillateur | C415 5,6 nF chargé par ~14 k depuis 12,5 V, R417 56 k | période = 1024 + hold + g × tension moyenne de la broche 1 ; oscillateur à relaxation : une impulsion pèse exp(−(temps restant avant la fin de rampe) / isf_tau) |

Sensibilité de l'oscillateur calculée sur le schéma : environ 42 px de période par volt sur la broche 1.

Unités : pixel mode 2 = 62,5 ns ; charge = courant puits du comparateur pendant un pixel ; C413 = 1.

## Valeurs tirées des photos (`best_schematic.json`)

Ajustement sur les 44 écrans SHAKER D3 « 2 CSYNC / LINE STUDIES » (#0314 à #033F), moyenne des photos
des CRTC 0, 1 et 2 : **2,75 px RMS**. Fourchette des six meilleures solutions :

| Paramètre | Meilleure | Fourchette | Sens |
|---|---|---|---|
| g | 0,0775 | 0,075 – 0,085 | gain de boucle ; avec 42,5 px/V, courant du comparateur ≈ 0,3 mA |
| eps | 0,124 | 0,11 – 0,13 | zone linéaire du comparateur (amplitude normalisée ±1) |
| delta | 0,065 | 0,060 – 0,066 | référence du comparateur |
| rho | 1,56 | 1,43 – 1,68 | dissymétrie source / puits (la moins bien déterminée) |
| isf_tau | 1970 px | 1670 – 2620 | constante de temps de la rampe (calcul schéma : 1000 à 1250 px, écart non expliqué) |
| isf_u0 | 124 px | 123 – 126 | début de rampe après le début du retour ligne |
| hold | −1,8 px/ligne | −3,5 – +0,4 | réglage H-HOLD de la séance de photos D3, mal déterminé |

Plage de maintien du modèle : environ ±10 px par ligne (±1 %).

## Contrôles

- Module A, #00BD-#00C1, #00C4-#00C7, #00CA, #03AC (références sûres) : dans l'émulateur, même synchro
  partout (2e C-HSYNC de 64 px, 464 px après la principale, bande de 88 lignes). Photos CRTC 0-2 :
  −12 à −15 px, plat (`a_shifts.json`). Modèle (`fit_joint.py`) : −18,8 au Hold de D3, −17,0 à
  −5 px/ligne, −14,8 à −10 px/ligne. Les photos D3 donnent −18 px pour la même synchro : les deux séances
  de photos ne s'accordent pas. L'écart est attribué au Hold, sans preuve (−10 px/ligne est au bord de la
  plage de maintien, peu vraisemblable pour un moniteur réglé).
- Photos CRTC 3 du module A : −23 px. Probablement un moniteur de 6128 Plus, hors étalonnage.
- #00C8 / #00C9 (2e impulsion de 38 / 50 px dans l'émulateur, principale 64) : photos = dérive lente sur
  toute la bande (−3 → −7 et −4 → −16 px) ; modèle = palier (−12,6 et −15,9). Non résolu. La largeur
  réelle de l'impulsion (R3 réécrit pendant la HSYNC) n'est pas vérifiée, et ces écrans ne sont pas dans
  les références sûres.
- #00C2 (deux impulsions en plus) : photo +11 px, modèle +26 (ancien modèle linéaire : +37).

## Défauts connus

- Fin du glissement à gauche pour les impulsions de 64 et 50 px : à +112 px le modèle donne −38 px,
  la photo −51 ; à +304 px, −54 contre −50. Le modèle dépend de la position de l'impulsion, pas la photo.
  Cause : la dérive lente est la différence de deux courants saturés presque égaux (2e impulsion côté
  source, principale côté puits), pondérés par la rampe ; 10 % sur l'un change tout.
- Pistes écartées : pompe de charge idéale sans oscillateur à relaxation (12 px RMS ; 4,6 px avec les
  autres paramètres libres, le palier à droite ne se forme pas) ; résistance interne sur la broche 1
  (3,0 px RMS mais plage de maintien de ±2 px/ligne) ; montée de dent de scie plus longue.
- Variante équivalente (`osc_tau`, `osc_u0` de `simulate()`) : la broche 2 intègre la tension de la
  broche 1 avec un oubli exponentiel. 2,77 px RMS avec un comparateur symétrique (rho ≈ 1), fin de
  glissement meilleure (−43 à −49), mais oubli de 115 à 160 px (le schéma donne ~1000 px) et palier à
  droite qui rampe. Les photos ne départagent pas les deux formes de l'oscillateur.

## Portage (`Monitor.cpp`)

L'oscillateur y est une rampe dont la pente suit la tension de la broche 1 à chaque pixel ; le retour ligne
part à la fin de la rampe. C'est la même loi que `simulate()` (période = 1024 + hold + g × tension moyenne)
sans retard d'une ligne (`lag=1` dans `simulate()` : 3,26 px RMS au lieu de 2,75). `Monitor::SetHorizontalHold`
règle le Hold ; valeur standard = celle des photos D3. Sans C-HSYNC (R3 = 2), l'oscillateur tourne en libre.

## Fichiers

| Fichier | Rôle |
|---|---|
| `afc_model.py` | simulateur : `simulate()` (tous les écrans à la fois, régime périodique de 312 lignes), `observe()` (applique au simulé la correction de ligne de base de la mesure sur photo) |
| `fit.py` | score RMS contre `profiles.json` : `python3 fit.py "$(cat best_schematic.json)" show` |
| `search.py` | tirage aléatoire puis descente par coordonnées, 16 processus : `python3 search.py '<espace JSON>' <tirages>` |
| `pulses.txt` | 2e C-HSYNC de chaque écran D3 (code, début, largeur) ; principale à 771, même largeur ; sondé dans l'émulateur |
| `edges.py`, `profiles.py`, `profiles.json` | décalage ligne à ligne de la bande sur les photos D3 (bordure bleue) |
| `aedge.py`, `ameasure.py`, `a_shifts.json` | décalage de bande sur les photos du module A (bordure jaune) |
| `best_schematic.json` | meilleure solution |
| `fit_joint.py` | score D3 + module A avec un Hold par séance (`hold_a`) ; `AFC_FIT=fit_joint python3 search.py ...` |

Les profils D3 sont décalés de 2 lignes : l'indice 0 est la 3e ligne de la bande (`profiles.py` suppose
un début de bande à 82 lignes du haut du papier, la capture donne 80). `fit.py` le corrige (`i0 = 2`).

Les photos se téléchargent avec `tools/run_shaker.py --fetch-real` (dossier `test-work/shaker_real`).
