#include "Crypto.h"
#include "cryptopp/dh.h"
#include "cryptopp/osrng.h"
#include "cryptopp/integer.h"
#include "cryptopp/nbtheory.h"
#include "cryptopp/secblock.h"
#include "cryptopp/hex.h"
#include "cryptopp/filters.h"
#include "cryptopp/modes.h"
#include "cryptopp/files.h"

using namespace CryptoPP;

DH dh;
AutoSeededRandomPool prng;
SecByteBlock privateKey, publicKey, serverPublicKey, AESKey;

char* XOR(char data[]) {
	char key[] = { 'v', 'e', 'r', 'y', 's', 'e', 'c', 'r', 'e', 't', 'k', 'e', 'y' };

	for (int i = 0; i < strlen(data); i++)
		data[i] = data[i] ^ key[i % (sizeof(key) / sizeof(char))];

	std::cout << "XOR: " << data << std::endl;
	return data;
}

char* XOR(char data[], int data_len) {
	char key[] = { 'v', 'e', 'r', 'y', 's', 'e', 'c', 'r', 'e', 't', 'k', 'e', 'y' };

	for (int i = 0; i < data_len; i++)
		data[i] = data[i] ^ key[i % (sizeof(key) / sizeof(char))];

	return data;
}

void generateKeys() {
	Integer p("0xB10B8F96A080E01DDE92DE5EAE5D54EC52C99FBCFB06A3C69A6A9DCA52D23B616073E28675A23D189838EF1E2EE652C013ECB4AEA906112324975C3CD49B83BFACCBDD7D90C4BD7098488E9C219A73724EFFD6FAE5644738FAA31A4FF55BCCC0A151AF5F0DC8B4BD45BF37DF365C1A65E68CFDA76D4DA708DF1FB2BC2E4A4371");
	Integer g("0xA4D1CBD5C3FD34126765A442EFB99905F8104DD258AC507FD6406CFF14266D31266FEA1E5C41564B777E690F5504F213160217B4B01B886A5E91547F9E2749F4D7FBD7D3B9A92EE1909D0D2263F80A76A6A24C087A091F531DBF0A0169B6A28AD662A4D18E73AFA32D779D5918D08BC8858F4DCEF97C2A24855E6EEB22B3B2E5");
	Integer q("0xF518AA8781A8DF278ABA4E7D64B7CB9D49462353");

	/*PrimeAndGenerator pg;
	pg.Generate(1, prng, 1024, 160);
	
	Integer q = pg.SubPrime();*/

	dh.AccessGroupParameters().Initialize(p, q, g);
	privateKey = SecByteBlock(dh.PrivateKeyLength());
	publicKey = SecByteBlock(dh.PublicKeyLength());
	dh.GenerateKeyPair(prng, privateKey, publicKey);


	Integer k1(privateKey, privateKey.size()), k2(publicKey, publicKey.size());

	std::cout << "PUBLIC KEY:";
	std::cout << std::hex << k2 << std::endl << std::endl;
	std::cout << "PRIVATE key:";
	std::cout << std::hex << k1 << std::endl << std::endl;
}

std::string bytesToString(SecByteBlock& block) {
	Integer iBlock(block, block.size());
	std::stringstream ss;
	ss << std::hex << iBlock;
	return ss.str();
}

std::string getPublicKey() {
	return bytesToString(publicKey);
}

bool verifyServerKey(std::string stringKey) {
	std::string destination;
	StringSource ss(stringKey, true, new HexDecoder(new StringSink(destination)));

	serverPublicKey = SecByteBlock(reinterpret_cast<byte*>((char*)destination.c_str()), destination.size());
	SecByteBlock sharedSecret(dh.AgreedValueLength());

	if (!dh.Agree(sharedSecret, privateKey, serverPublicKey)) {
		std::cout << "Failed to reach shared secret." << std::endl;
		return false;
	}

	int aesKeyLength = SHA256::DIGESTSIZE;
	int defBlockSize = AES::BLOCKSIZE;

	AESKey = SecByteBlock(SHA256::DIGESTSIZE);
	SHA256().CalculateDigest(AESKey, sharedSecret, sharedSecret.size());

	Integer sKey(AESKey, AESKey.size()), sShared(sharedSecret, sharedSecret.size());
	std::cout << "SHARED SECRET: " << std::hex << sShared << std::endl;
	std::cout << "AES KEY: " << std::hex << sKey << std::endl;

	return true;
}

std::string encryptMessage(std::string plain) {
	SecByteBlock iv(AES::BLOCKSIZE);
	prng.GenerateBlock(iv, iv.size());

	std::string cipher;

	CBC_Mode<AES>::Encryption e;
	e.SetKeyWithIV(AESKey, AESKey.size(), iv);

	StringSource s(plain, true,
		new StreamTransformationFilter(e,
			new StringSink(cipher)
		)
	);

	std::stringstream ss;
	ss << std::string(reinterpret_cast<const char*>(iv.data()), iv.size()) << cipher;
	return ss.str();
}

std::string decryptMessage(std::string cipher) {
	std::string plain;

	std::string dest;
	StringSource ss(cipher, true, new HexDecoder(new StringSink(dest)));
	SecByteBlock iv(reinterpret_cast<byte*>((char*)(dest.c_str())), AES::BLOCKSIZE);
	std::string cipherBin(dest.substr(AES::BLOCKSIZE).c_str(), dest.length() - AES::BLOCKSIZE);

	std::cout << "IV: " << bytesToString(iv) << std::endl;

	CBC_Mode< AES >::Decryption d;
	d.SetKeyWithIV(AESKey, AESKey.size(), iv);

	StringSource s(cipherBin, true,
		new StreamTransformationFilter(d,
			new StringSink(plain)
		) 
	);

	std::cout << "PLAINTEXT: " << plain << std::endl;
	return plain;
}
