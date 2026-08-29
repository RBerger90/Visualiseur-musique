#pragma once

#include <FastLED.h>
#include "Config.h"

// A appeler une fois dans setup() : initialise le bandeau/la matrice de LED.
void ledEffectsSetup();

// A appeler a chaque frame, apres avoir rempli le buffer de LEDs avec un effet.
void ledEffectsShow();

// Regle la luminosite globale du bandeau (0 = eteint, 255 = luminosite max).
void ledEffectsSetBrightness(int brightness);

// Palettes disponibles pour effectSpectrumBars(). L'ordre est celui dans
// lequel l'encodeur les fait defiler (mode POT_MODE_COLOR_PALETTE).
// Chaque forme (arc-en-ciel, degrades) existe en version horizontale (varie
// selon la colonne, donc la frequence) et verticale (varie selon la hauteur
// dans la colonne, donc l'amplitude). PALETTE_SOLID n'a qu'une version : une
// couleur unie ne change pas selon l'orientation choisie.
enum PaletteMode
{
  PALETTE_RAINBOW_STATIC_H,
  PALETTE_RAINBOW_STATIC_V,
  PALETTE_RAINBOW_ANIMATED_H,
  PALETTE_RAINBOW_ANIMATED_V,
  PALETTE_SOLID,
  PALETTE_GRADIENT_1_H,
  PALETTE_GRADIENT_1_V,
  PALETTE_GRADIENT_2_H,
  PALETTE_GRADIENT_2_V,
  PALETTE_MODE_COUNT
};

// Change la palette utilisee par effectSpectrumBars().
void ledEffectsSetPalette(PaletteMode mode);

// --- Effets ---
// Chaque effet lit les donnees audio (une valeur par colonne, calculee a partir
// du micro) et met a jour le buffer de LEDs en consequence. Il faut ensuite
// appeler ledEffectsShow() pour envoyer le resultat au bandeau.
// C'est ici qu'il faudra ajouter les prochains effets (un par fonction).

// Barres de spectre : chaque colonne s'allume jusqu'a une hauteur calculee a
// partir de bands[x] (magnitude FFT de la colonne). La couleur de chaque LED
// depend de la palette active (cf. PaletteMode), selon la colonne et/ou la
// ligne.
void effectSpectrumBars(const float bands[MATRIX_WIDTH]);

// Fait defiler `text` de droite a gauche, une colonne de moins toutes les
// stepIntervalMs. L'etat de defilement est conserve en interne (un seul
// defilement actif a la fois) : appeler cette fonction a chaque frame pour
// faire avancer l'animation, le texte reprend a droite une fois sorti a gauche.
void effectScrollingText(const char *text, CRGB color, uint16_t stepIntervalMs = 80);

void effectFillSnake(const int ledCount, const CRGB color);

// Superpose un repere de reglage sur les 3 premieres colonnes (a appeler
// apres l'effet principal, pour remplacer ces colonnes plutot que les
// superposer) :
// - colonne 0 : quel parametre est regle. Une seule LED allumee, au rang
//   modeIndex en partant du haut (0 = tout en haut).
// - colonne 1 : la valeur de ce parametre, en barre remplie depuis le bas.
//   barLevel va de 0 a MATRIX_HEIGHT-1 ; barLevel < 0 signifie qu'aucune
//   valeur n'est encore affichable pour ce mode, la colonne reste eteinte.
// - colonne 2 : toujours eteinte, separe le repere du reste du visuel.
void effectParamHud(int modeIndex, int barLevel);
