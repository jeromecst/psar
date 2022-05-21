## Projet SAR 2022 - Optimisation du noyau Linux

Ce dépôt contient toutes les sources de la plateforme de test qui a été dévelopée pour l'étude des mécanismes
d'ordonnancement NUMA du noyau Linux en interaction avec le page cache.

### Prérequis

* Un compilateur récent (GCC >= 10 ou Clang >= 10 par exemple)
* CMake (>= 3.16)

### Compilation

```
git clone ...
git submodule update --init --recursive
mkdir build
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
```

### Tests fournis

* test_get_time : mesure des temps de lecture dans différentes configurations.

Tests d'ordonnancement :

* test_get_time_all_bound_forced : aucune migration autorisée
* test_get_time_all_forced : migration de pages autorisée
* test_get_time_all_bound : migration de thread autorisée
* test_get_time_all : toute migration autorisée

Pour plus d'informations, se référer au rapport.

Les tests suivants sont obsolètes et ne sont plus utilisés pour le rapport.

* test_distant_reads_distant_buffer
* test_distant_reads_distant_buffer_forced
* test_distant_reads_local_buffer

### Exécution

Tous les tests doivent être lancés avec les permissions root.

### Licence

Le code de ce dépôt est sous licence GPLv3.
