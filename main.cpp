#include <iostream>
#include "InMemoryEngine.h"

int main() {
    std::cout << "--- NanoDB System Startup ---\n";

    // 1. POLIMORFISMO PURO
    // Creo un InMemoryEngine, ma lo tratto come un generico DBEngine.
    // Questo è fondamentale: il resto del main non sa come i dati sono salvati!
    DBEngine<std::string, std::string>* db = new InMemoryEngine<std::string, std::string>();

    // 2. Inserimento
    std::cout << "Inserisco dati...\n";
    db->put("user:101", "Alessio");
    db->put("config:mode", "Dark");

    // 3. Lettura
    std::optional<std::string> user = db->get("user:101");
    std::cout << "Trovato user:101 -> " << user.value() << "\n";

    // 4. Test chiave inesistente
    std::optional<std::string> ghost = db->get("user:999");
    if (!ghost.has_value()) {
        std::cout << "user:999 non esiste (Corretto)\n";
    } else {
        std::cout << "Errore: Trovato fantasma!\n";
    }

    // 5. Rimozione
    bool removed = db->remove("config:mode");
    std::cout << "Rimozione config:mode: " << (removed ? "Successo" : "Fallito") << "\n";

    // 6. CLEANUP (La domanda Bonus)
    // Qui chiamo delete su un puntatore DBEngine*.
    delete db;

    std::cout << "\n--- DB Numerico ---\n";
    DBEngine<int, float>* numDB = new InMemoryEngine<int, float>();

    numDB-> put(32, 19.4f);
    std::cout << "Trovato 32: " << numDB->get(32).value() << "\n";

    delete numDB;

    std::cout << "--- System Shutdown ---\n";
    return 0;
}