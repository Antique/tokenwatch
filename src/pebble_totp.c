#include "pebble_totp.h"

#include "unixtime.h"
#include "otp.h"

#define MIN(A, B) ((A) > (B) ? (B) : (A))

static unsigned short
pebble_totp_generate(pebble_totp *token)
{
    uint32_t currenttime = unixtime();
    unsigned int totpvalue;

    totpvalue = otp_value(token->key, sizeof(token->key), currenttime / token->interval);

    snprintf(token->buffer, sizeof(token->buffer), "%.6u", totpvalue);

    return currenttime % token->interval;
}


void
pebble_totp_init(pebble_totp *token,
                 const unsigned char *key,
                 size_t keylen,
                 unsigned short interval)
{
    memset(token->key, 0x00, sizeof(token->key));
    memcpy(token->key, key, MIN(sizeof(token->key), keylen));

    token->interval = interval;
    token->next = pebble_totp_generate(token);
}


bool
pebble_totp_tick(pebble_totp *token)
{
    if (++token->next < token->interval)
        return false;

    token->next = pebble_totp_generate(token);

    return true;
}


char *
pebble_totp_get_code(pebble_totp *token)
{
    return token->buffer;
}
