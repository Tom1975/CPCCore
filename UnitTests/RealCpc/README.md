# Tests à faire sur un vrai CPC

Questions que ni le compendium, ni le schéma du Gate Array, ni les photos du site SHAKER ne tranchent.
Tout se fait sur **CRTC 1** (la machine disponible). Reporter le résultat dans la colonne « Résultat ».

Schéma cité : `40010-simplified_V03.pdf` (révision 0.3, « Not verified against real 40010 »).

## 1. Programmes de `realcpc.dsk`

Disquette `realcpc.dsk` dans le lecteur, taper `RUN"MENU"`. Le menu affiche le CRTC détecté (lecture du port
`&BF00`, compendium §28.1.9 : R12 lisible sur les CRTC 0, 3 et 4, R4 relu comme R12 sur les CRTC 3 et 4, R31 non
nul sur le CRTC 1) et range les tests par type de CRTC ; la rubrique du CRTC détecté est en rouge (`<== CE CPC`).
On tape la touche du test : l'écran reste affiché **jusqu'à l'appui d'une touche** (le temps de faire la photo),
puis le menu revient, avec le résultat de `T1` ou `T2` sur sa dernière ligne. `Q` : retour au BASIC.

Le test de touche se fait à durée fixe (256 µs, `keyscan` dans `menu.asm`, à la place d'un `DELAY_LINES 4`) :
les écrans gardent le calage au µs près des anciens programmes (vérifié dans Sugarbox : mêmes captures).

Sources : `menu.asm` (menu, détection, clavier, pages de résultats) qui inclut `sync.asm`, `t1in.asm`, `t2vsync.asm`,
`t3mode.asm`, `t4border.asm`, `t5mon.asm`, `p1last.asm`, `p8vsync.asm`, `fdcrqm.asm`. Reconstruire : `./build.sh <chemin
de rasm>`. Les mesures (`T1`, `T2`, `P1`, `P8`, `FDC`) affichent une page de résultats : noter les nombres.
`calib.asm` assemble une variante sans menu (`calib.dsk`, pas sur la disquette) qui répète `P1`, `P8` ou `FDC` : elle
sert à caler les écritures au µs près avec la trace de Sugarbox (positions dans les en-têtes de `p1last.asm` et
`p8vsync.asm`).
Sugarbox les exécute tous : les valeurs ci-dessous sont celles qu'il donne aujourd'hui en CRTC 1.

| Touche | # | Rubrique | Question | Résultat possible | Sugarbox | Résultat |
|---|---|---|---|---|---|---|
| `0` | SYNC | Tous les CRTC | Où tombe la synchronisation des programmes (`HALT` sur l'interruption du Gate Array, `sync.asm`) sur un vrai CPC ? La broche VSYNC est lue sur le PPI à 8 µs consécutives autour de son front, après une synchro par `HALT` puis par une glissade de NOP. | Chaîne `00001111` : comme Sugarbox. Le premier 1 **une place à gauche** : le code tourne une µs plus tôt sur le vrai CPC (ce que suggère P1). Chaînes différentes entre `HALT` et `NOP` : la sortie de `HALT` est en cause. | `00001111` et `00001111` | `00001111` et `00001111` (2026-11-18) : la synchro tombe sur la même µs que dans Sugarbox |
| `1` | T1 | Tous les CRTC | Une lecture du PPI à l'adresse `&74xx` écrit-elle aussi dans le Gate Array ? Le schéma n'a ni RD ni WR dans ce décodage (`U401`). | Bordure **rouge** : oui, le schéma a raison. Bordure **bleue** : non. La page de résultats donne la valeur lue, qui doit valoir 76, sinon le test n'a pas lu la bonne valeur. | Bleue, 76 | **Rouge**, 76 (2026-11-18) : le Gate Array prend la valeur, corrigé (`Sig.cpp`, test `Compendium_4.InOnThePpiAndTheGateArrayWritesTheGateArray`) |
| `2` | T4 | Tous les CRTC | La bordure et l'encre changent-elles de couleur au même endroit (correctif `e2c490f`) ? | Deux bandes rouges superposées. **Bouts droits alignés** : le correctif est bon. **Décalés de 8 pixels mode 2** (la bande du bas s'arrête plus à gauche) : il faut l'annuler. | Alignés | **Alignés** à ~1 pixel près (2026-11-18, `batch_2`) : `e2c490f` confirmé |
| `3` | FDC | Tous les CRTC | RQM retombe-t-il entre deux octets de résultat ? Ni la fiche du µPD765 ni 64nops ne donnent de durée. `READ ID` sur la disquette du lecteur A, puis le registre d'état lu 5, 9, 13, 17 et 21 µs après chaque octet. | `D0` partout : RQM reste à 1. Des `50` : RQM retombe, leur nombre donne la durée (4 µs par échantillon). `80` après le 7e octet : fin du résultat. « Pas de réponse » : pas de disquette lisible en A. | `D0` partout, `80` au 7e | **RQM retombe** (2026-11-18) : `50` à 5 µs, `10` à 9 et 13 µs, `D0` vers 17-21 µs ; au 7e : `50 10 90 80 80`. Sugarbox à corriger |
| `4` | T2 | CRTC 1 | Une VSYNC relancée après un nombre impair de HSYNC garde-t-elle le bit 0 de son compteur (`U809`) ? La page donne les deux mesures (17 puis 18 HSYNC). | **Deux valeurs proches** : non. **La première plus petite d'environ 6** (une ligne) : oui, le schéma a raison. **Lancer 3 ou 4 fois** : Sugarbox donne 340 ou 346 pour la première selon le lancement. | 340 ou 346, puis 346 (346 et 346 avant `101c896`, C3h cadencé par la fin de HSYNC, P8 : à relire avec P8) | 5 lancements (2026-11-18) : 346/340, 334/340, 334/340, 346/340, 346/340 |
| `5` | T3 | CRTC 1 | Un R3.JIT déplace-t-il le changement de mode ? Le schéma dit oui de 4 pixels (`U1003`, `U1004`), le compendium dit non (9.3.4.4). | Photo à comparer à `reference/t3mode_crtc1.png`, zoomée sur le début de chaque rangée, juste après le noir à gauche. Les rangées peuvent être décalées d'un lancement à l'autre : comparer leur aspect, pas leur numéro. | Voir la référence | |
| `6` | P1 | CRTC 1 | Quand la bascule « dernière ligne » échantillonne-t-elle C4=R4 et C9=R9 ? Voir P1 dans la partie 1 bis. **(A)** R0=20 écrit sur C0=20 de la dernière ligne (C4=38, C9=7). **(B)** R4=39 écrit sur cette ligne, I/O en C0=60, 61, 62, 63 (=R0) puis C0=0 de la ligne suivante. Pas de 10 µs jusqu'à la VSYNC suivante. | (A) ≈1535 : la trame se termine ; ≈6100 : C4 déborde. (B) ≈1536 : la trame se termine ; ≈1587 : une rangée de plus. La première position à ≈1536 situe l'échantillonnage. | (A) 1535 ; (B) 1587, 1587, 1587, 1536, 1536 | (A) 1535 ; (B) 1587, 1587, 1587, **1587**, 1536 (2026-11-18). Les photos SHAKER E de la même machine (#03FB, #03FC : R4/R9 sur C0io=#3F sans effet ; #0401, #0402 : YOU'VE WON) confirment Sugarbox ; SYNC montre que la synchro n'est pas décalée. Reste à expliquer : l'instruction d'I/O diffère peut-être (position de l'I/O dans la µs) |
| `7` | P8 | CRTC 1 | Qu'est-ce qui fait avancer le compteur de lignes VSYNC C3h ? Voir P8 dans la partie 1 bis. Pas de 10 µs jusqu'à la fin de la VSYNC. **(A)** R0=0 pendant 40 µs sur C0=0 de la ligne C4=29, C9=7, et référence sans R0=0. **(B)** R0=20, R2=40 (plus de HSYNC) : VSYNC encore active 3 ms après son début ? **(C)** R7=20 écrit sur la ligne C4=20 en C0=#10 puis en C0=#3C. **(D)** comme (A), R0=0 pendant 10, 14 … 38 µs. | (A) ≈102 : C3h figé sans HSYNC ; ≈2000 : la VSYNC s'est finie pendant R0=0 (C3h compte les lignes de 1 µs). (B) OUI : figé sans HSYNC. (C) ≈100 puis ≈96 : le §16.4.2 ; ≈101 puis ≈103 : une ligne de plus après la fin de HSYNC (modèle). (D) ≈100 tant que la VSYNC survit, ≈1850 dès qu'elle finit pendant R0=0 : le seuil donne la vitesse de C3h (≈17 µs : une ligne par µs ; ≈33 µs : une sur deux). | (A) 105 et 102 ; (B) OUI, 100 ; (C) 101, 103 ; (D) 102 partout | (A) 105 et **1849** ; (B) **NON**, 1175 ; (C) 101, **99** (2026-11-18) : C3h avance sans HSYNC, `101c896` à revoir ; (D) **77 64 51 38 / 25 13 1888 1862** : chaque µs de R0=0 retire une demi-ligne à la VSYNC, C3h avance une µs sur deux avec des lignes de 1 µs ; la VSYNC finit entre 30 et 34 µs (16 lignes) |
| `8` à `F` | T5 | CRTC 1 | Boucle de synchro ligne du moniteur (modèle provisoire P2), écrans 1 à 8. Voir le détail ci-dessous. | Une photo par écran, 20 secondes au moins chacun, **même moniteur, sans toucher au H-HOLD**, cadrée comme les photos SHAKER (tout l'écran, de face). | `reference/t5mon_<n>_crtc1.png` | Photos `Menu_8 (2)` à `(10)` puis `(1)` = écrans 1 à 8 (`(5)`, `(6)` : écran 4 ; `(7)` : repère) (2026-11-18) |

Aucun test pour les CRTC 0, 2, 3 et 4 pour l'instant : les rubriques sont prêtes dans le menu.

### T5 : les huit écrans du moniteur

L'écran affiche 25 rangées `000102…27` en mode 2 (un octet = 16 pixels mode 2), papier blanc, bordure bleue. Dans une
bande de lignes, la synchro ligne est modifiée ; le reste de l'écran sert de repère. Ce qu'on lit sur la photo : le
décalage horizontal des rangées de la bande, ligne par ligne, par rapport aux rangées du dessus.

| n | Bande | Ce que l'écran tranche |
|---|---|---|
| 1 | 64 lignes sans HSYNC | L'oscillateur tourne en libre : la pente des rangées donne le réglage du H-HOLD de ce moniteur, et la façon dont il lâche (photos SHAKER D8 : −10 px en 64 lignes, rapide puis lent ; Sugarbox : −9 px, plus régulier). |
| 2 | 200 lignes sans HSYNC | La même chose sur une durée où la tension du filtre a le temps de bouger. |
| 3 | 2e C-HSYNC de 64 px, 464 px après la principale, 64 lignes | La synchro de SHAKER `#00BD` et `#0318`, dont les photos donnent −13 et −18 px : valeur de ce moniteur. |
| 4 | La même, 200 lignes | Le palier tient-il, ou l'image continue-t-elle de glisser ? |
| 5 | 2e C-HSYNC de 32 px, principale de 64 px, 64 lignes | Force de l'impulsion en plus seule (SHAKER D3 change toujours les deux largeurs ensemble). |
| 6 | 2e C-HSYNC de 64 px, principale de 32 px, 64 lignes | Force de rappel de l'impulsion principale seule. |
| 7 | 2e C-HSYNC de 64 px, 112 px après la principale, 200 lignes | Le glissement à gauche s'arrête-t-il (palier) ou décroche-t-il ? Sur 64 lignes les photos D3 ne le disent pas. |
| 8 | Deux C-HSYNC en plus (464 et 640 px après la principale), 64 lignes | La synchro de SHAKER `#00C2` : photo +11 px, Sugarbox +26. |

Les bandes de 64 lignes commencent 100 lignes sous le haut de l'écran, celles de 200 lignes 30 lignes sous le haut.
Dans la bande, la HSYNC principale est raccourcie à 6 caractères (R3 n'est jamais écrit pendant une HSYNC) : la
C-HSYNC garde ses 4 µs, mais le noir à gauche de l'image est plus court, c'est normal.

Ce que change le résultat :

- **T1 oui** : à ajouter à l'émulation (`Sig.cpp`, entrées), un `IN` qui sélectionne le PPI et le Gate Array écrirait dans ce dernier.
- **T2 oui** : à ajouter (`VGA.cpp`, `wait_for_hsync_` et `v26_` repartiraient d'un compteur dont le bit 0 survit).
- **T3 décalé de 4 pixels** : le schéma a raison, les chronogrammes du compendium sont à revoir. Sinon, le verrou transparent actuel est confirmé.
- **T4 décalé** : annuler `e2c490f` et signaler l'écart avec le §9.2.
- **T5** : les huit photos entrent dans l'ajustement du modèle du moniteur (`res/Shaker/afc_study/`) à la place des
  photos SHAKER de séances différentes ; l'écran 1 fixe la valeur standard de `Monitor::SetHorizontalHold`.

## 1 bis. Modèles provisoires à confirmer

Comportements implémentés pour suivre les photos SHAKER, avec un modèle électronique plausible mais pas
démontré. P1 et P8 ont un programme dans le menu ; P9 se lit sur SHAKER (partie 2) ; les autres demandent un
autre CRTC (partie 3) ou le moniteur (P2 : T5 et partie 4).

| # | Question | Modèle actuel de Sugarbox | Mesure proposée (CRTC 1) |
|---|---|---|---|
| P1 | Quand la bascule « dernière ligne » du CRTC 1 échantillonne-t-elle C4=R4 et C9=R9 ? (SHAKER A3 RVMB `#000F` à `#0016`, `#0023` à `#002A`, correctif `fix/shaker-crtc1-r4-zero`) | Sur le **front montant de la sortie du comparateur C0=R0** (`CRTC::SampleLastLine1`). En comptant, c'est le début du caractère C0=R0. Avec R0 écrit égal au C0 en cours, c'est l'instant de l'écriture (`WriteBus`, case 0) : une bascule cadencée par un comparateur et non par l'horloge, inhabituelle. | **Programme : touche `6` du menu (`p1last.asm`).** Sur la dernière ligne d'une trame (C4=R4, C9=R9, R5=0) : **(a)** écrire R0=C0 en cours de ligne et vérifier que la trame se termine (C4 revient à 0, pas de débordement) ; **(b)** écrire R4≠C4 pendant le caractère C0=R0 à chacune des 4 µs de l'`OUT`, pour situer l'instant d'échantillonnage. Si (b) annule la fin de trame quand l'écriture tombe avant la fin du caractère, l'échantillonnage est synchrone (front d'horloge qui termine C0=R0) et le crochet sur R0 est à remplacer. |
| P2 | Boucle AFC du moniteur CTM644 (LA7800) : comparateur, filtre, oscillateur (`Monitor.cpp`, étude `res/Shaker/afc_study/`) | Structure du schéma (dent de scie broche 16, filtre broche 1 aux valeurs du schéma, oscillateur à rampe broche 2). Valeurs **ajustées sur les photos SHAKER D3 et D8** (2,9 px RMS sur D3) : plage linéaire et référence du comparateur, dissymétrie source / puits 1,42, gain de l'oscillateur, mémoire de la rampe 2230 px (le schéma donne 1000 à 1250), H-HOLD +1,10 px/ligne (D8 : sans HSYNC l'image glisse à gauche). Les photos du module A donnent −13 px là où D3 donne −18 px et le modèle −21 : écart non expliqué. | Mesures M1 à M7 de la partie 4, plus : période libre de l'oscillateur (broche 2, sans signal vidéo) pour la valeur du H-HOLD ; rapport des courants source / puits sur la broche 1 (M2 contre M3). |
| P3 | CRTC 2 : que devient un début de ligne (C0=0) rencontré pendant une HSYNC ? Le compendium (15.5) dit que le test C0=0 n'est pas fait, les photos SHAKER D3 `#0316` `#0321` `#032C` `#0337` et D4 `#037F` `#0380` gardent le texte sur ces lignes. | **(a)** Le début de ligne est mémorisé et lève la bordure à la fin de la HSYNC ; C0=R1 l'annule et l'emporte sur le même caractère (`CRTC_2.cpp`). **(b)** La bordure verticale n'est levée que par un vrai début de trame, pas par un C4 revenu à 0 en débordant (`ClockDispEnHalvesCrtc02`) : c'est ce qui laisse en bordure les exemples du 15.6 où la HSYNC couvre C0=0 à chaque ligne. Aucune des deux règles n'est démontrée. | **Sur un CRTC 2** (pas la machine disponible) : R2=62, R3=6 dans une bande de quelques lignes au milieu de l'écran (texte attendu à partir de C0=4 si (a) est juste, bordure sur toute la ligne si le 15.5 est juste) ; puis R2=62, R3=6 sur tout l'écran avec R6 > C4 maximum, pour voir si l'affichage revient quand C4 repasse par 0 en débordant (b). |
| P4 | CRTC 0 : jusqu'où une écriture de R5 compte-t-elle sur une ligne ? Le compendium (11.3.1, 11.4.2) dit « uniquement lorsque C0<3 » ; l'autocontrôle SHAKER E `#03FA` attend que R5=0 écrit sur C0=3 et C0=#3D à #3F de la dernière ligne termine la trame. | La bascule « fin d'ajustement » (C9 suivant = R5) suit le comparateur jusqu'à C0=2, puis peut encore s'armer mais plus se désarmer (`CRTC_0.cpp`, `adjust_end_`). La partie « s'arme encore » n'est démontrée que pour R5=0 sur la dernière ligne. | **Sur un CRTC 0** : pendant un ajustement (R5=20), sur une ligne additionnelle C9=5, écrire R5=6 sur C0=10 : la trame se termine-t-elle à la fin de cette ligne (modèle actuel) ou le compteur déborde-t-il ? |
| P5 | CRTC 2 : que fait VMA' quand C0==R1 ne devient vrai qu'après l'écriture de R1 pendant le caractère (R1 écrit égal à C0) ? Le compendium (17.4.3) décrit seulement le cas inverse (R1 changé pendant C0==ancien R1 : chargé quand même) et le chargement de R12/R13 par un ET puis un OU. | Seule l'impulsion OU est faite, en fin de caractère (`CRTC_2.cpp`, `r1_load_at_start_`) : VMA' \|= VMA. Déduit de la photo SHAKER R1 STORIES `#009A` (rangées identiques, loin du texte) ; un chargement complet donne des rangées différentes, le ET seul répète la ligne de titre. | **Sur un CRTC 2** : R12/R13=&0101, R1=70 (>R0) ; sur la ligne C4=0, C9=R9, écrire R1=1 pendant C0=1 puis R1=70. Adresse de la rangée C4=1 : &0103 (OU seul, modèle), &0102 (chargement complet), &0101 (ET seul ou rien). |
| P6 | CRTC 3/4 : Status 1 bit 5 (21.3.4.1). Le compendium décrit R3h>0 (0 sur la ligne R3h) et R3h=0 (1 sur 15 lignes). | Un seul compteur de lignes VSYNC C3h (4 bits) : chargé à 1 par le début de VSYNC, il compte tant qu'il ne vaut pas 0, ou tant que la VSYNC dure sans que C3h==R3h la termine ; il continue donc après la VSYNC jusqu'à 0, où il s'arrête. Le comparateur C3h==R3h termine la VSYNC (début de la ligne suivante) et sa sortie inversée est le bit 5 (`CRTC_3_4.cpp`, `c3h_status_`). Hypothèse : c'est ce comptage après la VSYNC qui n'est pas démontré. Déduit des cartes SHAKER D3 `#03C8`, `#03C9`, `#03D2` et de la page FIESTA `#03C0` (« STATUS 1 BIT 5 : OK »). La recherche de R3h de SHAKER (`SHAKE27D #82B0` : une lecture par ligne depuis la VSYNC) trouve #9D et #7C comme les photos `#03BD`, `#03BE`. | **Sur un CPC+** : R3=#8E, lire STATUS 1 sur C0=10 de chaque ligne de C4=R7-1 à 20 lignes après : le bit 5 ne doit être à 0 que sur la 8e ligne de VSYNC. Même chose avec R3=#0E (0 avant la VSYNC, 1 sur les 15 premières lignes, 0 ensuite). Enfin R3=#8E, puis R3=#2E écrit sur la 5e ligne de VSYNC : la VSYNC dure-t-elle 16+2 lignes (C3h tourne jusqu'à 2) et le bit 5 suit-il ? |
| P7 | CRTC 3/4 : Status 2 bit 4, « toujours 1 » selon le 21.3.4.2. Le code de SHAKER (STATUS FIESTA, `SHAKE27D #9736`, recherche de R5 `#835A`) attend 0 sur toute la trame et 1 sur le dernier caractère des lignes additionnelles quand R5≠0. | Sortie du comparateur de grandeur « C5 suivant ≥ R5 » qui termine les lignes additionnelles (11.2.6, comme C9≥R9 du 10.3.4) : C5 est C9 pendant les lignes additionnelles, son chemin donne 0 en dehors ; « suivant » = sortie de l'incrémenteur, +1 sur C0=R0 (`CRTC.cpp`, `ReadRegister` case 3). Sans R5 : 1 partout. La ligne d'entrelacement (19.6.4) n'a aucun terme : en IVM le comportement n'est pas connu (D3 `#03E1`, en doute). | **Sur un CPC+** : R4=37, R5=8, lire STATUS 2 sur C0=10 et C0=R0 de chaque ligne additionnelle (bit 4 à 1 seulement sur C0=R0 de la 8e) ; puis écrire R5=3 sur la 4e ligne additionnelle : le bit passe-t-il à 1 aussitôt (comparateur de grandeur) ? En IVM (R8=3), R5=0 : le bit 4 reste-t-il à 1 sur les trames paires ? |
| P8 | CRTC 1 : qu'est-ce qui fait avancer le compteur de lignes VSYNC C3h ? Le compendium (13.2.6) dit que R0=0 ne bloque ni C9 ni C4 sur CRTC 1, et ne dit rien de C3h. SHAKER LOST VSYNC ON R0=0 (`#008D`, `SHAKE27A #6491`) : R0=0 écrit sur C0=0 de la ligne C4=29, C9=7 (R7=30, R2=46), puis R0=45 et R2=28 18 µs plus tard ; la photo lit la VSYNC aux quatre lectures (5, 23, 61 et 65 µs après l'écriture). SHAKER R52INC IN HSYNC (`#01F4`, `SHAKE27B #502A`) : R0=0, R2=0, R3=1 (HSYNC infinie, 15.3.2) ; aucune interruption en 3328 µs (#FF), alors que R52 compte les fins « invisibles » de HSYNC toutes les 16 µs (15.3.4) : la VSYNC doit revenir à chaque trame pour le remettre à 0. | C3h est cadencé par la **fin de la HSYNC interne du CRTC** (front descendant, fin par R3.JIT, fin invisible du 15.3.4, et HSYNC de longueur nulle sur C0=R2 quand R3l=0 : INTER TEST IM1 `#01F2`, R3=0, photographié, ne se bloque pas) ; C3h==R3h arme un latch qui termine la VSYNC au début de ligne suivant (`CRTC_1.cpp`, `hsync_prev_`, `vsync_end_pending_`). Sans HSYNC (R0<R2) C3h est figé. Écartée par `#01F4` : un C3h cadencé par le front de C0==R0 (figé aussi avec la HSYNC infinie, R52 atteint 52). Conséquence non vérifiée : une VSYNC déclenchée par R7 après la fin de HSYNC de la ligne dure une ligne de plus que ne le dit le 16.4.2. | **Programme : touche `7` du menu (`p8vsync.asm`).** **Sur un CRTC 1** : (a) R0=0 sur C0=0 de la ligne C4=R7-1, C9=R9 (R2=46), attendre 40 µs, R0=63 : la VSYNC dure-t-elle 40 µs + 16 lignes ? (b) R0=20, R2=40 (C0 n'atteint jamais R2) pendant une trame : la VSYNC reste-t-elle active (modèle) ? (c) R7=C4 écrit sur C0=#3C (après la fin de HSYNC) : la VSYNC dure-t-elle 16 lignes moins C0+1 (16.4.2) ou 17 lignes moins C0+1 (modèle) ? |
| P9 | CRTC 1 : qu'est-ce qui remet à 0 l'état « VMA rechargé depuis R12/R13 » quand un R.F.D. le met à 1 sur le même front ? Le 11.6.1 dit qu'une RFD par R5 sur la ligne C9=R9 l'annule (cas 2) ; le 13.7.1.2 dit qu'une RFD par allongement de R0 avec R9 écarté de C9 prend R12/R13, et avec R4 seul une trame sur deux. | Bascule à remise à 0 prioritaire : remise à 0 = latch « C0=R1 vu avec C9=R9 » (ou R1>R0) ET test C9=R9 du front de fin de ligne, avec ParitéC9 dès qu'un R.F.D. est posé (`CRTC_1.cpp`, `c9_eq_r9_parity`). SHAKER D RFD ROUND 2 (`SHAKE27D #4F86`) : « OVERRIDE BY R9 » et « R9+R4 » sans IVM (`#0393`, `#0394`, `#03F7`, `#03F8`) alternent comme les photos. **Non résolu** : « OVERRIDE BY R4 » (`#0395`, `#03F9`) et les variantes IVM ON/OFF (`#0396` à `#039B`). Une ParitéC9 sortie d'une porte XOR (parité de base xor (C4.0 et non R9.0), formules du 19.5.3) règle `#0396`, `#0397` mais casse C `#02D8` (CRTC 1, photo conforme avec la bascule) : non retenue. | **Pas de programme : vidéo de quelques secondes des pages SHAKER D « RFD ROUND 2 » (partie 2), l'alternance d'une trame sur deux se voit au clignotement.** **Sur un CRTC 1** : R9=7, R4=7 ; sur C0=R0 de la dernière ligne (C4=7, C9=7), R0=#BD puis R0=#7F, puis R9=14 sur C0=#66 ; IVM ON/OFF (R8=3 puis 0) sur C4=0 avant. R12/R13 différents de l'adresse courante : les rangées sous C4=8 répètent-elles R12/R13 sur toutes les trames (porte XOR) ou une sur deux (bascule) ? |

## 2. Photos SHAKER à refaire

Avec `shaker27.dsk`, `RUN"SHAKE27x"`, puis la touche de menu, puis la barre d'espace le nombre de fois indiqué.
La touche « Z » de SHAKER (base R9) est le **W** d'un clavier AZERTY. Les chemins viennent de `res/Shaker/CSL/MODULE_x/SHAKER-x-1.CSL`.

| Module | Touche, espaces | Codes | Pourquoi |
|---|---|---|---|
| A | `2` | `#03A5`, `#03A6` | Références acceptées avec un doute, une colonne étroite semble de la mauvaise couleur |
| A | `P`, 10 | `#039C` | Pas de photo sur le site |
| B | `1`, page par page | `#00D5` à `#00DF` | R4 et R5 de l'AUTOSYNC. Le correctif `21e025a` suit les photos contre le texte du compendium (11.3.2) |
| C | `S` | `#02D3` | Même correctif, garde-fou |
| B | `CTRL` | `#01DA`, `#01DB`, `#01DC` | Photos en C9=1, Sugarbox en C9=0 au même code |
| B | `I`, 5 | `#01E3` | Texte coloré dans Sugarbox, normal sur la photo |
| B | `COPY`, 2 | `#01D6` | Zone du haut : texte dans Sugarbox, hexa sur la photo |
| B | `5` | `#019B` | Page en transition, émulateurs partagés |
| D | `I`, 2 | `#02E5` | `#0759` ou `#0758` : la valeur semble dépendre de la machine. Noter plusieurs relevés |
| D | `9`, puis 0 à 8 | `#0393` à `#039B` (RFD ROUND 2) | **Vidéo** de 5 secondes par page plutôt qu'une photo : les rangées sous la rupture changent-elles une trame sur deux (P9, partie 1 bis) ? Sugarbox se trompe sur `#0395` à `#039B` |

## 3. Non mesurable sur cette machine

Il faudrait un autre CRTC. À garder si l'occasion se présente.

| CRTC | Codes | Écart |
|---|---|---|
| 0 | `#021B` (module B) | `R6=0` puis `R6=13` en zone `R0=0` : rayures dans Sugarbox, BORDER sur la photo |
| 0 | `#0006` (module A) | Référence acceptée avec un doute |
| 0 | `#00C2` (module A) | Moniteur : +37 contre +11 pixels. Cause : le comparateur du LA7800, voir la partie 4 |
| 2 | `#0316`, `#0321`, `#032C`, `#0337`, `#037F`, `#0380` (module D) contre `#000B` à `#001E` (module A, A3 RVMB) | BORDER à la place du texte dans Sugarbox (règle du §15.5 : pas de levée du BORDER sur C0=0 pendant une HSYNC). Les photos CRTC 2 se contredisent : D3 lève le BORDER sous une HSYNC finie sur C0=1, A3 ne le lève pas (marge jaune). Aucun modèle essayé n'explique les deux. Mesure : sur un CRTC 2, RVMB et D3 avec R1 avant ou pendant la HSYNC. Test `Compendium_15.DISABLED_Crtc2HSyncOnC0ZeroInTheMiddleOfTheScreenKeepsTheDisplay` |
| 2 | `#02BF` (module C, touche `7`) | LAST LINE COND. Photo prise avec une autre version de SHAKER (3 lignes de texte différentes). Elle donne 01 sur toutes les positions de R9=1 écrit sur C4=1, C9=1 (C0io #2C à #3F), HSYNC comprise (R2=#2E, R3=#0A dans `SHAKE27C #840E`) ; Sugarbox suit le §15.6 (« une mise à jour de R4 ou R9 est ignorée si elle a lieu durant une HSYNC ») et rend FF sous la HSYNC. Mesure : sur un CRTC 2 avec SHAKER 2.7, refaire la photo ; si 01 partout, le §15.6 est faux pour R9 |
| 0 | `#0180`, `#0181`, `#0184` à `#0187` (module B, base 5, parité paire) | INTERLACE C4/C9 COUNTERS. `SHAKE27B #7607` écrit R9=5 (C0=#14) puis R8=3 (C0=#22) sur la ligne C4=6, C9=x et ressort de l'IVM à instant fixe. AUTOSYNC pour x=0..7, photo : `2B00 2B00 2602 2602 2A02 2602 2602 2602`, Sugarbox : `2B00 2B00 2B00 2602 2602 2A02 2602 2602`. La photo correspond à notre comptage décalé d'une ligne : la ligne qui suit l'écriture compare C9 à R9, le régime doublé ne commence qu'après, et la ligne d'entrée ne termine pas le caractère même avec C9=R9=5 (le §19.8.1 dit l'inverse). La parité impaire (`#0127` à `#013A`) et les séries R9=6 sont justes avec le même modèle, et aucun mécanisme essayé (ParitéC9 du caractère d'entrée, cible d'entrée R9 avec bit 0 = ParitéFrame) n'explique les deux parités. Photos prises avec SHAKER 2.2. Mesure : sur un CRTC 0 avec SHAKER 2.7, base 5, parité paire, refaire les photos des pages D1, F1 et G1 |
| 3, 4 | `#03BE`, `#03C0`, `#03C8` et suivants (module D) | Registres et statuts. Le schéma du 40010 ne couvre pas ces ASIC |

`RESET` qui force la bordure à `#10` (`U409`) n'est pas observable sur un 6128 standard : le firmware la réécrit aussitôt.

## 4. Moniteur CTM644 : mesures à l'oscilloscope

Le moniteur décale l'image quand une ligne porte une C-HSYNC en plus (R2 réécrit pendant la HSYNC). `Monitor.cpp` suit
un modèle reconstruit du schéma et ajusté sur les photos SHAKER D3 « 2 CSYNC / LINE STUDIES » (44 écrans, CRTC 0, 1 et 2
concordants, 2,9 px RMS) et D8 « NO HSYNC » : modèle provisoire, entrée P2 de la partie 1 bis. La fiche Sanyo du LA7800 ne donne ni le
courant de l'AFC, ni sa plage linéaire, ni la commande de l'oscillateur : les mesures ci-dessous les remplaceraient.
Étude, mesures sur photo et simulateur : `res/Shaker/afc_study/`.

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
A7 +10, A8 +26, D1 -13, D8 +9. Sugarbox : tests `Monitor.*` (`TestMonitor.cpp`).
