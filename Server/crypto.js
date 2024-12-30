const crypto = require('crypto');

const p = Buffer.from("B10B8F96A080E01DDE92DE5EAE5D54EC52C99FBCFB06A3C69A6A9DCA52D23B616073E28675A23D189838EF1E2EE652C013ECB4AEA906112324975C3CD49B83BFACCBDD7D90C4BD7098488E9C219A73724EFFD6FAE5644738FAA31A4FF55BCCC0A151AF5F0DC8B4BD45BF37DF365C1A65E68CFDA76D4DA708DF1FB2BC2E4A4371", 'hex');
const g = Buffer.from("A4D1CBD5C3FD34126765A442EFB99905F8104DD258AC507FD6406CFF14266D31266FEA1E5C41564B777E690F5504F213160217B4B01B886A5E91547F9E2749F4D7FBD7D3B9A92EE1909D0D2263F80A76A6A24C087A091F531DBF0A0169B6A28AD662A4D18E73AFA32D779D5918D08BC8858F4DCEF97C2A24855E6EEB22B3B2E5", 'hex');
const dh = crypto.createDiffieHellman(p, g);
dh.generateKeys();

const getPublicKey = () => dh.getPublicKey('hex');

const generateSecret = (implantPublicKey) => {
    const secret = dh.computeSecret(implantPublicKey, 'hex');
    const aesKey = crypto.createHash('sha256').update(secret).digest();
    return aesKey;
}

const encrypt = (text, aesKey) => {
    const iv = crypto.randomBytes(16);
    const encrypter = crypto.createCipheriv("aes-256-cbc", aesKey, iv);
    let cipherBin = encrypter.update(text);
    return Buffer.concat([iv, cipherBin, encrypter.final()]).toString('hex');
}

const decrypt = (bin, aesKey) => {
    const iv = bin.slice(0, 16);
    const cipherText = bin.slice(16);
    const decrypter = crypto.createDecipheriv("aes-256-cbc", aesKey, iv);
    let plainText = decrypter.update(cipherText, outputEncoding = "utf8");
    plainText += decrypter.final("utf8");
    return plainText;
}

const encryptJSON = (json, aesKey) => encrypt(JSON.stringify(json), aesKey);
const decryptJSON = (bin, aesKey) => JSON.parse(decrypt(Buffer.from(bin), aesKey));

module.exports = {
    getPublicKey,
    generateSecret,
    encryptJSON,
    decryptJSON,
    encrypt,
    decrypt
}