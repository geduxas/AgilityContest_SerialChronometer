//
// Created by jantonio on 13/07/19.
//

#include <sys/stat.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/md5.h>
#include <openssl/bio.h>
#include <stdio.h>
#include <string.h>

#define SERIALCHRONOMETER_LICENSE_C

#include "sc_config.h"
#include "debug.h"
#include "license.h"

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

//Decodes a base64 encoded file
static char * base64Decode(char* fname,size_t *datalen) {
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
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Do not use newlines to flush buffer
    size_t len = BIO_read(bio, buffer, size);
    //Can test here if len == decodeLen - if not, then return an error
    *(buffer+len) = '\0'; // not really needed, but...
    *datalen=len;
    BIO_free_all(bio);
    fclose(stream);
    return (buffer); //success
}

static char *decryptLicense(char *data,size_t datalen, RSA *puk) {
    char *result=calloc(RSA_size(puk),sizeof(char));
    int resultlen=0;
    // data is encrypted in 1024 bytes blocks ( as for 8192bit key )
    // so divide incomming data in 1K chunks and decrypt then
    char buff[RSA_size(puk)-11]; // to store chunk decrypts
    for (char *pt=data;pt<data+datalen;pt+=1024) {
        int nbytes = RSA_public_decrypt(1024,(unsigned char *)pt,(unsigned char *) buff,puk,RSA_PKCS1_PADDING);
        if (nbytes<0) {
            debug(DBG_ERROR,"decryptLicense() failed");
            return NULL;
        }
        result=realloc(result,datalen+nbytes);
        memcpy(result+resultlen,buff,1+nbytes);
        resultlen+=nbytes;
    }
    result[resultlen]='\0';
    return result;
}

int readLicenseFromFile(configuration *config) {
    size_t len=0;
    // try to locate license file where config says
    char *data=base64Decode(config->license_file,&len);
    // else look in current directory
    if (!data) data=base64Decode("registration.info",&len);
    // finally try in AgilityContest std location
    if (!data) data=base64Decode(LICENSE_FILE,&len);
    if (!data) return -1;
    // retrieve public key
    RSA *puk=getPublicKey();
    // decrypt license
    license_data=decryptLicense(data,len,puk);
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
    debug(DBG_ERROR,"License file has empty logo data");
    char *fname="html/AgilityContest.png";
    struct stat st;
    // check file
    if (!fname) return error_null("filename is null","");
    if (stat(fname, &st) == -1)  return error_null("%s does not exists",fname);
    if ( ! S_ISREG(st.st_mode) ) return error_null("%s is not a regular file",fname);

    // load file into memory
    data=calloc(st.st_size,sizeof(char));
    if (!data) return error_null("Cannot allocate space for file %s",fname);
    FILE *from=fopen(fname,"rb");
    if (!from) return error_null("Error opening file %s",fname);
    fread(data,st.st_size,sizeof(char),from);
    fclose(from);
    *size=st.st_size;
    return data;
}