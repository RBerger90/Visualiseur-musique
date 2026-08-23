#pragma once

// A appeler a chaque frame pour un bouton branche entre `pin` et GND (pin en
// INPUT_PULLUP). Renvoie true une seule fois au moment de l'appui (front
// descendant), avec anti-rebond logiciel, meme si le bouton reste enfonce
// plusieurs frames de suite.
bool buttonPressed(int pin);
