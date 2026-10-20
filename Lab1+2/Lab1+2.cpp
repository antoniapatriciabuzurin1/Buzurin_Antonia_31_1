#define NOMINMAX
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <windows.h>

struct ArticolVector {
    std::string numeFisier;
    std::map<int, int> vectorFrecvente; 
};

std::unordered_set<std::string> stopWords;
std::map<std::string, int> dictionarGlobal; 
std::vector<ArticolVector> vectoriArticole;

void incarcaStopWords(const std::string& caleFisier) {
    std::ifstream file(caleFisier);
    if (!file.is_open()) return;
    std::string cuvant;
    while (file >> cuvant) {
        std::transform(cuvant.begin(), cuvant.end(), cuvant.begin(), [](unsigned char c) {
            return std::tolower(c);
            });
        stopWords.insert(cuvant);
    }
    file.close();
}

std::string inlocuiesteEntitatiXML(std::string text) {
    size_t pos;
    while ((pos = text.find("&quot;")) != std::string::npos) text.replace(pos, 6, "\"");
    while ((pos = text.find("&amp;")) != std::string::npos) text.replace(pos, 5, "&");
    while ((pos = text.find("&lt;")) != std::string::npos) text.replace(pos, 4, "<");
    while ((pos = text.find("&gt;")) != std::string::npos) text.replace(pos, 4, ">");
    while ((pos = text.find("&apos;")) != std::string::npos) text.replace(pos, 6, "'");
    return text;
}

std::string eliminaTaguriXML(const std::string& text) {
    std::string rezultat = "";
    bool inTag = false;
    for (char ch : text) {
        if (ch == '<') {
            inTag = true;
        }
        else if (ch == '>') {
            inTag = false;
            rezultat += ' ';
        }
        else if (!inTag) {
            rezultat += ch;
        }
    }
    return rezultat;
}

std::string obtineRadacina(const std::string& cuvant) {
    std::string s = cuvant;
    if (s.length() <= 3) return s;

    if (s.length() > 4 && s.substr(s.length() - 3) == "ing") return s.substr(0, s.length() - 3);
    if (s.length() > 3 && s.substr(s.length() - 2) == "ed") return s.substr(0, s.length() - 2);
    if (s.length() > 3 && s.substr(s.length() - 2) == "es") return s.substr(0, s.length() - 2);
    if (s.length() > 3 && s.back() == 's' && s[s.length() - 2] != 's') return s.substr(0, s.length() - 1);

    return s;
}

std::string curataSiFiltreazaText(const std::string& text) {
    std::string textFaraEntitati = inlocuiesteEntitatiXML(text);
    std::string faraTaguri = eliminaTaguriXML(textFaraEntitati);
    std::string textCurat = "";

    for (char ch : faraTaguri) {
        unsigned char uc = static_cast<unsigned char>(ch);
        if (std::isalnum(uc) || std::isspace(uc)) {
            textCurat += static_cast<char>(std::tolower(uc));
        }
        else {
            textCurat += ' ';
        }
    }

    std::stringstream ss(textCurat);
    std::string cuvant;
    std::string rezultat = "";

    while (ss >> cuvant) {
        if (stopWords.find(cuvant) == stopWords.end() && cuvant.length() > 1) {
            rezultat += cuvant + " ";
        }
    }

    return rezultat;
}

std::string extrageEticheta(const std::string& continut, const std::string& eticheta) {
    std::string continutLower = continut;
    std::transform(continutLower.begin(), continutLower.end(), continutLower.begin(), [](unsigned char c) {
        return std::tolower(c);
        });

    std::string tagStart = "<" + eticheta;
    std::string tagEnd = "</" + eticheta + ">";

    size_t posStart = continutLower.find(tagStart);
    if (posStart == std::string::npos) return "";

    size_t posStartClose = continutLower.find(">", posStart);
    if (posStartClose == std::string::npos) return "";

    size_t posEnd = continutLower.find(tagEnd, posStartClose);
    if (posEnd == std::string::npos || posEnd <= posStartClose) return "";

    size_t dataStart = posStartClose + 1;
    size_t dataLen = posEnd - dataStart;

    return continut.substr(dataStart, dataLen);
}

std::string getNumeFisierFaraExtensie(const std::string& numeFisier) {
    size_t lastDot = numeFisier.find_last_of(".");
    if (lastDot == std::string::npos) return numeFisier;
    return numeFisier.substr(0, lastDot);
}

std::vector<std::string> extrageRadacini(const std::string& text) {
    std::vector<std::string> radacini;
    std::stringstream ss(text);
    std::string cuvant;
    while (ss >> cuvant) {
        if (cuvant != "titlu:" && cuvant != "text:") {
            radacini.push_back(obtineRadacina(cuvant));
        }
    }
    return radacini;
}

void proceseazaFisierXML(const std::string& caleXml, const std::string& folderDestinatie, const std::string& numeFisier) {
    std::ifstream fileIn(caleXml);
    if (!fileIn.is_open()) return;

    std::string continut((std::istreambuf_iterator<char>(fileIn)), std::istreambuf_iterator<char>());
    fileIn.close();

    std::string titluBrut = extrageEticheta(continut, "title");
    std::string textBrut = extrageEticheta(continut, "text");
    if (textBrut.empty()) {
        textBrut = extrageEticheta(continut, "body");
    }

    std::string titluFiltrat = curataSiFiltreazaText(titluBrut);
    std::string textFiltrat = curataSiFiltreazaText(textBrut);

    std::string caleTxt = folderDestinatie + "\\" + getNumeFisierFaraExtensie(numeFisier) + ".txt";
    std::ofstream fileOut(caleTxt);

    if (fileOut.is_open()) {
        fileOut << "titlu: " << titluFiltrat << "\n";
        fileOut << "text: " << textFiltrat << "\n";
        fileOut.close();
    }
}

void cautaSiProceseazaXML(const std::string& folderCurent, const std::string& folderDestinatie, int& contor) {
    std::string cautarePath = folderCurent + "\\*";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(cautarePath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        std::string numeElement = findData.cFileName;
        if (numeElement == "." || numeElement == "..") continue;

        std::string caleCompleta = folderCurent + "\\" + numeElement;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (numeElement != "rezultat_testing" && numeElement != "rezultat_training" &&
                numeElement != "Debug" && numeElement != "Release" && numeElement != ".vs") {
                cautaSiProceseazaXML(caleCompleta, folderDestinatie, contor);
            }
        }
        else {
            std::string numeLower = numeElement;
            std::transform(numeLower.begin(), numeLower.end(), numeLower.begin(), [](unsigned char c) {
                return std::tolower(c);
                });

            if (numeLower.size() >= 4 && numeLower.substr(numeLower.size() - 4) == ".xml") {
                proceseazaFisierXML(caleCompleta, folderDestinatie, numeElement);
                contor++;
            }
        }
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);
}

void adaugaInDictionarGlobal(const std::string& caleTxt) {
    std::ifstream file(caleTxt);
    if (!file.is_open()) return;

    std::string continut((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    std::vector<std::string> radacini = extrageRadacini(continut);
    for (const auto& r : radacini) {
        if (dictionarGlobal.find(r) == dictionarGlobal.end()) {
            int noulIndex = static_cast<int>(dictionarGlobal.size());
            dictionarGlobal[r] = noulIndex;
        }
    }
}

void genereazaVectorArticol(const std::string& caleTxt, const std::string& numeFisier) {
    std::ifstream file(caleTxt);
    if (!file.is_open()) return;

    std::string continut((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    std::vector<std::string> radacini = extrageRadacini(continut);
    std::unordered_map<std::string, int> frecventeLocale;

    for (const auto& r : radacini) {
        frecventeLocale[r]++;
    }

    ArticolVector artVec;
    artVec.numeFisier = numeFisier;

    for (const auto& pair : frecventeLocale) {
        if (dictionarGlobal.find(pair.first) != dictionarGlobal.end()) {
            int index = dictionarGlobal[pair.first];
            artVec.vectorFrecvente[index] = pair.second;
        }
    }

    vectoriArticole.push_back(artVec);
}

void parcurgeTxt(const std::string& folder, bool doDictionar) {
    std::string cautarePath = folder + "\\*";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(cautarePath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        std::string numeElement = findData.cFileName;
        if (numeElement == "." || numeElement == "..") continue;

        std::string caleCompleta = folder + "\\" + numeElement;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            parcurgeTxt(caleCompleta, doDictionar);
        }
        else {
            std::string numeLower = numeElement;
            std::transform(numeLower.begin(), numeLower.end(), numeLower.begin(), [](unsigned char c) {
                return std::tolower(c);
                });

            if (numeLower.size() >= 4 && numeLower.substr(numeLower.size() - 4) == ".txt") {
                if (doDictionar) {
                    adaugaInDictionarGlobal(caleCompleta);
                }
                else {
                    genereazaVectorArticol(caleCompleta, numeElement);
                }
            }
        }
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);
}

int main() {
    incarcaStopWords("stopwords.txt");
    if (stopWords.empty()) {
        incarcaStopWords(".\\Lab2A\\stopwords.txt");
    }

    CreateDirectoryA(".\\rezultat_testing", NULL);
    CreateDirectoryA(".\\rezultat_training", NULL);

    std::cout << "Pasul 1: Se extrag articolele din XML in .txt...\n";
    int contorTesting = 0, contorTraining = 0;
    cautaSiProceseazaXML(".\\Testing", ".\\rezultat_testing", contorTesting);
    cautaSiProceseazaXML(".\\Training", ".\\rezultat_training", contorTraining);

    std::cout << ">> S-au extras " << contorTesting << " articole in 'rezultat_testing'.\n";
    std::cout << ">> S-au extras " << contorTraining << " articole in 'rezultat_training'.\n\n";

    std::cout << "Pasul 2: Se construieste Dictionarul Global din articole...\n";
    parcurgeTxt(".\\rezultat_testing", true);
    parcurgeTxt(".\\rezultat_training", true);

    std::ofstream fileDict("dictionar_global.txt");
    for (const auto& entry : dictionarGlobal) {
        fileDict << "Index: " << entry.second << " -> Radacina: " << entry.first << "\n";
    }
    fileDict.close();
    std::cout << ">> Dictionarul Global a fost salvat in 'dictionar_global.txt' (" << dictionarGlobal.size() << " radacini unice).\n\n";

    std::cout << "Pasul 3: Se genereaza vectorii de frecvente pentru fiecare articol...\n";
    parcurgeTxt(".\\rezultat_testing", false);
    parcurgeTxt(".\\rezultat_training", false);

    std::ofstream fileVectori("vectori_articole.txt");
    int idArticol = 1;
    for (const auto& art : vectoriArticole) {
        fileVectori << "v" << idArticol++ << " (" << art.numeFisier << "): ";
        for (const auto& pair : art.vectorFrecvente) {
            fileVectori << pair.first << ":" << pair.second << " ";
        }
        fileVectori << "\n";
    }
    fileVectori.close();
    std::cout << ">> Vectorii au fost salvati in 'vectori_articole.txt'.\n\n";

    std::cout << "Procesare complet finalizata!";
    return 0;
}