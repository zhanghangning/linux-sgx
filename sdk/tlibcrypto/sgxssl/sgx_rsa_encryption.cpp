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

/**
* File:
*     sgx_rsa_encryption.cpp
* Description:
*     Wrapper for rsa operation functions
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sgx_error.h"
#include "sgx_tcrypto.h"
#include "se_tcrypto_common.h"
#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/core_names.h>
#include <openssl/param_build.h>
#include "ssl_wrapper.h"

sgx_status_t sgx_create_rsa_key_pair(int n_byte_size, int e_byte_size, unsigned char *p_n, unsigned char *p_d, unsigned char *p_e,
	unsigned char *p_p, unsigned char *p_q, unsigned char *p_dmp1,
	unsigned char *p_dmq1, unsigned char *p_iqmp)
{
	if (n_byte_size <= 0 || e_byte_size <= 0 || p_n == NULL || p_d == NULL || p_e == NULL ||
		p_p == NULL || p_q == NULL || p_dmp1 == NULL || p_dmq1 == NULL || p_iqmp == NULL) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	sgx_status_t ret_code = SGX_ERROR_UNEXPECTED;
	EVP_PKEY* pkey = NULL;
	EVP_PKEY_CTX* pkey_ctx = NULL;
	BIGNUM* bn_n = NULL;
	BIGNUM* bn_e = NULL;
	BIGNUM* tmp_bn_e = NULL;
	BIGNUM* bn_d = NULL;
	BIGNUM* bn_dmp1 = NULL;
	BIGNUM* bn_dmq1 = NULL;
	BIGNUM* bn_iqmp = NULL;
	BIGNUM* bn_q = NULL;
	BIGNUM* bn_p = NULL;

	do {
		//create new rsa ctx
		//
		pkey_ctx = EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL);
		if (pkey_ctx == NULL) {
			ret_code = SGX_ERROR_OUT_OF_MEMORY;
			break;
		}
		if (EVP_PKEY_keygen_init(pkey_ctx) <= 0) {
			break;
		}

		//generate rsa key pair, with n_byte_size*8 mod size and p_e exponent
		//
		tmp_bn_e = BN_lebin2bn(p_e, e_byte_size, tmp_bn_e);
		BN_CHECK_BREAK(tmp_bn_e);
		if (EVP_PKEY_CTX_set_rsa_keygen_bits(pkey_ctx, n_byte_size * 8) <= 0) {
		        break;
		}
		if (EVP_PKEY_CTX_set1_rsa_keygen_pubexp(pkey_ctx, tmp_bn_e) <= 0) {
		        break;
		}
		if (EVP_PKEY_generate(pkey_ctx, &pkey) <= 0) {
			break;
		}

		//get RSA key internal values
		//
		if (EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_N, &bn_n) == 0) {
                    break;
		}
		if (EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_E, &bn_e) == 0) {
                    break;
		}
		if (EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_D, &bn_d) == 0) {
                    break;
		}
		if (EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_FACTOR1, &bn_p) == 0) {
                    break;
		}
		if (EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_FACTOR2, &bn_q) == 0) {
                    break;
		}
		if (EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_EXPONENT1, &bn_dmp1) == 0) {
                    break;
		}
		if (EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_EXPONENT2, &bn_dmq1) == 0) {
                    break;
		}
		if (EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_COEFFICIENT1, &bn_iqmp) == 0) {
                    break;
		}

		//copy the generated key to input pointers
		//
		if (!BN_bn2lebinpad(bn_n, p_n, BN_num_bytes(bn_n)) ||
			!BN_bn2lebinpad(bn_d, p_d, BN_num_bytes(bn_d)) ||
			!BN_bn2lebinpad(bn_e, p_e, BN_num_bytes(bn_e)) ||
			!BN_bn2lebinpad(bn_p, p_p, BN_num_bytes(bn_p)) ||
			!BN_bn2lebinpad(bn_q, p_q, BN_num_bytes(bn_q)) ||
			!BN_bn2lebinpad(bn_dmp1, p_dmp1, BN_num_bytes(bn_dmp1)) ||
			!BN_bn2lebinpad(bn_dmq1, p_dmq1, BN_num_bytes(bn_dmq1)) ||
			!BN_bn2lebinpad(bn_iqmp, p_iqmp, BN_num_bytes(bn_iqmp))) {
			break;
		}

		ret_code = SGX_SUCCESS;
	} while (0);

	//free rsa ctx (RSA_free also free related BNs obtained in RSA_get functions)
	//
	EVP_PKEY_CTX_free(pkey_ctx);
	EVP_PKEY_free(pkey);
	BN_clear_free(tmp_bn_e);
	BN_clear_free(bn_n);
	BN_clear_free(bn_d);
	BN_clear_free(bn_e);
	BN_clear_free(bn_p);
	BN_clear_free(bn_q);
	BN_clear_free(bn_dmp1);
	BN_clear_free(bn_dmq1);
	BN_clear_free(bn_iqmp);

	return ret_code;
}

sgx_status_t sgx_create_rsa_priv2_key(int mod_size, int exp_size, const unsigned char *p_rsa_key_e, const unsigned char *p_rsa_key_p, const unsigned char *p_rsa_key_q,
	const unsigned char *p_rsa_key_dmp1, const unsigned char *p_rsa_key_dmq1, const unsigned char *p_rsa_key_iqmp,
	void **new_pri_key2)
{
	if (mod_size <= 0 || exp_size <= 0 || new_pri_key2 == NULL ||
		p_rsa_key_e == NULL || p_rsa_key_p == NULL || p_rsa_key_q == NULL || p_rsa_key_dmp1 == NULL ||
		p_rsa_key_dmq1 == NULL || p_rsa_key_iqmp == NULL) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	EVP_PKEY_CTX* rsa_ctx = NULL;
	EVP_PKEY *rsa_key = NULL;
	sgx_status_t ret_code = SGX_ERROR_UNEXPECTED;
	BIGNUM* n = NULL;
	BIGNUM* e = NULL;
	BIGNUM* d = NULL;
	BIGNUM* dmp1 = NULL;
	BIGNUM* dmq1 = NULL;
	BIGNUM* iqmp = NULL;
	BIGNUM* q = NULL;
	BIGNUM* p = NULL;
	BN_CTX* tmp_ctx = NULL;
	OSSL_PARAM_BLD *param_build = NULL;
	OSSL_PARAM *params = NULL;

	do {
		tmp_ctx = BN_CTX_new();
		NULL_BREAK(tmp_ctx);
		n = BN_new();
		NULL_BREAK(n);

		// convert RSA params, factors to BNs
		//
		p = BN_lebin2bn(p_rsa_key_p, (mod_size / 2), p);
		BN_CHECK_BREAK(p);
		q = BN_lebin2bn(p_rsa_key_q, (mod_size / 2), q);
		BN_CHECK_BREAK(q);
		dmp1 = BN_lebin2bn(p_rsa_key_dmp1, (mod_size / 2), dmp1);
		BN_CHECK_BREAK(dmp1);
		dmq1 = BN_lebin2bn(p_rsa_key_dmq1, (mod_size / 2), dmq1);
		BN_CHECK_BREAK(dmq1);
		iqmp = BN_lebin2bn(p_rsa_key_iqmp, (mod_size / 2), iqmp);
		BN_CHECK_BREAK(iqmp);
		e = BN_lebin2bn(p_rsa_key_e, (exp_size), e);
		BN_CHECK_BREAK(e);

		// calculate n value
		//
		if (!BN_mul(n, p, q, tmp_ctx)) {
			break;
		}

		//calculate d value
		//ϕ(n)=(p−1)(q−1)
		//d=(e^−1) mod ϕ(n)
		//
		d = BN_dup(n);
		NULL_BREAK(d);

		//select algorithms with an execution time independent of the respective numbers, to avoid exposing sensitive information to timing side-channel attacks.
		//
		BN_set_flags(d, BN_FLG_CONSTTIME);
		BN_set_flags(e, BN_FLG_CONSTTIME);

		if (!BN_sub(d, d, p) || !BN_sub(d, d, q) || !BN_add_word(d, 1) || !BN_mod_inverse(d, e, d, tmp_ctx)) {
			break;
		}

		// allocates and initializes an RSA key structure
		//
		rsa_ctx = EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL);
		if (rsa_ctx == NULL) {
			ret_code = SGX_ERROR_OUT_OF_MEMORY;
			break;
		}
		param_build = OSSL_PARAM_BLD_new();
		if (param_build == NULL) {
			break;
		}
		if( !OSSL_PARAM_BLD_push_BN(param_build, "n", n) 
		 || !OSSL_PARAM_BLD_push_BN(param_build, "e", e) 
		 || !OSSL_PARAM_BLD_push_BN(param_build, "d", d) 
		 || !OSSL_PARAM_BLD_push_BN(param_build, "rsa-factor1", p) 
		 || !OSSL_PARAM_BLD_push_BN(param_build, "rsa-factor2", q) 
		 || !OSSL_PARAM_BLD_push_BN(param_build, "rsa-exponent1", dmp1) 
		 || !OSSL_PARAM_BLD_push_BN(param_build, "rsa-exponent2", dmq1) 
		 || !OSSL_PARAM_BLD_push_BN(param_build, "rsa-coefficient1", iqmp) 
		 || (params = OSSL_PARAM_BLD_to_param(param_build)) == NULL) { 
			break;
		}
		if( EVP_PKEY_fromdata_init(rsa_ctx) <= 0) {
			break;
		}
		if( EVP_PKEY_fromdata(rsa_ctx, &rsa_key, EVP_PKEY_KEYPAIR, params) <= 0) {
			EVP_PKEY_free(rsa_key);
			break;
		}

		*new_pri_key2 = rsa_key;
		ret_code = SGX_SUCCESS;
	} while (0);

	OSSL_PARAM_BLD_free(param_build);
	OSSL_PARAM_free(params);
	EVP_PKEY_CTX_free(rsa_ctx);
	BN_clear_free(n);
	BN_clear_free(e);
	BN_clear_free(d);
	BN_clear_free(dmp1);
	BN_clear_free(dmq1);
	BN_clear_free(iqmp);
	BN_clear_free(q);
	BN_clear_free(p);
	BN_CTX_free(tmp_ctx);

	return ret_code;
}

sgx_status_t sgx_create_rsa_pub1_key(int mod_size, int exp_size, const unsigned char *le_n, const unsigned char *le_e, void **new_pub_key1)
{
	if (new_pub_key1 == NULL || mod_size <= 0 || exp_size <= 0 || le_n == NULL || le_e == NULL) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	EVP_PKEY *rsa_key = NULL;
	EVP_PKEY_CTX *rsa_ctx = NULL;
	sgx_status_t ret_code = SGX_ERROR_UNEXPECTED;
	BIGNUM* n = NULL;
	BIGNUM* e = NULL;
	OSSL_PARAM_BLD *param_build = NULL;
	OSSL_PARAM *params = NULL;

	do {
		//convert input buffers to BNs
		//
		n = BN_lebin2bn(le_n, mod_size, n);
		BN_CHECK_BREAK(n);
		e = BN_lebin2bn(le_e, exp_size, e);
		BN_CHECK_BREAK(e);

		rsa_ctx = EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL);
		if (rsa_ctx == NULL) {
			ret_code = SGX_ERROR_OUT_OF_MEMORY;
			break;
		}
		param_build = OSSL_PARAM_BLD_new();
		if (param_build == NULL) {
			break;
		}
		if( !OSSL_PARAM_BLD_push_BN(param_build, "n", n) 
		 || !OSSL_PARAM_BLD_push_BN(param_build, "e", e) 
		 || (params = OSSL_PARAM_BLD_to_param(param_build)) == NULL) { 
			break;
		}
		if( EVP_PKEY_fromdata_init(rsa_ctx) <= 0) {
			break;
		}
		if( EVP_PKEY_fromdata(rsa_ctx, &rsa_key, EVP_PKEY_PUBLIC_KEY, params) <= 0) {
			EVP_PKEY_free(rsa_key);
			break;
		}

		*new_pub_key1 = rsa_key;
		ret_code = SGX_SUCCESS;
	} while (0);

	EVP_PKEY_CTX_free(rsa_ctx);
	OSSL_PARAM_BLD_free(param_build);
	OSSL_PARAM_free(params);
	BN_clear_free(n);
	BN_clear_free(e);

	return ret_code;
}

sgx_status_t sgx_rsa_pub_encrypt_sha256(const void* rsa_key, unsigned char* pout_data, size_t* pout_len, const unsigned char* pin_data,
                                        const size_t pin_len)
{

    if (rsa_key == NULL || pout_len == NULL || pin_data == NULL || pin_len < 1 || pin_len >= INT_MAX)
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    EVP_PKEY_CTX *ctx = NULL;
    size_t data_len = 0;
    sgx_status_t ret_code = SGX_ERROR_UNEXPECTED;

    do
    {
        //allocate and init PKEY_CTX
        //
        ctx = EVP_PKEY_CTX_new((EVP_PKEY*)rsa_key, NULL);
        if ((ctx == NULL) || (EVP_PKEY_encrypt_init(ctx) < 1))
        {
            break;
        }

        //set the RSA padding mode, init it to use SHA256
        //
        EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING);
        EVP_PKEY_CTX_set_rsa_oaep_md(ctx, EVP_sha256());

        if (EVP_PKEY_encrypt(ctx, NULL, &data_len, pin_data, pin_len) <= 0)
        {
            break;
        }

        if(pout_data == NULL)
        {
            *pout_len = data_len;
            ret_code = SGX_SUCCESS;
            break;
        }

        else if(*pout_len < data_len)
        {
            ret_code = SGX_ERROR_INVALID_PARAMETER;
            break;
        }

        if (EVP_PKEY_encrypt(ctx, pout_data, pout_len, pin_data, pin_len) <= 0)
        {
            break;
        }

        ret_code = SGX_SUCCESS;
    }
    while (0);

    EVP_PKEY_CTX_free(ctx);

    return ret_code;
}

sgx_status_t sgx_rsa_priv_decrypt_sha256(const void* rsa_key, unsigned char* pout_data, size_t* pout_len, const unsigned char* pin_data,
        const size_t pin_len)
{

    if (rsa_key == NULL || pout_len == NULL || pin_data == NULL || pin_len < 1 || pin_len >= INT_MAX)
    {
        return SGX_ERROR_INVALID_PARAMETER;
    }

    EVP_PKEY_CTX *ctx = NULL;
    size_t data_len = 0;
    sgx_status_t ret_code = SGX_ERROR_UNEXPECTED;

    do
    {
        //allocate and init PKEY_CTX
        //
        ctx = EVP_PKEY_CTX_new((EVP_PKEY*)rsa_key, NULL);
        if ((ctx == NULL) || (EVP_PKEY_decrypt_init(ctx) < 1))
        {
            break;
        }

        //set the RSA padding mode, init it to use SHA256
        //
        EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING);
        EVP_PKEY_CTX_set_rsa_oaep_md(ctx, EVP_sha256());

        if (EVP_PKEY_decrypt(ctx, NULL, &data_len, pin_data, pin_len) <= 0)
        {
            break;
        }
        if(pout_data == NULL)
        {
            *pout_len = data_len;
            ret_code = SGX_SUCCESS;
            break;
        }

        else if(*pout_len < data_len)
        {
            ret_code = SGX_ERROR_INVALID_PARAMETER;
            break;
        }

        if (EVP_PKEY_decrypt(ctx, pout_data, pout_len, pin_data, pin_len) <= 0)
        {
            break;
        }
        ret_code = SGX_SUCCESS;
    }
    while (0);

    EVP_PKEY_CTX_free(ctx);

    return ret_code;
}

sgx_status_t sgx_create_rsa_priv1_key(int n_byte_size, int e_byte_size, int d_byte_size, const unsigned char *le_n, const unsigned char *le_e,
	const unsigned char *le_d, void **new_pri_key1)
{
	if (n_byte_size <= 0 || e_byte_size <= 0 || d_byte_size <= 0 || new_pri_key1 == NULL ||
		le_n == NULL || le_e == NULL || le_d == NULL) {
		return SGX_ERROR_INVALID_PARAMETER;
	}

	EVP_PKEY *rsa_key = NULL;
	EVP_PKEY_CTX *rsa_ctx = NULL;
	sgx_status_t ret_code = SGX_ERROR_UNEXPECTED;
	BIGNUM* n = NULL;
	BIGNUM* e = NULL;
	BIGNUM* d = NULL;
	OSSL_PARAM_BLD *param_build = NULL;
	OSSL_PARAM *params = NULL;

	do {
		//convert input buffers to BNs
		//
		n = BN_lebin2bn(le_n, n_byte_size, n);
		BN_CHECK_BREAK(n);
		e = BN_lebin2bn(le_e, e_byte_size, e);
		BN_CHECK_BREAK(e);
		d = BN_lebin2bn(le_d, d_byte_size, d);
		BN_CHECK_BREAK(d);

		rsa_ctx = EVP_PKEY_CTX_new_from_name(NULL, "RSA", NULL);
		if (rsa_ctx == NULL) {
			ret_code = SGX_ERROR_OUT_OF_MEMORY;
			break;
		}
		param_build = OSSL_PARAM_BLD_new();
		if (param_build == NULL) {
			break;
		}
		if( !OSSL_PARAM_BLD_push_BN(param_build, "n", n) 
		 || !OSSL_PARAM_BLD_push_BN(param_build, "e", e) 
		 || !OSSL_PARAM_BLD_push_BN(param_build, "d", d) 
		 || (params = OSSL_PARAM_BLD_to_param(param_build)) == NULL) { 
			break;
		}
		if( EVP_PKEY_fromdata_init(rsa_ctx) <= 0) {
			break;
		}
		if( EVP_PKEY_fromdata(rsa_ctx, &rsa_key, EVP_PKEY_KEYPAIR, params) <= 0) {
			EVP_PKEY_free(rsa_key);
			break;
		}

		*new_pri_key1 = rsa_key;
		ret_code = SGX_SUCCESS;
	} while (0);

	EVP_PKEY_CTX_free(rsa_ctx);
	OSSL_PARAM_BLD_free(param_build);
	OSSL_PARAM_free(params);
	BN_clear_free(n);
	BN_clear_free(e);
	BN_clear_free(d);

	return ret_code;
}

sgx_status_t sgx_free_rsa_key(void *p_rsa_key, sgx_rsa_key_type_t key_type, int mod_size, int exp_size) {
	(void)(key_type);
	(void)(mod_size);
	(void)(exp_size);
	if (p_rsa_key != NULL) {
		EVP_PKEY_free((EVP_PKEY*)p_rsa_key);
	}
	return SGX_SUCCESS;
}
