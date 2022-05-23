#include "nrf_soc.h"
#include "nrf_delay.h"
#include "app_error.h"

#define ECB_KEY_LEN            (16UL)
#define COUNTER_BYTE_LEN       (4UL)
#define NONCE_RAND_BYTE_LEN    (12UL)

// The RNG wait values are typical and not guaranteed. See Product Specifications for more info.
#ifdef NRF51
#define RNG_BYTE_WAIT_US       (677UL)
#elif defined NRF52
#define RNG_BYTE_WAIT_US       (124UL)
#else
#error "Either NRF51 or NRF52 must be defined."
#endif

/**
 * @brief Uses the RNG to write a 12-byte nonce to a buffer
 * @details The 12 bytes will be written to the buffer starting at index 4 to leave
 *          space for the 4-byte counter value.
 *
 * @param[in]    p_buf    An array of length 16
 */
void nonce_generate(uint8_t * p_buf)
{
    uint8_t i         = COUNTER_BYTE_LEN;
    uint8_t remaining = NONCE_RAND_BYTE_LEN;

    // The random number pool may not contain enough bytes at the moment so
    // a busy wait may be necessary.
    while(0 != remaining)
    {
        uint32_t err_code;
        uint8_t  available = 0;

        err_code = sd_rand_application_bytes_available_get(&available);
        APP_ERROR_CHECK(err_code);

        available = ((available > remaining) ? remaining : available);
        if (0 != available)
        {
            err_code = sd_rand_application_vector_get((p_buf + i), available);
            APP_ERROR_CHECK(err_code);

            i         += available;
            remaining -= available;
        }

        if (0 != remaining)
        {
            nrf_delay_us(RNG_BYTE_WAIT_US * remaining);
        }
    }    
}

static bool m_initialized = false;

// NOTE: The ECB data must be located in RAM or a HardFault will be triggered.
static nrf_ecb_hal_data_t m_ecb_data;

/**
 * @brief Initializes the module with the given nonce and key
 * @details The nonce will be copied to an internal buffer so it does not need to
 *          be retained after the function returns. Additionally, a 32-bit counter
 *          will be initialized to zero and placed into the least-significant 4 bytes
 *          of the internal buffer. The nonce value should be generated in a
 *          reasonable manner (e.g. using this module's nonce_generate function).
 *
 * @param[in]    p_nonce    An array of length 16 containing 12 random bytes
 *                          starting at index 4
 * @param[in]    p_ecb_key  An array of length 16 containing the ECB key
 */
void ctr_init(const uint8_t * p_nonce, const uint8_t * p_ecb_key)
{
    m_initialized = true;

    // Save the key.
    memcpy(&m_ecb_data.key[0], p_ecb_key, ECB_KEY_LEN);

    // Copy the nonce.
    memcpy(&m_ecb_data.cleartext[COUNTER_BYTE_LEN],
              &p_nonce[COUNTER_BYTE_LEN],
              NONCE_RAND_BYTE_LEN);

    // Zero the counter value.
    memset(&m_ecb_data.cleartext[0], 0x00, COUNTER_BYTE_LEN);
}

static uint32_t crypt(uint8_t * buf)
{
    uint8_t  i;
    uint32_t err_code;

    if (!m_initialized)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    err_code = sd_ecb_block_encrypt(&m_ecb_data);
    if (NRF_SUCCESS != err_code)
    {
        return err_code;
    }

    for (i=0; i < ECB_KEY_LEN; i++)
    {
        buf[i] ^= m_ecb_data.ciphertext[i];
    }

    // Increment the counter.
    (*((uint32_t*) m_ecb_data.cleartext))++;

    return NRF_SUCCESS;
}

/**
 * @brief Encrypts the given buffer in-situ
 * @details The encryption step is done separately (using the nonce, counter, and
 *          key) and then the result from the encryption is XOR'd with the given
 *          buffer in-situ. The counter will be incremented only if no error occurs.
 *
 * @param[in]    p_clear_text    An array of length 16 containing the clear text
 *
 * @retval    NRF_SUCCESS                         Success
 * @retval    NRF_ERROR_INVALID_STATE             Module has not been initialized
 * @retval    NRF_ERROR_SOFTDEVICE_NOT_ENABLED    SoftDevice is present, but not enabled
 */
uint32_t ctr_encrypt(uint8_t * p_clear_text)
{
    return crypt(p_clear_text);
}

/**
 * @brief Decrypts the given buffer in-situ
 * @details The encryption step is done separately (using the nonce, counter, and
 *          key) and then the result from the encryption is XOR'd with the given
 *          buffer in-situ. The counter will be incremented only if no error occurs.
 *
 * @param[in]    p_cipher_text    An array of length 16 containing the cipher text
 *
 * @retval    NRF_SUCCESS                         Succeess
 * @retval    NRF_ERROR_INVALID_STATE             Module has not been initialized
 * @retval    NRF_ERROR_SOFTDEVICE_NOT_ENABLED    SoftDevice is present, but not enabled
 */
uint32_t ctr_decrypt(uint8_t * p_cipher_text)
{
    return crypt(p_cipher_text);
}

