#pragma once

// A appeler une fois dans setup(). Configure clkPin/dtPin en entree avec
// pull-up et attache les interruptions qui decodent le signal en quadrature.
void encoderSetup(int clkPin, int dtPin);

// Nombre de crans effectues depuis le dernier appel : positif en rotation
// horaire, negatif en antihoraire, 0 si l'encodeur n'a pas bouge. Remet le
// compteur a zero a chaque appel, donc a n'interroger qu'une fois par frame.
int encoderRead();
