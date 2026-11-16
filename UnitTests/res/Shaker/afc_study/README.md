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

Ajustement commun (`fit_d8.py`) sur les 44 écrans SHAKER D3 « 2 CSYNC / LINE STUDIES » (#0314 à #033F,
moyenne des photos des CRTC 0, 1 et 2) et sur D8 « NO HSYNC FOR XX LINES » (#0392, 64 lignes sans
C-HSYNC, même module) : **2,89 px RMS sur D3, 2,7 px sur D8**.

| Paramètre | Valeur | Six meilleures solutions | Sens |
|---|---|---|---|
| g | 0,088 | 0,083 – 0,089 | gain de boucle ; avec 42,5 px/V, courant du comparateur ≈ 0,3 mA |
| eps | 0,105 | 0,09 – 0,135 | zone linéaire du comparateur (amplitude normalisée ±1) |
| delta | 0,067 | 0,062 – 0,074 | référence du comparateur |
| rho | 1,42 | 1,24 – 1,67 | dissymétrie source / puits (la moins bien déterminée) |
| isf_tau | 2230 px | 1500 – 3280 | constante de temps de la rampe (calcul schéma : 1000 à 1250 px, écart non expliqué) |
| isf_u0 | 126 px | 126 – 132 | début de rampe après le début du retour ligne |
| hold | +1,10 px/ligne | 1,10 – 1,57 | réglage H-HOLD de la séance de photos du module D |

Plage de maintien du modèle : environ ±10 px par ligne (±1 %).

### Le Hold vient de D8

Sans C-HSYNC le comparateur ne débite plus : la broche 1 retombe sur le pont R418 / R417 (elle perd
R418 / (R417 + R418) = 13 % de son écart au repos en une ligne), puis C414 se vide lentement. La période
passe aussitôt à 1024 + 0,13 × hold. Photos D8 (décalage des rangées de texte, corrélation ligne à ligne,
CRTC 0 et 2 concordants) : l'image glisse à **gauche** de 10 à 11 px en 64 lignes, vite au début (−0,3 px
par ligne) puis lentement. Le signe impose hold > 0 ; D3 seul le laissait entre −3,5 et +0,4 (ancienne
solution : −1,77, 2,75 px RMS sur D3, mais +12 px à droite sur D8). Le modèle glisse plus régulièrement
que la photo (−7,7 px à la ligne 50 pour −9,8).

Sur ces photos le haut de l'écran n'est pas à l'équilibre : loin de toute bande, les rangées dérivent
encore de +4,6 px sur une cinquantaine de lignes (#038D, bande de 8 lignes). Reprise après la synchro
verticale, probablement : le modèle ne la représente pas, et les profils D3 y sont superposés.

## Contrôles

- Module A, #00BD-#00C1, #00C4-#00C7, #00CA, #03AC (références sûres) : dans l'émulateur, même synchro
  partout (2e C-HSYNC de 64 px, 464 px après la principale, bande de 88 lignes). Photos CRTC 0-2 :
  −12 à −15 px, plat (`a_shifts.json`). Modèle (`fit_joint.py`) : −21 px au Hold du module D. Les photos
  D3 donnent −18 px pour la même synchro : les deux séances de photos ne s'accordent pas. Un Hold de
  −10 px/ligne rapprocherait le modèle du module A, mais c'est le bord de la plage de maintien : peu
  vraisemblable. Écart non expliqué ; l'écran 3 de `RealCpc/t5mon.asm` donnera la valeur d'un moniteur connu.
- Photos CRTC 3 du module A : −23 px. Probablement un moniteur de 6128 Plus, hors étalonnage.
- #00C8 / #00C9 (2e impulsion de 38 / 50 px dans l'émulateur, principale 64) : photos = dérive lente sur
  toute la bande (−3 → −7 et −4 → −16 px) ; modèle = palier (−13,6 et −17,4). Non résolu. La largeur
  réelle de l'impulsion (R3 réécrit pendant la HSYNC) n'est pas vérifiée, et ces écrans ne sont pas dans
  les références sûres.
- #00C2 (deux impulsions en plus) : photo +11 px, modèle trop à droite (voir `TestMonitor.cpp`).
- D4 #037F/#0380 (8 C-HSYNC de 64 px par ligne, tous les 128 px, 64 lignes ; sondé dans l'émulateur) :
  photo = la bande part à droite d'environ +7 px (±3, lecture grossière du bord des blocs noirs, déformation
  de la photo non corrigée) et se pose en une quinzaine de lignes ; modèle = +4,4 px en une quinzaine de
  lignes. Même sens, même durée ; pas dans l'ajustement faute d'une mesure propre.
- Sondés mais non exploités : D1 #0310/#0311 (2 C-HSYNC de 34 px à 771 et 851 ; l'émulateur les met sur
  toute la trame, la photo A montre une image inclinée : l'état photographié n'est pas identifié), D7
  (C-HSYNC principale une ligne sur deux à 771 et 835).

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
sans retard d'une ligne (`lag=1` dans `simulate()` coûte 0,5 px RMS). `Monitor::SetHorizontalHold`
règle le Hold ; valeur standard = celle des photos du module D (+1,10). Sans C-HSYNC (R3 = 2), l'oscillateur tourne en libre.

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
| `fit_d8.py` | score D3 + D8 (celui de `best_schematic.json`) : `AFC_FIT=fit_d8 python3 search.py ...` |
| `fit_joint.py` | score D3 + module A avec un Hold par séance (`hold_a`) ; `AFC_FIT=fit_joint python3 search.py ...` |

Les profils D3 sont décalés de 2 lignes : l'indice 0 est la 3e ligne de la bande (`profiles.py` suppose
un début de bande à 82 lignes du haut du papier, la capture donne 80). `fit.py` le corrige (`i0 = 2`).

Les photos se téléchargent avec `tools/run_shaker.py --fetch-real` (dossier `test-work/shaker_real`).
