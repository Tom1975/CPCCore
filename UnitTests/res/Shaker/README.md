# SHAKER

SHAKER (Longshot / Logon System, https://shaker.logonsystem.eu) teste le CRTC et le Gate Array.

| Fichier | Contenu |
|---|---|
| `shaker27.dsk` | SHAKER 2.7, modules SHAKE27A à SHAKE27E (livraison CSLSSM d'octobre 2026, module E révisé) |
| `shaker24.dsk`, `shaker_addon.dsk` | anciennes versions |
| `CSL/MODULE_x/SHAKER-x-n.CSL` | scripts CSL officiels (livraison CSLSSM, synchronisés par `wait_ssm`) : module x, CRTC n |
| `CSL/*.pdf` | normes CSL 1.5 et SSM 1.2 |
| `reference/<module>/<crtc>/*.png` | captures validées à la main d'après les photos sur machine réelle du site SHAKER |

## État des lieux, pas une suite à passer

SHAKER mesure où en est l'émulation : chaque capture SSM (`SUGARBOX_<crtc>_<HHLL>.png`) est
- **OK** : identique au pixel près à sa référence validée ;
- **KO** : différente de sa référence validée, ou plus produite (régression) ;
- **à valider** : pas encore de référence.

Une référence n'est ajoutée qu'après comparaison avec la photo sur machine réelle du même code SSM
(site SHAKER, https://shaker.logonsystem.eu/tests, colonne *Real Hardware*). Seuls les KO font
échouer le test ; un script qui s'interrompt est signalé dans le rapport.

## Lancer la suite

Les 25 scripts sont les tests `Shaker/ShakerSuite.MatchesReference/<module>_<crtc>` de `unitTests`.
Ils sont ignorés sans `CPCCORE_SHAKER=1` (2 h 35 de temps émulé au total).

```
python3 UnitTests/tools/run_shaker.py --unittests <build>/CPCCore/UnitTests/unitTests --fetch-real   # une fois
python3 UnitTests/tools/run_shaker.py --unittests <build>/CPCCore/UnitTests/unitTests
```

- `--fetch-real` télécharge la liste des tests et les photos sur machine réelle du site SHAKER
  (≈ 400 Mo, dans `<build>/.../test-work/shaker_real`, certificat du site non vérifié) ;
- les 25 scripts tournent en parallèle (`--jobs`, un processus par script) ;
- le rapport `test-work/shaker_report/index.html` (+ `summary.json`) affiche les compteurs OK / KO /
  à valider et, pour chaque capture, la photo sur machine réelle du même code SSM ;
- on coche les captures conformes, le rapport donne la commande `--accept <case>:<HHLL>,...` qui les
  copie dans `reference/` ; `--revoke` en retire ;
- code de sortie 1 seulement s'il y a des KO.

Options : `--cases A_0,E_2`, `--report-only`.

**CI** (`.github/workflows/build_ubuntu.yml`, étape *Shaker*) : la suite tourne à chaque PR vers
`master` / `develop` et échoue si une référence change. Le rapport est archivé (artefact
`Shaker_report`), sans les photos : pour les voir, le télécharger dans `<build>/.../test-work/shaker_report`
puis lancer `--fetch-real` et `--report-only` en local.

Pré-tri visuel (`triage/<case>.json`) : `ok`, `moniteur` (le CPC est juste, seul l'effet moniteur
diffère de la photo), `doute`, `ko`. Seuls les `ok` validés deviennent des références.

Un script CSL quelconque peut tourner sans fenêtre :
`CPCCORE_CSL_SCRIPT=fichier.csl unitTests --gtest_filter=Csl.RunScriptFromEnvironment`.

## Écarts avec les scripts distribués

- CRTC 3 : c'est le CRTC de l'ASIC, les scripts tournent sur un 6128 Plus FR. Le test ajoute
  `key_output '\(FN1)'` avant `disk_insert` (menu de la cartouche → BASIC) ; le rapport le signale.
- Clavier : SHAKER lit la matrice ; son « Z » (base R9) est la ligne 8 bit 7, marquée W sur l'AZERTY du
  6128 FR. Le test remplace `key_output 'Z'` par `key_output 'W'` ; le rapport le signale.
- `wait_ssm` : la norme ne fixe pas de délai ; le test abandonne après 5 min émulées (la page la plus
  longue, module C test 7, dure environ 64 s).
- `wait_ssm0000` / `wait_ssm` se terminent aussitôt si le code est arrivé depuis le `wait_ssm`
  précédent : SHAKER envoie `#0000` dès qu'il lit la touche, avant que le script n'attende.
- Capture : la norme SSM dit « immédiatement après l'octet `#HH` ». Le code arrive en cours d'image :
  la capture prend les lignes déjà terminées de la trame en cours et, en dessous, la trame précédente.
- Le site SHAKER n'a pas de photos du module E.
