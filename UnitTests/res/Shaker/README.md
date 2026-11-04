# SHAKER

SHAKER (Longshot / Logon System, https://shaker.logonsystem.eu) teste le CRTC et le Gate Array.

| Fichier | Contenu |
|---|---|
| `shaker27.dsk` | SHAKER 2.7, modules SHAKE27A à SHAKE27E |
| `shaker24.dsk`, `shaker_addon.dsk` | anciennes versions |
| `CSL/MODULE_x/SHAKER-x-n.CSL` | scripts CSL officiels (`Shaker_CSL.zip`) : module x, CRTC n |
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

Un script CSL quelconque peut tourner sans fenêtre :
`CPCCORE_CSL_SCRIPT=fichier.csl unitTests --gtest_filter=Csl.RunScriptFromEnvironment`.

## Écarts avec les scripts distribués

- Module A : les scripts nomment encore `shaker26.dsk` et `RUN"SHAKE26A"`, absents du disque 2.7.
  Le test les remplace par `shaker27.dsk` / `SHAKE27A` (le rapport le signale).
- `wait_ssm0000` / `wait_ssm` se terminent aussitôt si le code est arrivé depuis le `wait_ssm`
  précédent : SHAKER envoie `#0000` dès qu'il lit la touche, avant que le script n'attende.
- Le site SHAKER n'a pas de photos du module E.
- E_0 et E_1 s'interrompent : la page `#0405` (E_0) arrive vers 4,2 s au lieu de 3 s, la page
  `#03FC` (E_1) vers 6 s au lieu de 5 s (écart de timing de l'émulation, à expliquer).
