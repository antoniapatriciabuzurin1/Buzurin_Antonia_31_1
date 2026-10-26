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

using namespace std;

struct ArticolVector {
    string numeFisier;
    map<int, int> vectorFrecvente;
};

unordered_set<string> stopWords;
map<string, int> dictionarGlobal;
vector<ArticolVector> vectoriArticole;

void incarcaStopWords(const string& caleFisier) {
    ifstream file(caleFisier);
    if (!file.is_open()) return;
    string cuvant;
    while (file >> cuvant) {
        transform(cuvant.begin(), cuvant.end(), cuvant.begin(), [](unsigned char c) {
            return tolower(c);
            });
        stopWords.insert(cuvant);
    }
    file.close();
}

string inlocuiesteEntitatiXML(string text) {
    size_t pos;
    while ((pos = text.find("&quot;")) != string::npos) text.replace(pos, 6, "\"");
    while ((pos = text.find("&amp;")) != string::npos) text.replace(pos, 5, "&");
    while ((pos = text.find("&lt;")) != string::npos) text.replace(pos, 4, "<");
    while ((pos = text.find("&gt;")) != string::npos) text.replace(pos, 4, ">");
    while ((pos = text.find("&apos;")) != string::npos) text.replace(pos, 6, "'");
    return text;
}

string eliminaTaguriXML(const string& text) {
    string rezultat = "";
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

string obtineRadacina(const string& cuvant) {
    string s = cuvant;
    if (s.length() <= 3) return s;

    if (s.length() > 4 && s.substr(s.length() - 3) == "ing") return s.substr(0, s.length() - 3);
    if (s.length() > 3 && s.substr(s.length() - 2) == "ed") return s.substr(0, s.length() - 2);
    if (s.length() > 3 && s.substr(s.length() - 2) == "es") return s.substr(0, s.length() - 2);
    if (s.length() > 3 && s.back() == 's' && s[s.length() - 2] != 's') return s.substr(0, s.length() - 1);

    return s;
}

string curataSiFiltreazaText(const string& text) {
    string textFaraEntitati = inlocuiesteEntitatiXML(text);
    string faraTaguri = eliminaTaguriXML(textFaraEntitati);
    string textCurat = "";

    for (char ch : faraTaguri) {
        unsigned char uc = static_cast<unsigned char>(ch);
        if (isalnum(uc) || isspace(uc)) {
            textCurat += static_cast<char>(tolower(uc));
        }
        else {
            textCurat += ' ';
        }
    }

    stringstream ss(textCurat);
    string cuvant;
    string rezultat = "";

    while (ss >> cuvant) {
        if (stopWords.find(cuvant) == stopWords.end() && cuvant.length() > 1) {
            rezultat += cuvant + " ";
        }
    }

    return rezultat;
}

string extrageEticheta(const string& continut, const string& eticheta) {
    string continutLower = continut;
    transform(continutLower.begin(), continutLower.end(), continutLower.begin(), [](unsigned char c) {
        return tolower(c);
        });

    string tagStart = "<" + eticheta;
    string tagEnd = "</" + eticheta + ">";

    size_t posStart = continutLower.find(tagStart);
    if (posStart == string::npos) return "";

    size_t posStartClose = continutLower.find(">", posStart);
    if (posStartClose == string::npos) return "";

    size_t posEnd = continutLower.find(tagEnd, posStartClose);
    if (posEnd == string::npos || posEnd <= posStartClose) return "";

    size_t dataStart = posStartClose + 1;
    size_t dataLen = posEnd - dataStart;

    return continut.substr(dataStart, dataLen);
}

string getNumeFisierFaraExtensie(const string& numeFisier) {
    size_t lastDot = numeFisier.find_last_of(".");
    if (lastDot == string::npos) return numeFisier;
    return numeFisier.substr(0, lastDot);
}

vector<string> extrageRadacini(const string& text) {
    vector<string> radacini;
    stringstream ss(text);
    string cuvant;
    while (ss >> cuvant) {
        if (cuvant != "titlu:" && cuvant != "text:") {
            radacini.push_back(obtineRadacina(cuvant));
        }
    }
    return radacini;
}

void proceseazaFisierXML(const string& caleXml, const string& folderDestinatie, const string& numeFisier) {
    ifstream fileIn(caleXml);
    if (!fileIn.is_open()) return;

    string continut((istreambuf_iterator<char>(fileIn)), istreambuf_iterator<char>());
    fileIn.close();

    string titluBrut = extrageEticheta(continut, "title");
    string textBrut = extrageEticheta(continut, "text");
    if (textBrut.empty()) {
        textBrut = extrageEticheta(continut, "body");
    }

    string titluFiltrat = curataSiFiltreazaText(titluBrut);
    string textFiltrat = curataSiFiltreazaText(textBrut);

    string caleTxt = folderDestinatie + "\\" + getNumeFisierFaraExtensie(numeFisier) + ".txt";
    ofstream fileOut(caleTxt);

    if (fileOut.is_open()) {
        fileOut << "titlu: " << titluFiltrat << "\n";
        fileOut << "text: " << textFiltrat << "\n";
        fileOut.close();
    }
}

void cautaSiProceseazaXML(const string& folderCurent, const string& folderDestinatie, int& contor) {
    string cautarePath = folderCurent + "\\*";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(cautarePath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        string numeElement = findData.cFileName;
        if (numeElement == "." || numeElement == "..") continue;

        string caleCompleta = folderCurent + "\\" + numeElement;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (numeElement != "rezultat_testing" && numeElement != "rezultat_training" &&
                numeElement != "Debug" && numeElement != "Release" && numeElement != ".vs") {
                cautaSiProceseazaXML(caleCompleta, folderDestinatie, contor);
            }
        }
        else {
            string numeLower = numeElement;
            transform(numeLower.begin(), numeLower.end(), numeLower.begin(), [](unsigned char c) {
                return tolower(c);
                });

            if (numeLower.size() >= 4 && numeLower.substr(numeLower.size() - 4) == ".xml") {
                proceseazaFisierXML(caleCompleta, folderDestinatie, numeElement);
                contor++;
            }
        }
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);
}

void adaugaInDictionarGlobal(const string& caleTxt) {
    ifstream file(caleTxt);
    if (!file.is_open()) return;

    string continut((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    vector<string> radacini = extrageRadacini(continut);
    for (const auto& r : radacini) {
        if (dictionarGlobal.find(r) == dictionarGlobal.end()) {
            int noulIndex = static_cast<int>(dictionarGlobal.size());
            dictionarGlobal[r] = noulIndex;
        }
    }
}

void genereazaVectorArticol(const string& caleTxt, const string& numeFisier) {
    ifstream file(caleTxt);
    if (!file.is_open()) return;

    string continut((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    vector<string> radacini = extrageRadacini(continut);
    unordered_map<string, int> frecventeLocale;

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

void parcurgeTxt(const string& folder, bool doDictionar) {
    string cautarePath = folder + "\\*";
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(cautarePath.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        string numeElement = findData.cFileName;
        if (numeElement == "." || numeElement == "..") continue;

        string caleCompleta = folder + "\\" + numeElement;

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            parcurgeTxt(caleCompleta, doDictionar);
        }
        else {
            string numeLower = numeElement;
            transform(numeLower.begin(), numeLower.end(), numeLower.begin(), [](unsigned char c) {
                return tolower(c);
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

    int contorTesting = 0, contorTraining = 0;
    cautaSiProceseazaXML(".\\Testing", ".\\rezultat_testing", contorTesting);
    cautaSiProceseazaXML(".\\Training", ".\\rezultat_training", contorTraining);

    cout << ">> S-au extras " << contorTesting << " articole in 'rezultat_testing'.\n";
    cout << ">> S-au extras " << contorTraining << " articole in 'rezultat_training'.\n\n";
    parcurgeTxt(".\\rezultat_testing", true);
    parcurgeTxt(".\\rezultat_training", true);

    ofstream fileDict("dictionar_global.txt");
    for (const auto& entry : dictionarGlobal) {
        fileDict << "Index: " << entry.second << " -> Radacina: " << entry.first << "\n";
    }
    fileDict.close();
    cout << ">> Dictionarul Global a fost salvat in 'dictionar_global.txt' (" << dictionarGlobal.size() << " radacini unice).\n\n";
    parcurgeTxt(".\\rezultat_testing", false);
    parcurgeTxt(".\\rezultat_training", false);

    ofstream fileVectori("vectori_articole.txt");
    int idArticol = 1;
    for (const auto& art : vectoriArticole) {
        fileVectori << "v" << idArticol++ << " (" << art.numeFisier << "): ";
        for (const auto& pair : art.vectorFrecvente) {
            fileVectori << pair.first << ":" << pair.second << " ";
        }
        fileVectori << "\n";
    }
    fileVectori.close();
    cout << ">> Vectorii au fost salvati in 'vectori_articole.txt'.\n\n";
    return 0;
}