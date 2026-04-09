// Variables globales pour mémoriser l'état précédent
MergedTrain anciensTrains[5]; // La structure MergedData limite déjà à 5 trains
bool premierAffichage = true;

// =========================================================================
// FONCTION UTILITAIRE : Dessine UNE SEULE ligne de train
// =========================================================================
void dessinerLigneTrain(const MergedTrain &train, int index, int startY, int espacementY) {
  int x = 5;
  int y = startY + (index * espacementY);

  // Variables booléennes pour simplifier la lecture
  bool enRetard = (train.prevu != train.reel);
  bool aUneAlerte = (train.alerteMsg != "" && train.alerteMsg != "-");

  // -- 1. LOGIQUE D'AFFICHAGE DE L'HEURE --
  if (!enRetard) {
    // Train à l'heure
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(x, y);
    display.print(train.prevu);
  } else {
    // Train en retard : On écrit l'heure prévue en rouge et on la barre
    display.setTextColor(GxEPD_RED);
    display.setCursor(x, y);
    display.print(train.prevu);

    // Calcul de la zone du texte pour dessiner la ligne par-dessus
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(train.prevu, x, y, &x1, &y1, &w, &h);

    int hauteurLigne = y - (h / 2) + 2;
    display.drawLine(x, hauteurLigne, x + w, hauteurLigne, GxEPD_RED);
    display.drawLine(x, hauteurLigne - 1, x + w, hauteurLigne - 1, GxEPD_RED); // Ligne double pour plus d'épaisseur

    // On décale le curseur pour écrire l'heure réelle
    x += w + 8; 
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(x, y);
    display.print(train.reel);
  }

  // -- 2. AFFICHAGE DU STATUT / ALERTE --
  x = 110; // Position X pour démarrer les messages
  display.setCursor(x, y);

  if (!enRetard && !aUneAlerte) {
    // Tout va bien : on l'écrit en noir
    display.setTextColor(GxEPD_BLACK);
    display.print("A l'heure");
  } else {
    // Soit en retard, soit une alerte : on attire l'attention en rouge
    display.setTextColor(GxEPD_RED);
    
    if (aUneAlerte) {
      // On affiche l'alerte textuelle nettoyée de ses accents
      display.print(enleverAccents(train.alerteMsg.substring(0, 22)));
    } else {
      // S'il est en retard sans texte d'alerte spécifique, on affiche son délai
      display.print(enleverAccents(train.retardStr)); // Affiche par ex "⚠️ +5m" (sans l'émoji si ta police ne le supporte pas)
    }
  }
}

// =========================================================================
// FONCTION PRINCIPALE : Affichage intelligent des horaires
// =========================================================================
void afficherHorairesTrains(const MergedData &affichageFinal) {
  int startY = 25;
  int espacementY = 30;

  if (premierAffichage) {
    // ---------------------------------------------------------
    // 1er PASSAGE : Rafraîchissement complet de l'écran (Full)
    // ---------------------------------------------------------
    display.setFullWindow();
    display.firstPage();
    do {
      display.fillScreen(GxEPD_WHITE);
      for (int i = 0; i < affichageFinal.count; i++) {
        dessinerLigneTrain(affichageFinal.trains[i], i, startY, espacementY);
        anciensTrains[i] = affichageFinal.trains[i];  // On sauvegarde l'état en mémoire
      }
    } while (display.nextPage());

    premierAffichage = false;

  } else {
    // ---------------------------------------------------------
    // PASSAGES SUIVANTS : Mise à jour partielle (Fast)
    // ---------------------------------------------------------
    for (int i = 0; i < affichageFinal.count; i++) {

      // On vérifie s'il y a une différence avec l'ancien état mémorisé
      if (affichageFinal.trains[i].prevu != anciensTrains[i].prevu || 
          affichageFinal.trains[i].reel != anciensTrains[i].reel || 
          affichageFinal.trains[i].alerteMsg != anciensTrains[i].alerteMsg) {

        // Calcul de la zone rectangulaire (Bounding Box) à effacer et redessiner
        int yBase = startY + (i * espacementY);
        int windowY = yBase - 20; 
        int windowHeight = espacementY;

        display.setPartialWindow(0, windowY, display.width(), windowHeight);

        display.firstPage();
        do {
          // On efface l'ancienne ligne
          display.fillRect(0, windowY, display.width(), windowHeight, GxEPD_WHITE);
          // On dessine la nouvelle ligne
          dessinerLigneTrain(affichageFinal.trains[i], i, startY, espacementY);
        } while (display.nextPage());

        // On met à jour la mémoire
        anciensTrains[i] = affichageFinal.trains[i];
      }
    }
  }
}

String enleverAccents(String texte) {
  // 1. Remplacement des accents français courants (UTF-8)
  texte.replace("é", "e");
  texte.replace("è", "e");
  texte.replace("ê", "e");
  texte.replace("ë", "e");
  texte.replace("à", "a");
  texte.replace("â", "a");
  texte.replace("ç", "c");
  texte.replace("î", "i");
  texte.replace("ï", "i");
  texte.replace("ô", "o");
  texte.replace("ù", "u");
  texte.replace("û", "u");
  
  // Majuscules
  texte.replace("É", "E");
  texte.replace("È", "E");
  texte.replace("Ê", "E");
  texte.replace("À", "A");
  texte.replace("Ç", "C");

  // 2. Suppression des caractères non-ASCII restants (supprimera les Emojis !)
  String textePropre = "";
  for (int i = 0; i < texte.length(); i++) {
    char c = texte[i];
    // On garde uniquement les caractères de la table ASCII standard
    if (c >= 32 && c <= 126) { 
      textePropre += c;
    }
  }
  
  return textePropre;
}