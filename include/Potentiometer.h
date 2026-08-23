#pragma once

// A appeler a chaque frame. Lit le potentiometre, lisse la valeur brute de
// l'ADC (bruite par nature sur l'ESP32) via une moyenne mobile exponentielle,
// puis la fige des qu'aucun mouvement significatif n'est detecte pendant un
// court delai. Ca evite que la valeur affichee/utilisee tressaute quand
// l'utilisateur ne touche plus au potar.
// Renvoie une valeur stable entre minValue et maxValue.
int potRead(const int potPin, const int minValue, const int maxValue);
