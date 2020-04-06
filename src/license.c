//
// Created by jantonio on 13/07/19.
//

#include <sys/stat.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/md5.h>
#include <openssl/bio.h>
#include <openssl/aes.h>
#include <openssl/hmac.h>
#include <stdio.h>
#include <string.h>

#define SERIALCHRONOMETER_LICENSE_C

#include "sc_config.h"
#include "sc_tools.h"
#include "debug.h"
#include "license.h"
#include "ini.h"

static char *license_data=NULL;

static char *getPK() {
     char *str=
    "MIIEIjANBgkqhkiG9w0BAQEFAAOCBA8AMIIECgKCBAEAzgeD27TXHKde3iNMtQSq\n"
    "yAFoeZYVOoPPjGQkFcNamxfGR8rFmgvGrJn28u2bq1dVnIduF9Lj4sPMt9cs/rT+\n"
    "IOnFD3uACZbqku+e39gyrKyu7aZ2t+XTR4IUFKhGYuwr1TRmb/46iXJF7V7ZU9ci\n"
    "q5/A7vCU8XJ9IyInd52yFeSgZdVl4TB/+qqlVJNiyvfpxwHakx/qM+JqjsgXoS+B\n"
    "6Xr9ZChtTh5gljGMxmLQlLLnScwZ7Ku7pnYkZnP/Nb2jT0gaNHBM1af1mpdwpDRD\n"
    "3igilVchrydcknKoF1LRbApgoIbLbzX9QufiJbfKA3ZWtB4c6YL8sSbgIgeHDguS\n"
    "i0obvVFESynMs4WKYtzIIJZEw3G7jXtVHPLoJ56udAT0Moxw6oiVhdGvSejKz/Ik\n"
    "95uPYRMOnlOle+y18UVduHuudKPqoCKFmnub+z8eVLm/aP1SHBpo1l87tDM3w3aT\n"
    "tHwkmAGhQn+udRMCrb0f897XlR7ReNdwppBLLmLR5sdWpAIjpNatw4N2YJG3uehf\n"
    "rHYwlIYovjHVZUbz8D0NxUVJHXv99QB9f37D6D40aiYSA5YnHgkKCq9dkSHeq7GU\n"
    "79fzrCEUihx2d2Nn7tLC3rR/45XI5WU7T61R0N6PPAl8slrOOEnYBs5YlSLH8Sdt\n"
    "Pi4f2+65qK8jVM1fTly4YOvsuA8mPtimTAgMBa0ys0I9RLezwmlbQbXoYwrx7avC\n"
    "SjAe7o3i8g8uGEW+WTPsT87KZdJKRrlr48lMTnonuAbU59phK+b8IAVS3q6rHOyc\n"
    "uwtPRofqBh9Qok3PYYf0Tobc3R8OKi8cX1rXjPGodxFQfcINIAlmLEJxWyMJlM8l\n"
    "rD33czqk2Opx1kYmjkU7zn1x046w0IaUgZqrBCO6K0qMsrfV0VfHf4lAn3WGDY+h\n"
    "2oZlQqmExBcEdLu2aeNwTAq8G1zwS4atF62r2uR4ZEBJNxWfM32kDHKDEEg8selk\n"
    "iD8lxxyGvUgc0/6EboY2JoN0n8QTJbJzC57cqYhYSxh7FekAGZ+xAxA5Ujy+e16H\n"
    "MBKVhRbAj5+Dk803kvej+F6mOx69VekeEVr3C0xlzMslI7wvXY+IZHD+EgLgTFaO\n"
    "TY5ilQ7AjX4id7cXiUHvPpkYsxr9A+ImM3JzdjFkWaKAwmPK6rTbtxA4j86dMktF\n"
    "bKuGHf0lnGPLLNIVrBYfUXa+rdvBlxuZ6dLDaHoq3+3AH0s+5+b3HJCYLY/VMSgV\n"
    "qwCr9Xj8P28JFYziX0aic/0O7Q5Hkp39I7PikB4EJB73NUaYd/UK8EJ1c5zSz2tI\n"
    "LHN0iN/VSKutDHrfZ0om7krDSEyY6TZ/rVDewnFQmbIiIORgig7mjH0EXBUiXBJF\n"
    "VQIDAQAB\n";
    return str;
}

char *md5sum(const char *str, int length) {
    int n;
    MD5_CTX c;
    unsigned char digest[16];
    char *out = (char*)calloc(33,sizeof(char));
    MD5_Init(&c);
    while (length > 0) {
        if (length > 512)  MD5_Update(&c, str, 512);
        else  MD5_Update(&c, str, length);
        length -= 512;
        str += 512;
    }
    MD5_Final(digest, &c);
    for (n = 0; n < 16; ++n)  snprintf(&(out[n*2]), 16*2, "%02x", (unsigned int)digest[n]);
    return out;
}


static RSA *getPublicKey() {

    // compose public key
    char *hdr = "-----BEGIN PUBLIC KEY-----\n";
    char *pk = getPK();
    char *tail = "-----END PUBLIC KEY-----\n";
    char *md5 = "ff430f62f2e112d176110b704b256542";
    char *pkstr = calloc(16 + strlen(hdr) + strlen(pk) + strlen(tail), sizeof(char));
    if (!pkstr) {
        debug(DBG_ERROR,"getPublicKey() cannot allocate space for read puk");
        return NULL;
    }
    sprintf(pkstr, "%s%s%s", hdr, pk, tail);

    // check md5 of composed key
    char *sum = md5sum(pkstr,strlen(pkstr));
    if (strcmp(md5, sum) != 0) { // invalid md5 sum
        debug(DBG_ERROR,"getPublicKey() public key md5 sum missmatch: '%s' expected '%s",sum,md5);
        return NULL;
    }
    // ok. no data corruption load pk into openssl rsa struct
    BIO *bio = BIO_new_mem_buf((void *) pkstr, -1);
    if (!bio) { // error creating basic i/o data
        debug(DBG_ERROR,"getPublicKey() cannot create BIO data");
        return NULL;
    }
    RSA *rsa = PEM_read_bio_RSA_PUBKEY(bio, 0, 0, 0);
    if (!rsa) { // readed data is not a public RSA key
        debug(DBG_ERROR,"getPublicKey() data is not a valid public key");
        return NULL;
    }
    // cleanup and return
    BIO_free(bio);
    return rsa;
}

static char *error_null(char *format,const char *msg) {
    debug(DBG_ERROR,format,msg);
    return NULL;
}

// base 64 encode provided file
// returns encoded data or null on error
static char *base64Encode(const char* fname) {
    struct stat st;
    // check file
    if (!fname) return error_null("filename is null","");
    if (stat(fname, &st) == -1)  return error_null("%s does not exists",fname);
    if ( ! S_ISREG(st.st_mode) ) return error_null("%s is not a regular file",fname);

    // load file into memory
    char *data=calloc(st.st_size,sizeof(char));
    if (!data) return error_null("Cannot allocate space for file %s",fname);
    FILE *from=fopen(fname,"rb");
    if (!from) return error_null("Error opening file %s",fname);
    fread(data,st.st_size,sizeof(char),from);
    fclose(from);

    // perform encoding operation
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Ignore newlines - write everything in one line
    BIO_write(bio, data, st.st_size);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);
    BIO_set_close(bio, BIO_NOCLOSE);
    BIO_free_all(bio);
    return bufferPtr->data;
}

static char *decryptLicense(const unsigned char *data,size_t datalen, const unsigned char *uniqueID, RSA *puk) {

    // procedemos al descifrado simétrico a partir de la uniqueid
    /* extract hash_hmac to get hash and enc/dec key*/
    const char *e="ENCRYPTION";
    const char *a="AUTHENTICATION";
    unsigned char enc[32];
    unsigned int enc_len;
    unsigned char auth[32];
    unsigned int auth_len;
    HMAC(EVP_sha256(), uniqueID, strlen((char *)uniqueID), e, strlen((char *)e), enc, &enc_len);
    HMAC(EVP_sha256(), uniqueID, strlen((char *)uniqueID), a, strlen((char *)a), auth, &auth_len);

    // remember dat message has the format $hash.$iv.$message

    // debug(DBG_TRACE,"enc: '%s' len:%d",hexdump(enc,enc_len),enc_len);
    // debug(DBG_TRACE,"auth: '%s' len:%d",hexdump(auth,auth_len),auth_len);

    // PENDING: verify hash
    // skip hash from encoded data
    data += auth_len; // sha256 hmac size is 256 bits (32 bytes)
    datalen -= auth_len;

    /* symmetric decrypt buffer with extracted encryption/decryption key */
    unsigned char iv[AES_BLOCK_SIZE]; // init vector
    memcpy(iv, data, AES_BLOCK_SIZE);
    // debug(DBG_TRACE,"iv: '%s' len:%d",hexdump(iv,AES_BLOCK_SIZE),AES_BLOCK_SIZE);

    data += AES_BLOCK_SIZE; // 16 bytes for aes_256_cbc
    datalen -= AES_BLOCK_SIZE;
    unsigned char *dec_out=calloc(datalen, sizeof(char));

    /* prepare cipher */
    EVP_CIPHER_CTX *cctx =EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(cctx, EVP_aes_256_cbc(), NULL, enc, iv);
    int len,ptlen;

    EVP_DecryptUpdate(cctx, dec_out, &len, data, datalen);
    ptlen=len;
    EVP_DecryptFinal_ex(cctx, dec_out + len, &len);
    ptlen+=len;
    EVP_CIPHER_CTX_free(cctx);
    // debug(DBG_TRACE,"symm_dec: '%s' len:%d",hexdump(dec_out,32),len);

    // now perform RSA decryption of resulted data
    char *result=calloc(RSA_size(puk),sizeof(char));
    int resultlen=0;

    // data is encrypted in 1024 bytes blocks ( as for 8192bit key )
    // so divide incomming data in 1K chunks and decrypt then
    char buff[RSA_size(puk)-11]; // to store chunk decrypts
    // debug(DBG_INFO,"chunk buffer size is:%d",RSA_size(puk)-11);
    for (unsigned char *pt=dec_out;pt<dec_out+ptlen;pt+=1024) {
        int nbytes = RSA_public_decrypt(1024,(unsigned char *)pt,(unsigned char *) buff,puk,RSA_PKCS1_PADDING);
        if (nbytes<0) {
            debug(DBG_ERROR,"decryptLicense() failed");
            return NULL;
        }
        result=realloc(result,datalen+nbytes);
        // debug(DBG_TRACE,"decrypted %d bytes: %s",nbytes,hexdump(buff,nbytes));
        memcpy(result+resultlen,buff,1+nbytes);
        resultlen+=nbytes;
    }
    result[resultlen]='\0';
    debug(DBG_INFO,"Decrypted license is: %s len:%d",result,resultlen);
    return result;
}

//Decodes a base64 encoded file
static char * base64DecodeFile(char* fname,size_t *datalen) {
    BIO *bio, *b64;
    struct stat st;
    if (!fname) return NULL;
    if (stat(fname, &st) == -1)  return NULL; // not valid fname
    if ( ! S_ISREG(st.st_mode) ) return NULL; // not regular file
    size_t size=1+0.75*st.st_size;
    char *buffer = (char*)calloc(size,sizeof(char));
    FILE* stream = fopen(fname, "r");
    if(!stream) return NULL;
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_fp(stream, BIO_NOCLOSE);
    bio = BIO_push(b64, bio);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); //Do not use newlines to flush buffer
    size_t index=0;
    size_t len=0;
    while ((len=BIO_read(b64, buffer+index, 1024))>0) index+=len;
    *datalen=index;
    BIO_free_all(b64);
    fclose(stream);
    return (buffer); //success
}

// from https://stackoverflow.com/questions/342409/how-do-i-base64-encode-decode-in-c
/**
 * decode a base64 encoded string extracting up to maxlen bytes
 * @param from base64 encoded string
 * @param maxlen maximun data to extract
 * @return extracted data
 */
static char *base64DecodeString (const void *input, size_t *datalen){
    //Declares two OpenSSL BIOs: a base64 filter and a memory BIO.
    BIO *b64,*bio;
    size_t size=1+(3*strlen(input))/4;
    char *buffer = calloc( size, sizeof(char) );
    b64 = BIO_new(BIO_f_base64());          // Initialize our base64 filter BIO.
    bio = BIO_new_mem_buf(input, -1);  // Initialize our memory source BIO.
    BIO_push(b64, bio);                     // Link the BIOs by creating a filter-source BIO chain.
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); // Don't require trailing newlines.
    size_t index=0;
    size_t len=0;
    while ((len=BIO_read(b64, buffer+index, 1024))>0) index+=len;
    *datalen=index;
    BIO_free_all(b64);               // Destroys all BIOs in chain, starting with b64 (i.e. the 1st one).
    return buffer;                   // Returns base-64 decoded data with trailing null terminator.
}

/**
 * ini_parse() handler to extract unique ID from system.ini AgilityContest file
 */
static int sysini_handler(void * data, const char* section, const char* name, const char* value) {
    char **result=data;
    if (strcmp("uniqueID",name)==0) *result=str_replace(value,"\"","");
    return 1;
}

/**
 * extract unique id from AgilityContest's system.ini <fname>
 * @param fname file name. may be null
 * @return evaluated UniqueID or DEFAULT_UniqueID on error ( no file, or no tag found)
 */
static char *retrieveUniqueID(char *fname) {
    if (!fname) return DEFAULT_UniqueID;
    fname=str_replace(fname,"registration.info","system.ini");
    char *result=NULL;
    if (ini_parse(fname, sysini_handler, &result) < 0) {
        debug(DBG_ERROR,"Can't retrieve UniqueID from system.ini file '%s'",fname);
        return DEFAULT_UniqueID;
    }
    // stored data is base64 encoded, so decode and return
    size_t result_len=0;
    char *ret=base64DecodeString(result,&result_len);
    if (ret) *(ret+result_len)='\0';
    // debug(DBG_TRACE,"UniqueID from '%s' is '%s' --> '%s'",fname,result,ret);
    free(result);
    return ret;
}

int readLicenseFromFile(configuration *config) {
    size_t len=0;
    // try to locate license file where config says ( may be null, no need to check )
    // also locate unique id to check hash
    char *data=base64DecodeFile(config->license_file,&len);
    char *uniqueid=retrieveUniqueID(config->license_file); // internal str_replace to system.ini
    // else look in AgilityContest std location
    if (!data) {
        data=base64DecodeFile(LICENSE_FILE,&len);
        uniqueid=retrieveUniqueID(LICENSE_FILE); // internal str_replace to system.ini
    }
    // finally try in current directory
    if (!data) {
        data=base64DecodeFile("registration.info",&len);
        uniqueid=retrieveUniqueID(NULL);
    }
    if (!data) { debug(DBG_ERROR,"Cannot read license file"); return -1; }
    if (!uniqueid) { debug(DBG_ERROR,"Cannot retrieve uniqueID"); return -1; }
    // retrieve public key
    RSA *puk=getPublicKey();
    // decrypt license
    license_data=decryptLicense(data,len,uniqueid,puk);
    if (!license_data) return -1;
    return strlen(license_data);
}

char *getLicenseItem(char *item) {
    char searchkey[64];
    if (!license_data) return NULL;
    snprintf(searchkey,64,"\"%s\" : \"",item);
    char *pt1=strstr(license_data,searchkey);
    if (!pt1) return NULL; // no "key" : " found
    pt1+=strlen(item)+6;
    char *pt2=strchr(pt1,'"');
    if (!pt2) return NULL; // no closing quotes
    char *result=calloc(1+pt2-pt1,sizeof(char));
    if (!result) return NULL;
    memcpy(result,pt1,pt2-pt1);
    return result;
}

/**
 * get logo from license (base 64 encoded )
 * if license has no logo use default and mark it unencoded
 */
char *getLicenseLogo(size_t *size) {
    // PENDING: should be base64decode'd ?
    char *data= getLicenseItem("image");
    if (data && strlen(data)>0) return data;

    // not found in license: use the one existing in html tree
    debug(DBG_ERROR,"License file has empty logo data");
    char *fname="html/AgilityContest.png";
    struct stat st;
    // check file
    if (stat(fname, &st) == -1)  return error_null("Logo file '%s' does not exists",fname);
    if ( ! S_ISREG(st.st_mode) ) return error_null("Logo file '%s' is not a regular file",fname);

    // load file into memory
    data=calloc(st.st_size,sizeof(char));
    if (!data) return error_null("Cannot allocate space for Logo file '%s'",fname);
    FILE *from=fopen(fname,"rb");
    if (!from) return error_null("Error opening Logo file '%s'",fname);
    fread(data,st.st_size,sizeof(char),from);
    fclose(from);
    *size=st.st_size;
    return data;
}