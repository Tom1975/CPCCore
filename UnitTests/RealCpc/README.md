# Tests à faire sur un vrai CPC

Questions que ni le compendium, ni le schéma du Gate Array, ni les photos du site SHAKER ne tranchent.
Tout se fait sur **CRTC 1** (la machine disponible). Reporter le résultat dans la colonne « Résultat ».

Schéma cité : `40010-simplified_V03.pdf` (révision 0.3, « Not verified against real 40010 »).

## 1. Programmes de `realcpc.dsk`

Sur le prompt BASIC, avec la disquette `realcpc.dsk` dans le lecteur, taper la ligne indiquée. Chaque programme
rend la main au BASIC tout seul. `T3` et `T4` durent 15 secondes : c'est le moment de la photo.

Sources : `t1in.asm`, `t2vsync.asm`, `t3mode.asm`, `t4border.asm`. Reconstruire : `./build.sh <chemin de rasm>`.
Sugarbox les exécute tous : les valeurs ci-dessous sont celles qu'il donne aujourd'hui en CRTC 1.

| # | Ligne à taper | Question | Résultat possible | Sugarbox | Résultat |
|---|---|---|---|---|---|
| T1 | `MEMORY &3FFF:LOAD"T1IN.BIN":CALL &4000:PRINT PEEK(&4F00)` | Une lecture du PPI à l'adresse `&74xx` écrit-elle aussi dans le Gate Array ? Le schéma n'a ni RD ni WR dans ce décodage (`U401`). | Bordure **rouge** 3 s : oui, le schéma a raison. Bordure **bleue** : non. Le nombre affiché doit valoir 76, sinon le test n'a pas lu la bonne valeur. | Bleue, 76 | |
| T2 | `MEMORY &3FFF:LOAD"T2VSYNC.BIN":CALL &4000:PRINT PEEK(&4F00)+256*PEEK(&4F01),PEEK(&4F02)+256*PEEK(&4F03)` | Une VSYNC relancée après un nombre impair de HSYNC garde-t-elle le bit 0 de son compteur (`U809`) ? | **Deux valeurs proches** : non. **La première plus petite d'environ 6** (une ligne) : oui, le schéma a raison. | 346 et 346 | |
| T3 | `MEMORY &3FFF:LOAD"T3MODE.BIN":CALL &4000` | Un R3.JIT déplace-t-il le changement de mode ? Le schéma dit oui de 4 pixels (`U1003`, `U1004`), le compendium dit non (9.3.4.4). | Photo à comparer à `reference/t3mode_crtc1.png`, zoomée sur le début de chaque rangée, juste après le noir à gauche. Les rangées peuvent être décalées d'un lancement à l'autre : comparer leur aspect, pas leur numéro. | Voir la référence | |
| T4 | `MEMORY &3FFF:LOAD"T4BORDER.BIN":CALL &4000` | La bordure et l'encre changent-elles de couleur au même endroit (correctif `e2c490f`) ? | Deux bandes rouges superposées. **Bouts droits alignés** : le correctif est bon. **Décalés de 8 pixels mode 2** (la bande du bas s'arrête plus à gauche) : il faut l'annuler. | Alignés | |

Ce que change le résultat :

- **T1 oui** : à ajouter à l'émulation (`Sig.cpp`, entrées), un `IN` qui sélectionne le PPI et le Gate Array écrirait dans ce dernier.
- **T2 oui** : à ajouter (`VGA.cpp`, `wait_for_hsync_` et `v26_` repartiraient d'un compteur dont le bit 0 survit).
- **T3 décalé de 4 pixels** : le schéma a raison, les chronogrammes du compendium sont à revoir. Sinon, le verrou transparent actuel est confirmé.
- **T4 décalé** : annuler `e2c490f` et signaler l'écart avec le §9.2.

## 1 bis. Modèles provisoires à confirmer

Comportements implémentés pour suivre les photos SHAKER, avec un modèle électronique plausible mais pas
démontré. Pas encore de programme : à écrire si l'occasion se présente (mêmes conventions que `T1` à `T4`).

| # | Question | Modèle actuel de Sugarbox | Mesure proposée (CRTC 1) |
|---|---|---|---|
| P1 | Quand la bascule « dernière ligne » du CRTC 1 échantillonne-t-elle C4=R4 et C9=R9 ? (SHAKER A3 RVMB `#000F` à `#0016`, `#0023` à `#002A`, correctif `fix/shaker-crtc1-r4-zero`) | Sur le **front montant de la sortie du comparateur C0=R0** (`CRTC::SampleLastLine1`). En comptant, c'est le début du caractère C0=R0. Avec R0 écrit égal au C0 en cours, c'est l'instant de l'écriture (`WriteBus`, case 0) : une bascule cadencée par un comparateur et non par l'horloge, inhabituelle. | Sur la dernière ligne d'une trame (C4=R4, C9=R9, R5=0) : **(a)** écrire R0=C0 en cours de ligne et vérifier que la trame se termine (C4 revient à 0, pas de débordement) ; **(b)** écrire R4≠C4 pendant le caractère C0=R0 à chacune des 4 µs de l'`OUT`, pour situer l'instant d'échantillonnage. Si (b) annule la fin de trame quand l'écriture tombe avant la fin du caractère, l'échantillonnage est synchrone (front d'horloge qui termine C0=R0) et le crochet sur R0 est à remplacer. |

## 2. Photos SHAKER à refaire

Avec `shaker27.dsk`, `RUN"SHAKE27x"`, puis la touche de menu, puis la barre d'espace le nombre de fois indiqué.
La touche « Z » de SHAKER (base R9) est le **W** d'un clavier AZERTY. Les chemins viennent de `res/Shaker/CSL/MODULE_x/SHAKER-x-1.CSL`.

| Module | Touche, espaces | Codes | Pourquoi |
|---|---|---|---|
| A | `2` | `#03A5`, `#03A6` | Références acceptées avec un doute, une colonne étroite semble de la mauvaise couleur |
| A | `P`, 10 | `#039C` | Pas de photo sur le site |
| B | `1`, page par page | `#00D5` à `#00DF` (base 7), `#00EE` à `#00F4` (base 6 : `Z` puis `1`) | R4 et R5 de l'AUTOSYNC. Le correctif `21e025a` suit les photos contre le texte du compendium (11.3.2) |
| C | `S` | `#02D3` | Même correctif, garde-fou |
| B | `P` | `#01E8` à `#01EF` | R4 SYNC illisible sur les photos : `#2A` ou `#2B` |
| B | `CTRL` | `#01DA`, `#01DB`, `#01DC` | Photos en C9=1, Sugarbox en C9=0 au même code |
| B | `I`, 5 | `#01E3` | Texte coloré dans Sugarbox, normal sur la photo |
| B | `COPY`, 2 | `#01D6` | Zone du haut : texte dans Sugarbox, hexa sur la photo |
| B | `5` | `#019B` | Page en transition, émulateurs partagés |
| D | `I`, 2 | `#02E5` | `#0759` ou `#0758` : la valeur semble dépendre de la machine. Noter plusieurs relevés |

## 3. Non mesurable sur cette machine

Il faudrait un autre CRTC. À garder si l'occasion se présente.

| CRTC | Codes | Écart |
|---|---|---|
| 0 | `#021B` (module B) | `R6=0` puis `R6=13` en zone `R0=0` : rayures dans Sugarbox, BORDER sur la photo |
| 0 | `#01CB` à `#01CF` (module B) | Décrochage dans Sugarbox, photo stable |
| 0 | `#012D` à `#0137` (module B) | R5 `#07` sur la photo, `#05` dans Sugarbox, avec une photo prise en base 5 et un script en base 7 |
| 0 | `#0006` (module A) | Référence acceptée avec un doute |
| 0 | `#00C2` (module A) | Moniteur : +37 contre +11 pixels. Cause : le comparateur du LA7800, voir la partie 4 |
| 2 | `#0316`, `#0321`, `#032C`, `#0337`, `#037F`, `#0380` (module D) contre `#000B` à `#001E` (module A, A3 RVMB) | BORDER à la place du texte dans Sugarbox (règle du §15.5 : pas de levée du BORDER sur C0=0 pendant une HSYNC). Les photos CRTC 2 se contredisent : D3 lève le BORDER sous une HSYNC finie sur C0=1, A3 ne le lève pas (marge jaune). Aucun modèle essayé n'explique les deux. Mesure : sur un CRTC 2, RVMB et D3 avec R1 avant ou pendant la HSYNC. Test `Compendium_15.DISABLED_Crtc2HSyncOnC0ZeroInTheMiddleOfTheScreenKeepsTheDisplay` |
| 3, 4 | `#03BE`, `#03C0`, `#03C8` et suivants (module D) | Registres et statuts. Le schéma du 40010 ne couvre pas ces ASIC |

`RESET` qui force la bordure à `#10` (`U409`) n'est pas observable sur un 6128 standard : le firmware la réécrit aussitôt.

## 4. Moniteur CTM644 : mesures à l'oscilloscope

Le moniteur décale l'image quand une ligne porte une C-HSYNC en plus (R2 réécrit pendant la HSYNC). Les photos SHAKER
D3 « 2 CSYNC RELATIVE » (44 écrans, CRTC 0, 1 et 2 concordants) montrent un comparateur de phase en tout-ou-rien, avec deux
constantes de temps. Le modèle linéaire de `Monitor.cpp` se trompe d'un facteur 2 à 7. La fiche Sanyo du LA7800
(`res/Shaker/afc_study/LA7800_Sanyo_ENN725F.pdf`) ne donne ni le courant de l'AFC, ni sa plage linéaire, ni la commande de
l'oscillateur : il faut les mesurer. Étude, mesures sur photo et simulateurs : `res/Shaker/afc_study/` (non suivi).

Schéma : manuel de service CPC6128 / CTM644 / GT65, page 26 (IC401 = LA7800). Points de mesure :

- **broche 16** (entrée AFC) : dent de scie de comparaison. R406 6,2 kΩ et C403 180 pF depuis l'impulsion de retour, C404 0,039 µF à la masse ;
- **broche 1** (sortie AFC) : tension d'erreur. C413 0,01 µF ∥ (R418 8,2 kΩ + C414 1 µF), R417 56 kΩ vers la broche 2 ;
- **broche 14** (entrée synchro) ou sortie C-SYNC du CPC : déclenchement.

Programme : `shaker27.dsk`, `RUN"SHAKE27D"`, touche `3` (2 CSYNC RELATIVE), puis la barre d'espace pour passer d'un écran à
l'autre (A1 à A11, B1 à B11, C1 à C11, D1 à D11). L'en-tête de l'écran affiche R2 et R3 de la HSYNC supplémentaire.

| # | Écran | Mesure | Ce qu'elle tranche |
|---|---|---|---|
| M1 | Un écran sans bande (le menu) | Broche 16 sur une ligne : forme, amplitude crête à crête, créneau pendant le retour ; broche 1 : niveau continu | Forme réelle de la dent de scie (triangle, demi-sinusoïde, part du créneau de C403 ≈ 0,23 calculé) |
| M2 | A1 (`R2=#35`, bande à gauche) | Broche 1 au passage du haut de l'écran à la bande, sur ~10 lignes, synchro sur la VSYNC + retard | Saut de tension à chaque impulsion supplémentaire : **taille constante** quelle que soit la position (tout-ou-rien) ou **proportionnelle** à la dent de scie |
| M3 | A8 (`R2=#26`, bande à droite) | Même relevé | Signe opposé, même taille ? |
| M4 | A1 puis D1 (`R3=#06` puis `#03`) | Même relevé | Charge ∝ largeur de l'impulsion (64 puis 18 pixels) |
| M5 | A5, A6, A7 (`R2=#22`, `#23`, `#24`) | Broche 16 pendant l'impulsion supplémentaire, et broche 1 | Plage linéaire du comparateur : la bande passe de -17 à +11 pixels sur ces 3 écrans |
| M6 | A1 | Broche 1 sur toute la bande puis à sa sortie (~70 lignes) | Les deux constantes de temps : saut en 3-4 lignes puis glissement lent sur la photo ; retour en 3-5 lignes à la fin |
| M7 | A1 | Broche 2 (oscillateur, C415 5 600 pF) en même temps que la broche 1 | Comment la tension de la broche 1 agit sur l'oscillateur (fréquence seule, ou phase aussi) |

Décalages mesurés sur les photos SHAKER (pixels mode 2, + = vers la droite, fin de bande) : A1 -45, A5 -17, A6 -7,
A7 +10, A8 +26, D1 -13, D8 +9. Sugarbox : voir `Monitor.DISABLED_R2UpdatedTwiceDuringHsyncMovesTheBandRightAsOnTheRealCpc`.
