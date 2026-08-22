#pragma once

#include <FastLED.h>
#include "Config.h"

// A appeler une fois dans setup() : initialise le bandeau/la matrice de LED.
void ledEffectsSetup();

// A appeler a chaque frame, apres avoir rempli le buffer de LEDs avec un effet.
void ledEffectsShow();

// --- Effets ---
// Chaque effet lit les donnees audio (une valeur par colonne, calculee a partir
// du micro) et met a jour le buffer de LEDs en consequence. Il faut ensuite
// appeler ledEffectsShow() pour envoyer le resultat au bandeau.
// C'est ici qu'il faudra ajouter les prochains effets (un par fonction).

// Barres de spectre : chaque colonne s'allume jusqu'a la hauteur donnee par
// levels[x] (0 = eteinte, MATRIX_HEIGHT = colonne pleine), avec une couleur
// fixe par colonne.
void effectSpectrumBars(const int levels[MATRIX_WIDTH]);
