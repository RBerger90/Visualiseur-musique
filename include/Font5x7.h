#pragma once

#include <stdint.h>

const int FONT_WIDTH = 5;
const int FONT_HEIGHT = 7;

// Renvoie les FONT_HEIGHT rangees du caractere c, de haut en bas. Dans chaque
// rangee, les FONT_WIDTH bits de poids faible representent les colonnes de
// gauche a droite (1 = pixel allume). Les lettres minuscules sont converties
// en majuscules. Un caractere non defini dans la police (accents, symboles...)
// renvoie un glyphe vide : ajouter une entree dans Font5x7.cpp pour l'ajouter.
const uint8_t *getGlyphRows(char c);
