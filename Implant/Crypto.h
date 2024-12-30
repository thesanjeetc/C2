#pragma once
#include <string.h>
#include <iostream>

void generateKeys();
std::string getPublicKey();
bool verifyServerKey(std::string stringKey);
std::string encryptMessage(std::string plain);
std::string decryptMessage(std::string cipher);
char* XOR(char input[]);
char* XOR(char data[], int data_len);