#include "miner.h"
#include "algo-gate-api.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef NO_AES_NI
  #include "sph_groestl.h"
#else
  #include "aes_ni/hash-groestl.h"
#endif
#include "algo/sha3/sph_sha2.h"

typedef struct {
#ifdef NO_AES_NI
    sph_groestl512_context  groestl;
#else
        hashState_groestl       groestl;
#endif
    sph_sha256_context sha;
} myrgr_ctx_holder;

myrgr_ctx_holder myrgr_ctx;

void init_myrgr_ctx()
{
#ifdef NO_AES_NI
     sph_groestl512_init( &myrgr_ctx.groestl );
#else
        init_groestl (&myrgr_ctx.groestl );
#endif
     sph_sha256_init(&myrgr_ctx.sha);
}

void myriadhash(void *output, const void *input)
{
     myrgr_ctx_holder ctx;
     memcpy( &ctx, &myrgr_ctx, sizeof(myrgr_ctx) );

 	uint32_t _ALIGN(32) hash[16];
//	sph_groestl512_context ctx;
//	sph_sha256_context sha_ctx;

	// memset(&hash[0], 0, sizeof(hash));

//	sph_groestl512_init(&ctx);
#ifdef NO_AES_NI
	sph_groestl512(&ctx.groestl, input, 80);
	sph_groestl512_close(&ctx.groestl, hash);
#else
        update_groestl( &ctx.groestl, (char*)hash,512);
        final_groestl( &ctx.groestl, (char*)hash);
#endif

//	sph_sha256_init(&sha_ctx);
	sph_sha256(&ctx.sha, hash, 64);
	sph_sha256_close(&ctx.sha, hash);

	memcpy(output, hash, 32);

}

int scanhash_myriad(int thr_id, struct work *work,
	uint32_t max_nonce, uint64_t *hashes_done)
{
        uint32_t *pdata = work->data;
        uint32_t *ptarget = work->target;

	uint32_t _ALIGN(64) endiandata[20];
	const uint32_t first_nonce = pdata[19];
	uint32_t nonce = first_nonce;

	if (opt_benchmark)
		((uint32_t*)ptarget)[7] = 0x0000ff;

	for (int k=0; k < 20; k++)
		be32enc(&endiandata[k], ((uint32_t*)pdata)[k]);

	do {
		const uint32_t Htarg = ptarget[7];
		uint32_t hash[8];
		be32enc(&endiandata[19], nonce);
		myriadhash(hash, endiandata);

		if (hash[7] <= Htarg && fulltest(hash, ptarget)) {
			pdata[19] = nonce;
			*hashes_done = pdata[19] - first_nonce;
			return 1;
		}
		nonce++;

	} while (nonce < max_nonce && !work_restart[thr_id].restart);

	pdata[19] = nonce;
	*hashes_done = pdata[19] - first_nonce + 1;
	return 0;
}

bool register_myriad_algo( algo_gate_t* gate )
{
      gate->aes_ni_optimized = true;
      gate->init_ctx = (void*)&init_myrgr_ctx;
      gate->scanhash = (void*)&scanhash_myriad;
      gate->hash     = (void*)&myriadhash;
      gate->hash_alt = (void*)&myriadhash;
      gate->get_max64 = (void*)&get_max64_0x3ffff;
    return true;
};

