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
| 0 | `#00C2` (module A) | Moniteur : deuxième C-HSYNC, +37 contre +11 pixels |
| 2 | `#0316`, `#0321`, `#032C`, `#0337`, `#037F`, `#0380` (module D) | BORDER à la place du texte. La photo suffit, à garder pour vérifier un correctif |
| 3, 4 | `#03BE`, `#03C0`, `#03C8` et suivants (module D) | Registres et statuts. Le schéma du 40010 ne couvre pas ces ASIC |

`RESET` qui force la bordure à `#10` (`U409`) n'est pas observable sur un 6128 standard : le firmware la réécrit aussitôt.
