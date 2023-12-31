/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "se_tcrypto_common.h"
#include <openssl/sha.h>
#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include "ssl_wrapper.h"
#include "se_memcpy.h"
#include "sgx_tcrypto.h"

extern EVP_PKEY *get_priv_key_from_bin(const sgx_ec256_private_t *p_private, sgx_ecc_state_handle_t ecc_handle);
extern EVP_PKEY *get_pub_key_from_coords(const sgx_ec256_public_t *p_public, sgx_ecc_state_handle_t ecc_handle);
/* Computes signature for data based on private key
* Parameters:
*   Return: sgx_status_t - SGX_SUCCESS or failure as defined sgx_error.h
*   Inputs: sgx_ecc_state_handle_t ecc_handle - Handle to ECC crypto system
*           sgx_ec256_private_t *p_private - Pointer to the private key - LITTLE ENDIAN
*           sgx_uint8_t *p_data - Pointer to the data to be signed
*           uint32_t data_size - Size of the data to be signed
*   Output: sgx_ec256_signature_t *p_signature - Pointer to the signature - LITTLE ENDIAN  */
sgx_status_t sgx_ecdsa_sign(const uint8_t *p_data,
                            uint32_t data_size,
                            const sgx_ec256_private_t *p_private,
                            sgx_ec256_signature_t *p_signature,
                            sgx_ecc_state_handle_t ecc_handle)
{
	if ((ecc_handle == NULL) || (p_private == NULL) || (p_signature == NULL) || (p_data == NULL) || (data_size < 1))
	{
		return SGX_ERROR_INVALID_PARAMETER;
	}

	EVP_PKEY *private_key = NULL;
	EVP_MD_CTX *mctx = NULL;
	ECDSA_SIG *ecdsa_sig = NULL;
	const BIGNUM *r = NULL;
	const BIGNUM *s = NULL;
	unsigned char *sig_data = NULL;
	unsigned char *sig_tmp = NULL;
	int written_bytes = 0;
	size_t sig_size = 0;
	sgx_status_t retval = SGX_ERROR_UNEXPECTED;

	do {
		private_key = get_priv_key_from_bin(p_private, ecc_handle);
		if (!private_key) {
			break;
		}

		mctx = EVP_MD_CTX_new();
		if(!mctx) {
			break;
		}
		if(1 != EVP_DigestSignInit(mctx, NULL, EVP_sha256(), NULL, private_key)) {
			break;
		}

		if(1 != EVP_DigestSignUpdate(mctx, p_data, data_size)) {
			break;
		}
		if(1 != EVP_DigestSignFinal(mctx, NULL, &sig_size)) {
			break;
		}
		sig_data = (unsigned char*)OPENSSL_malloc(sig_size);
		if (sig_data == NULL) {
			break;
		}
		if(1 != EVP_DigestSignFinal(mctx, sig_data, &sig_size)) {
			break;
		}
		ecdsa_sig = ECDSA_SIG_new();
		if (NULL == ecdsa_sig) {
			retval = SGX_ERROR_OUT_OF_MEMORY;
			break;
		}
		sig_tmp = sig_data;
		if(NULL == d2i_ECDSA_SIG(&ecdsa_sig, (const unsigned char **)&sig_tmp, sig_size)) {
			break;
		}

		// returns internal pointers the r and s values contained in ecdsa_sig.
		ECDSA_SIG_get0(ecdsa_sig, &r, &s);

		// converts the r BIGNUM of the signature to little endian buffer, bounded with the len of out buffer
		//
		written_bytes = BN_bn2lebinpad(r, (unsigned char*)p_signature->x, SGX_ECP256_KEY_SIZE);
		if (0 >= written_bytes) {
			break;
		}

		// converts the s BIGNUM of the signature to little endian buffer, bounded with the len of out buffer
		//
		written_bytes = BN_bn2lebinpad(s, (unsigned char*)p_signature->y, SGX_ECP256_KEY_SIZE);
		if (0 >= written_bytes) {
			break;
		}

		retval = SGX_SUCCESS;
	} while(0);

	if (sig_data)
		OPENSSL_free(sig_data);
	if (ecdsa_sig)
		ECDSA_SIG_free(ecdsa_sig);
	if (private_key)
		EVP_PKEY_free(private_key);
	if (mctx)
		EVP_MD_CTX_free(mctx);

	return retval;
}

sgx_status_t sgx_ecdsa_verify(const uint8_t *p_data,
    uint32_t data_size,
    const sgx_ec256_public_t *p_public,
    const sgx_ec256_signature_t *p_signature,
    uint8_t *p_result,
    sgx_ecc_state_handle_t ecc_handle)
{
	if ((ecc_handle == NULL) || (p_public == NULL) || (p_signature == NULL) ||
		(p_data == NULL) || (data_size < 1) || (p_result == NULL)) {
		return SGX_ERROR_INVALID_PARAMETER;
	}
	unsigned char digest[SGX_SHA256_HASH_SIZE] = { 0 };

	/* generates digest of p_data */
	SHA256((const unsigned char *)p_data, data_size, (unsigned char *)digest);
	return (sgx_ecdsa_verify_hash(digest, p_public, p_signature, p_result, ecc_handle));
}


sgx_status_t sgx_ecdsa_verify_hash(const uint8_t *p_data,
                              const sgx_ec256_public_t *p_public,
                              const sgx_ec256_signature_t *p_signature,
                              uint8_t *p_result,
                              sgx_ecc_state_handle_t ecc_handle)
{
	if ((ecc_handle == NULL) || (p_public == NULL) || (p_signature == NULL) ||
		(p_data == NULL) || (p_result == NULL)) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	EVP_PKEY_CTX *ctx = NULL;
	EVP_PKEY *public_key = NULL;
	BIGNUM *bn_r = NULL;
	BIGNUM *bn_s = NULL;
	ECDSA_SIG *ecdsa_sig = NULL;
	unsigned char *sig_data = NULL;
	size_t sig_size = 0;
	sgx_status_t retval = SGX_ERROR_UNEXPECTED;
	int ret = -1;

	*p_result = SGX_EC_INVALID_SIGNATURE;

	do {
		public_key = get_pub_key_from_coords(p_public, ecc_handle);
		if(NULL == public_key) {
			break;
		}
		// converts the x value of the signature, represented as positive integer in little-endian into a BIGNUM
		//
		bn_r = BN_lebin2bn((unsigned char*)p_signature->x, sizeof(p_signature->x), 0);
		if (NULL == bn_r) {
			break;
		}

		// converts the y value of the signature, represented as positive integer in little-endian into a BIGNUM
		//
		bn_s = BN_lebin2bn((unsigned char*)p_signature->y, sizeof(p_signature->y), 0);
		if (NULL == bn_s) {
			break;
		}


		// allocates a new ECDSA_SIG structure (note: this function also allocates the BIGNUMs) and initialize it
		//
		ecdsa_sig = ECDSA_SIG_new();
		if (NULL == ecdsa_sig) {
			retval = SGX_ERROR_OUT_OF_MEMORY;
			break;
		}

		// setes the r and s values of ecdsa_sig
		// calling this function transfers the memory management of the values to the ECDSA_SIG object,
		// and therefore the values that have been passed in should not be freed directly after this function has been called
		//
		if (1 != ECDSA_SIG_set0(ecdsa_sig, bn_r, bn_s)) {
			ECDSA_SIG_free(ecdsa_sig);
			ecdsa_sig = NULL;
			break;
		}
		sig_size = i2d_ECDSA_SIG(ecdsa_sig, &sig_data);
		if (sig_size <= 0) {
			break;
		}
		ctx = EVP_PKEY_CTX_new(public_key, NULL);
		if (!ctx) {
			break;
		}
		if (1 != EVP_PKEY_verify_init(ctx)) {
			break;
		}
		if (1 != EVP_PKEY_CTX_set_signature_md(ctx, EVP_sha256())) {
			break;
		}
		ret = EVP_PKEY_verify(ctx, sig_data, sig_size, p_data, SGX_SHA256_HASH_SIZE);
		if (ret < 0) {
			break;
		}

		// sets the p_result based on verification result
		//
		if (ret == 1)
			*p_result = SGX_EC_VALID;

		retval = SGX_SUCCESS;
	} while(0);

	if (ecdsa_sig) { 
		ECDSA_SIG_free(ecdsa_sig);
		bn_r = NULL;
		bn_s = NULL;
	}
	if (ctx)
		EVP_PKEY_CTX_free(ctx);
	if (public_key)
		EVP_PKEY_free(public_key);
	if (bn_r)
		BN_clear_free(bn_r);
	if (bn_s)
		BN_clear_free(bn_s);

	return retval;
}


sgx_status_t sgx_calculate_ecdsa_priv_key(const unsigned char* hash_drg, int hash_drg_len,
	const unsigned char* sgx_nistp256_r_m1, int sgx_nistp256_r_m1_len,
	unsigned char* out_key, int out_key_len) {

	if (out_key == NULL || hash_drg_len <= 0 || sgx_nistp256_r_m1_len <= 0 ||
		out_key_len <= 0 || hash_drg == NULL || sgx_nistp256_r_m1 == NULL) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	sgx_status_t ret_code = SGX_ERROR_UNEXPECTED;
	int result_len = 0;
	BIGNUM* bn_d = NULL;
	BIGNUM* bn_m = NULL;
	BIGNUM* bn_o = NULL;
	BN_CTX* tmp_ctx = NULL;

	do {

		bn_o = BN_new();
		NULL_BREAK(bn_o);

		bn_d = BN_lebin2bn(hash_drg, hash_drg_len, bn_d);
		BN_CHECK_BREAK(bn_d);

		bn_m = BN_lebin2bn(sgx_nistp256_r_m1, sgx_nistp256_r_m1_len, bn_m);
		BN_CHECK_BREAK(bn_m);

		tmp_ctx = BN_CTX_new();
		NULL_BREAK(tmp_ctx);

		if (!BN_mod(bn_o, bn_d, bn_m, tmp_ctx)) {
			break;
		}

		if (!BN_add_word(bn_o, 1)) {
			break;
		}

		result_len = BN_num_bytes(bn_o);
		if ((result_len < 0) || (out_key_len < result_len)) {
			break;
		}
		if (BN_bn2bin(bn_o, out_key) != result_len) {
			break;
		}

		ret_code = SGX_SUCCESS;
	} while (0);

	//clear and free used structs
	//
	BN_CTX_free(tmp_ctx);
	BN_clear_free(bn_d);
	BN_clear_free(bn_m);
	BN_clear_free(bn_o);

	if (ret_code != SGX_SUCCESS) {
		(void)memset_s(out_key, out_key_len, 0, out_key_len);
	}

	return ret_code;
}
