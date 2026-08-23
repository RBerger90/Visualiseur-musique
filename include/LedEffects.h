#pragma once

#include <FastLED.h>
#include "Config.h"

// A appeler une fois dans setup() : initialise le bandeau/la matrice de LED.
void ledEffectsSetup();

// A appeler a chaque frame, apres avoir rempli le buffer de LEDs avec un effet.
void ledEffectsShow();

// Regle la luminosite globale du bandeau (0 = eteint, 255 = luminosite max).
void ledEffectsSetBrightness(int brightness);

// --- Effets ---
// Chaque effet lit les donnees audio (une valeur par colonne, calculee a partir
// du micro) et met a jour le buffer de LEDs en consequence. Il faut ensuite
// appeler ledEffectsShow() pour envoyer le resultat au bandeau.
// C'est ici qu'il faudra ajouter les prochains effets (un par fonction).

// Barres de spectre : chaque colonne s'allume jusqu'a la hauteur donnee par
// levels[x] (0 = eteinte, MATRIX_HEIGHT = colonne pleine), avec une couleur
// fixe par colonne.
void effectSpectrumBars(const int levels[MATRIX_WIDTH]);

// Fait defiler `text` de droite a gauche, une colonne de moins toutes les
// stepIntervalMs. L'etat de defilement est conserve en interne (un seul
// defilement actif a la fois) : appeler cette fonction a chaque frame pour
// faire avancer l'animation, le texte reprend a droite une fois sorti a gauche.
void effectScrollingText(const char *text, CRGB color, uint16_t stepIntervalMs = 80);

void effectFillSnake(const int ledCount, const CRGB color);
