# Canon XL : retour sur l'établi
[English](README.md) | **Français**\
![Objectif Canon XL avec une capture des signaux SPI en superposition](doc/images/canon-xl-lens-spi-cover.jpg)\
Il y a quelques années, je me suis mis en tête de faire fonctionner des objectifs
vidéo Canon XL sans leur caméscope d'origine. Ces optiques ont encore de beaux
arguments : zoom motorisé, stabilisation et, sur certains modèles, éléments en
fluorite. Ça aurait été dommage de les laisser prendre la poussière avec le
caméscope.

Mais le projet, qui était bien avancé, a lui aussi passé quelque temps sur une
étagère. Aujourd'hui, je le ressors et je reprends le contrôleur Arduino développé
lors des premiers essais.
Ce dépôt rassemble le code revu, le câblage, ce que j'ai compris du protocole et
les tests qui permettent de continuer le travail.

## Une seconde vie en astronomie

Ce qui m'intéresse derrière ce projet, c'est de réutiliser du matériel déclassé
pour observer le ciel. Transformer ces objectifs en « mini-télescopes » donne un
petit banc de démonstration et de prise de vue, sur lequel on peut expérimenter
le cadrage, la mise au point et l'acquisition d'images.

Les premiers essais avaient déjà permis de photographier la Lune. Pour cette reprise,
je me concentre d'abord sur le contrôle de l'optique. L'intégration du reste viendra
ensuite.\
![Objectif Canon XL 20x avec multiplicateur 1,6x, bague imprimée en 3D et caméra ZWO ASI678MC](doc/images/canon-xl-20x-asi678mc-adapter.jpg)\
Le montage de prise de vue : un objectif XL 20x, un multiplicateur 1,6x et une
caméra ZWO ASI678MC, reliés par une bague d'adaptation imprimée en 3D.\
![Vue en coupe de la bague d'adaptation entre la monture XL et une caméra ASI](doc/images/canon-xl-asi-camera-adapter-cutaway.jpg)\
La bague vue en coupe. Le fichier STL est disponible dans le
[dossier des pièces imprimables](hardware/README.md).\
![Objectif Canon XL 20x sur trépied et ordinateur affichant la Lune](doc/images/canon-xl-20x-lunar-imaging-setup.jpg)\
Le montage des premiers essais, avec l'objectif XL 20x et son multiplicateur
1,6x sur trépied. Sur l'ordinateur, la Lune.\
![La Lune photographiée avec un objectif Canon XL 20x](doc/images/canon-xl-20x-moon.jpg)\
Une image de la Lune obtenue lors de ces premiers essais, avec l'objectif XL 20x,
le multiplicateur 1,6x et un filtre Baader Continuum 10 nm.

À terme, j'aimerais réunir trois projets : ce contrôleur d'objectif, l'acquisition
d'images avec [EIP](https://eip.eliotis.com) et le pilotage de la monture du télescope.
J'envisage aussi une base Raspberry Pi avec une caméra Pi pour la prise de vue.
Chaque projet garde
son rôle, et il reste à les faire travailler ensemble pour pouvoir piloter
l'observation de bout en bout.

## À l'écoute de l'objectif

Pour commencer, il fallait comprendre ce que le caméscope racontait à son objectif.
J'ai donc soudé des fils sur les contacts du culot d'un 16x pour récupérer les
signaux SPI et les envoyer à un analyseur logique. Pendant ces captures, le
caméscope reste maître du bus : l'analyseur se contente d'enregistrer les échanges.\
![Caméscope Canon avec objectif XL 16x, analyseur logique et MacBook Air affichant les signaux](doc/images/canon-xl-16x-logic-analyser-bench.jpg)\
Le banc de rétro-ingénierie : le 16x sur son caméscope, l'analyseur logique et
l'ordinateur portable. Les essais du contrôleur Arduino se font ensuite avec le 20x.

L'horloge et le signal de sélection délimitent les échanges. MOSI porte les
données envoyées par le caméscope ; MISO, celles renvoyées par l'objectif.
La méthode consiste à faire varier une commande à la fois, puis à comparer les
captures. On repère les octets qui changent, on distingue l'initialisation des
échanges habituels et on identifie peu à peu les commandes et les retours.

Les captures étudiées montrent des trames de **40 octets dans chaque sens**, avec
une somme de contrôle. C'est sur cette base que j'ai construit le contrôleur
Arduino : reproduire le dialogue nécessaire au fonctionnement de l'objectif,
puis vérifier sur le banc ce que chaque commande provoque.

Certains champs restent à comprendre. Savoir lire une valeur dans une trame ne
suffit pas à en faire une mesure optique calibrée.

## Ce qui fonctionne aujourd'hui

| Fonction | État |
| --- | --- |
| Dialogue avec l'objectif sans caméscope | ✓ Validé |
| Ouverture et fermeture de l'iris | ✓ Validé |
| Lecture d'informations renvoyées par l'objectif | ✓ Validé pour certaines commandes |
| Zoom manuel | ✓ Validé |
| Zoom commandé à distance | ✓ Validé |
| Pilotage du stabilisateur en X/Y | ✓ Validé |
| Mise au point manuelle | ✓ Validé |
| Mise au point commandée à distance | ☐ À développer |

Ces fonctions ont été validées avec le **Canon XL 20x et un Arduino Nano
classique**.
La compatibilité avec les autres modèles reste à vérifier.

Deux vidéos du prototype d'origine donnent une idée du résultat :

- [Zoom commandé à distance](https://www.youtube.com/watch?v=Sq8p5S8JtGc).
- [Pilotage du stabilisateur](https://www.youtube.com/watch?v=eJLDcAYVf6c).

## Matériel et compatibilité

| Matériel | Validation |
| --- | --- |
| Objectif XL 20x + Nano classique ATmega328P | Dialogue, iris, zoom et pilotage X/Y du stabilisateur validés ; certaines informations de retour identifiées. |
| Objectif XL 16x + caméscope Canon | Dialogue observé à l'analyseur logique ; pilotage avec le contrôleur actuel à valider. |
| Nano RP2040 Connect | Compilation et tests logiciels ; validation électrique et essais sur objectif à réaliser. |

Pour reproduire le banc autonome, il faut :

- Un objectif Canon XL et un Arduino Nano classique.
- Un régulateur séparé pour alimenter l'objectif, avec une masse commune au Nano.
- Les fils de raccordement et un câble USB pour le Nano.
- Un ordinateur avec l'IDE Arduino et un moniteur série.

L'analyseur logique est utile pour étudier le protocole, mais le contrôleur peut
s'utiliser sans lui. Pour la prise de vue, le montage présenté ici utilise une
caméra ZWO ASI678MC et une bague d'adaptation imprimée en 3D.

## Le projet en images
![Objectif Canon XL 20x sur le banc, piloté par le Nano](doc/images/canon-xl-20x-bench.jpg)\
L'objectif 20x fluorite sur le banc, piloté par le Nano sans son caméscope d'origine.\
![Arduino Nano classique utilisé sur le banc](doc/images/classic-nano-bench.jpg)\
Le Nano classique utilisé pour les essais.

## Le montage
![Culot d'un objectif Canon XL ouvert, avec les fils de raccordement visibles](doc/images/canon-xl-lens-mount-wiring.jpg)\
Le culot ouvert montre les fils soudés côté objectif. Leurs couleurs correspondent
au schéma et à la table du [guide de câblage](doc/wiring.md). Ce sont les couleurs
de mon montage.\
![Régulateur UBEC utilisé pour alimenter l'objectif séparément du Nano](doc/images/ubec-lens-power-regulator.jpg)\
Le montage utilise un régulateur **UBEC-3A-7,4 V**, alimenté en **12 V**.
Le Nano pilote l'objectif, qui reçoit son alimentation du régulateur.
**L'objectif ne doit pas être alimenté par une sortie du Nano.** Vérifier la
tension de sortie et la polarité du régulateur avant le raccordement.\
![Vue d'ensemble du contrôleur et de l'alimentation de l'objectif](doc/images/controller-overview.svg)\
![Câblage SPI du Nano classique vers l'objectif Canon XL](doc/images/classic-nano-wiring.svg)\
Le câblage représente les fonctions du faisceau, pas la position des contacts
sur la monture. Le
[guide de câblage](doc/wiring.md) détaille les précautions et les variantes selon
la carte utilisée.

## Pièces imprimables

Trois modèles sont disponibles au format STL :

| Pièce | Fichier |
| --- | --- |
| Bague d'adaptation XL vers caméra ZWO ASI | [canon-xl-to-zwo-asi-adapter.stl](hardware/stl/canon-xl-to-zwo-asi-adapter.stl) |
| Bouchon avant de l'objectif | [canon-xl-front-lens-cap.stl](hardware/stl/canon-xl-front-lens-cap.stl) |
| Bouchon arrière de l'objectif | [canon-xl-rear-lens-cap.stl](hardware/stl/canon-xl-rear-lens-cap.stl) |
### Aperçu
![Aperçu des trois pièces : bague d'adaptation, bouchon avant et bouchon arrière](hardware/images/printed-parts-overview.png)\
Les dimensions et les précautions de montage sont dans le
[guide des pièces imprimables](hardware/README.md).

## Pour commencer

Ouvrir [canon_xl_controller.ino](canon_xl_controller/canon_xl_controller.ino) dans
l'IDE Arduino. Pour le Nano classique utilisé sur le banc, sélectionner
**Arduino Nano**, puis **ATmega328P (Old Bootloader)**. Aucune bibliothèque Arduino
externe n'est nécessaire.

Avant de brancher quoi que ce soit, lire le [guide de câblage](doc/wiring.md).
L'objectif a besoin de sa propre alimentation et d'une masse commune avec le
contrôleur. Le Nano classique et le RP2040 Connect utilisent des niveaux logiques
différents : 5 V et 3,3 V. Avant de changer de carte, vérifier les tensions du bus
SPI et adapter les niveaux si nécessaire.

Sélectionner le port USB du Nano, puis compiler et téléverser le sketch. Si la
carte contient déjà un programme à conserver, en sauvegarder une copie avant
de le remplacer.

Ouvrir ensuite un moniteur série à **115200 bauds**, avec une fin de ligne LF ou
CRLF. Attendre les premières lignes de télémétrie, puis envoyer :

```text
STATUS
```

Une réponse de la forme `XL,1,...,ready,normal,...` (abrégée ici) indique que le
dialogue numérique est établi. Les états `init` et `probe` correspondent à
l'initialisation.
Le [protocole série](doc/serial-protocol.md) décrit les autres champs et les
commandes disponibles.

À garder en tête : l'ouverture du port peut redémarrer le Nano, et le contrôleur
demande la fermeture de l'iris au démarrage.

## Travailler sur le code

Le protocole, le contrôleur, le traitement des commandes série et la télémétrie
sont écrits en C++11, indépendamment d'Arduino. Les modules propres à chaque carte
gèrent le SPI ; le sketch coordonne les échanges et les entrées-sorties série.

Quatre suites de tests peuvent être exécutées sur l'ordinateur, sans objectif
connecté :

```sh
cmake -S . -B build/host -DXL_SANITIZERS=ON
cmake --build build/host
ctest --test-dir build/host --output-on-failure
```

Le [guide de développement](doc/development.md) explique la compilation avec
Arduino CLI, le formatage et la génération de documentation. Les
[conventions de code](doc/coding-style.md) donnent les règles suivies dans le projet.

## L'interface de pilotage

L'interface développée avec [Processing](https://processing.org/) permet de
commander l'iris, le zoom et le stabilisateur X/Y depuis l'ordinateur. Elle
affiche aussi les informations renvoyées par l'objectif et permet d'enregistrer
les échanges. La commande de mise au point reste à développer.\
![Interface Processing actuelle connectée à l'objectif Canon XL 20x, avec les trames de retour affichées](doc/images/canon-xl-processing-control-panel.png)\
Sur cette capture, le panneau est connecté au Nano en lecture seule et affiche
les retours réels du 20x. Il faut activer explicitement les commandes pour agir
sur l'objectif.

Les [sources du panneau et son guide de lancement](processing/README.md) sont
inclus dans ce dépôt, avec un mode de démonstration utilisable sans objectif.

## La suite

Il reste à identifier davantage d'informations dans les retours de l'objectif
et à comparer les modèles. La mise au point commandée à distance est aussi au
programme, puis viendra l'intégration avec l'acquisition d'images et la monture.

## Pourquoi partager ce projet ?

J'aimerais que ces essais puissent servir à ceux qui ont encore un objectif XL
et l'envie de lui trouver un nouvel usage. Le code, le câblage et les observations
sont ici pour ça. Et si d'autres s'y mettent, nous pourrons aussi comparer nos
résultats et mieux comprendre ce matériel.

Vous avez un objectif XL, des mesures ou de l'expérience avec ces optiques ? Les
retours sont les bienvenus, notamment pour comparer les modèles. Pour que les
essais soient reproductibles, pensez à préciser le modèle d'objectif, la carte,
le câblage et la version du firmware utilisée.

## Rétro-ingénierie et indépendance

Ce projet repose sur l'observation des échanges SPI entre un caméscope Canon et
son objectif pendant leur fonctionnement normal. L'analyseur logique enregistre
passivement les signaux, sans envoyer de commandes sur le bus. Ces observations
permettent de développer un contrôleur indépendant pour réutiliser l'objectif
sans son caméscope d'origine. Cette démarche se distingue de la décompilation
d'un firmware.

**Le droit français reconnaît cette possibilité d'étude sous certaines conditions.**
L'[article L151-3 du Code de commerce](https://www.legifrance.gouv.fr/codes/article_lc/LEGIARTI000037266559)
prévoit que l'observation, l'étude, le démontage ou le test d'un produit mis à la
disposition du public ou détenu licitement constituent des modes d'obtention
licite d'un secret des affaires, sauf restriction contractuelle applicable.

Pour les logiciels embarqués, le **III de
l'[article L122-6-1 du Code de la propriété intellectuelle](https://www.legifrance.gouv.fr/codes/article_lc/LEGIARTI000044365559)**
permet à la personne ayant le droit d'utiliser le logiciel d'en observer, étudier
ou tester le fonctionnement lors d'opérations qu'elle est autorisée à effectuer,
afin d'en comprendre les idées et principes. Les clauses contraires à cette
disposition sont nulles. Le IV encadre séparément la reproduction ou la traduction
du code indispensable à l'interopérabilité.

Ce dépôt ne distribue ni firmware Canon ni copie de documentation constructeur.
La publication des résultats reste soumise aux droits applicables et aux
éventuelles obligations de confidentialité. L'ancienneté du matériel ne suffit
pas non plus à exclure l'existence de brevets applicables. Une
[note juridique et de licence](doc/legal-and-licensing.md) détaille les règles
relatives à l'observation du matériel et des logiciels, à la publication des
résultats, aux marques, aux brevets et aux composants tiers.

Ce projet n'est ni affilié à Canon ni approuvé par la marque. Le nom Canon est
utilisé uniquement pour identifier le matériel concerné.

## Conditions de partage

CanonXL est distribué sous la [licence non commerciale CanonXL, version 1.0](LICENSE).
Vous pouvez utiliser, modifier et redistribuer les sources pour vos projets
personnels, pédagogiques et scientifiques non commerciaux. La recherche et
l'enseignement institutionnels sont autorisés, même avec financement, frais de
scolarité ou salariés, tant qu'ils ne servent pas au développement d'un produit
ou service commercial ni à une prestation de recherche commerciale.

Lors d'une redistribution, conservez la licence et les attributions, indiquez vos
modifications et fournissez les sources correspondantes si vous distribuez un
firmware compilé. Les composants tiers conservent leurs propres licences.

**Toute exploitation commerciale nécessite un accord écrit préalable**, notamment
la vente de produits intégrant le code, les services payants et la R&D commerciale.

Contact : [ouvrir une issue sur GitHub](https://github.com/NoxSilicium/CanonXL/issues/new).

Le [guide de licence](doc/licensing.md) précise les conditions d'utilisation,
de modification et de redistribution.

Le texte anglais de `LICENSE` fait foi. Il s'agit d'une licence à sources
disponibles, pas d'une GPLv3 ni d'une licence open source au sens strict.
