#include <iostream>
#include <vector>
#include <chrono>   // <--- Libreria per il tempo
#include <string>
#include "InMemoryEngine.h"
#include "FileEngine.h"

// Funzione generica per testare un motore
void runBenchmark(DBEngine<std::string, std::string> *db, int count, std::string engineName) {
    std::cout << "--- Benchmarking: " << engineName << " (" << count << " ops) ---\n";

    // 1. MISURA SCRITTURA (PUT)
    auto startWrite = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < count; ++i) {
        // Creiamo chiavi uniche: key:0, key:1, ...
        std::string key = "key:" + std::to_string(i);
        std::string val = "value_" + std::to_string(i);
        db->put(key, val);
    }

    auto endWrite = std::chrono::high_resolution_clock::now();
    auto durationWrite = std::chrono::duration_cast<std::chrono::milliseconds>(endWrite - startWrite).count();

    std::cout << ">> Write Time: " << durationWrite << " ms ("
            << (count * 1000.0 / durationWrite) << " ops/sec)\n";

    // 2. MISURA LETTURA (GET)
    auto startRead = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < count; ++i) {
        std::string key = "key:" + std::to_string(i);
        auto result = db->get(key);
        // Piccolo check per evitare che il compilatore ottimizzi via la lettura
        if (!result.has_value()) std::cerr << "Errore! Chiave persa: " << key << "\n";
    }

    auto endRead = std::chrono::high_resolution_clock::now();
    auto durationRead = std::chrono::duration_cast<std::chrono::milliseconds>(endRead - startRead).count();

    std::cout << ">> Read Time:  " << durationRead << " ms ("
            << (count * 1000.0 / durationRead) << " ops/sec)\n";

    std::cout << "------------------------------------------\n\n";
}

void printFileSize(const std::string& filename) {
    try {
        auto size = std::filesystem::file_size(filename);
        std::cout << "[DISK INFO] File '" << filename << "' size: " << size << " bytes.\n";
    } catch (...) {
        std::cout << "[DISK INFO] File '" << filename << "' not found (0 bytes).\n";
    }
}

int main() {
    const std::string dbName = "stress_test.db";
    const int NUM_RECORDS = 100;

    // PULIZIA INIZIALE: Partiamo da zero
    std::remove(dbName.c_str());
    std::cout << "=== INIZIO STRESS TEST ===\n\n";

    // --- FASE 1: POPOLAMENTO MASSIVO ---
    std::cout << "--- FASE 1: Scrittura e Riavvio ---\n";
    {
        FileEngine db(dbName);
        for (int i = 0; i < NUM_RECORDS; ++i) {
            // Chiavi: user:0, user:1 ...
            db.put("user:" + std::to_string(i), "Dato_Molto_Importante_" + std::to_string(i));
        }
        std::cout << ">> Scritti " << NUM_RECORDS << " record.\n";
    }
    // Qui si chiude lo scope: db viene distrutto (flush su disco).
    printFileSize(dbName);


    // --- FASE 2: CANCELLAZIONE (Creazione di "Buchi") ---
    std::cout << "\n--- FASE 2: Cancellazione del 50% dei dati (Tombstones) ---\n";
    {
        FileEngine db(dbName); // Riapriamo (Rehydration)

        // Cancelliamo tutti i numeri PARI
        int deletedCount = 0;
        for (int i = 0; i < NUM_RECORDS; i += 2) {
            bool success = db.remove("user:" + std::to_string(i));
            if (success) deletedCount++;
        }
        std::cout << ">> Cancellati " << deletedCount << " record (numeri pari).\n";

        // A questo punto il file contiene: 100 inserimenti + 50 tombstones.
        // È PIÙ GRANDE di prima, anche se ha meno dati logici!
    }
    printFileSize(dbName); // Dovrebbe essere cresciuto!


    // --- FASE 3: VERIFICA ZOMBIE (I dati cancellati sono morti?) ---
    std::cout << "\n--- FASE 3: Verifica Integrita' (No Zombie) ---\n";
    {
        FileEngine db(dbName);

        // Testiamo una chiave cancellata (user:0)
        if (db.get("user:0").has_value()) {
            std::cerr << "ERRORE GRAVE: user:0 dovrebbe essere morto!\n";
            return 1;
        } else {
            std::cout << ">> OK: user:0 e' correttamente sparito.\n";
        }

        // Testiamo una chiave viva (user:1)
        if (db.get("user:1").has_value()) {
            std::cout << ">> OK: user:1 esiste ancora.\n";
        } else {
            std::cerr << "ERRORE GRAVE: user:1 e' sparito ma non doveva!\n";
            return 1;
        }
    }


    // --- FASE 4: COMPATTAZIONE (Il momento della verità) ---
    std::cout << "\n--- FASE 4: Compattazione (Garbage Collection) ---\n";
    {
        FileEngine db(dbName);
        std::cout << ">> Avvio compact()...\n";
        db.compact();
        std::cout << ">> Compattazione terminata.\n";
    }

    // ORA IL FILE DEVE ESSERE DIMINUITO DRASTICAMENTE!
    // Dovrebbe contenere solo i 50 record dispari. Niente più tombstones o dati vecchi.
    printFileSize(dbName);


    // --- FASE 5: VERIFICA FINALE POST-COMPATTAZIONE ---
    std::cout << "\n--- FASE 5: Verifica Finale ---\n";
    {
        FileEngine db(dbName); // Ricarichiamo dal file nuovo compattato

        // Ricontrolliamo che user:1 ci sia ancora
        auto val = db.get("user:1");
        if (val.has_value() && val.value() == "Dato_Molto_Importante_1") {
            std::cout << ">> SUCCESS: I dati sono sopravvissuti alla compattazione.\n";
        } else {
            std::cerr << "ERRORE CRITICO: Dati corrotti dopo la compattazione!\n";
            return 1;
        }
    }

    std::cout << "\n=== TEST SUPERATO CON SUCCESSO! PUOI COMMITTARE ===\n";
    return 0;
}