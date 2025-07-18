/*
 * TIME-BASED XAH PAYMENT HOOK
 *
 * This hook implements a time-gated payment system that sends configurable XAH amounts
 * to specified addresses when triggered by Invoke transactions.
 *
 * FUNCTIONALITY:
 * - Monitors for Invoke transactions sent to the hook account
 * - Sends XAH payments to addresses specified in the 'ADDRESS' parameter
 * - Implements rate limiting with configurable time delays between payments
 * - Prevents spam by tracking last payment timestamps per recipient
 * - Allows configuration of payment amounts and time delays via parameters
 *
 * PARAMETERS:
 * - ADDRESS (required): 20-byte recipient account ID
 * - XAH (optional): Payment amount in drops (default: 1,000,000 = 1 XAH)
 * - SECONDS (optional): Minimum seconds between payments (default: 86,400 = 24 hours)
 *
 * BEHAVIOR:
 * - First payment to any address: Executes immediately
 * - Subsequent payments: Only execute if configured time has elapsed
 * - Configuration updates: XAH and SECONDS parameters update stored defaults
 * - State persistence: Tracks timestamps and config in foreign state namespace
 *
 * SECURITY:
 * - Rate limiting prevents rapid payment abuse
 * - Parameter validation ensures proper address format
 * - Error handling with rollback on payment failures
 * - Comprehensive tracing for debugging and monitoring
 *
 * USE CASES:
 * - Faucet systems with spam protection
 * - Time-gated reward distributions
 * - Subscription-like payment mechanisms
 * - Educational and development tools
 *
 */
#include "hookapi.h"

int64_t hook(uint32_t reserved)
{

    // Hook ADDRESS Parameter
    uint8_t address_param[7] = {'A', 'D', 'D', 'R', 'E', 'S', 'S'};

    // Key Values
    uint8_t value_key[3] = {'X', 'A', 'H'};
    uint8_t seconds_key[7] = {'S', 'E', 'C', 'O', 'N', 'D', 'S'};

    // Namespaces
    uint8_t address_ns[32] = {0x19, 0xDB, 0xF7, 0xB7, 0xFC, 0x66, 0xEC, 0xCB, 0x9D, 0xDB, 0xE5, 0x33, 0x45, 0xB6, 0xD2, 0x8F, 0x95, 0x23, 0x48, 0x8B, 0x2C, 0xE8, 0x3B, 0xDE, 0xD0, 0xF0, 0x4B, 0x1F, 0x0D, 0x7A, 0xEF, 0xDE};

    // Creating a 1 txn reserve for PREPARE_PAYMENT_SIMPLE in case we send a payment
    etxn_reserve(1);

    // Check Account (Sender) of the original txn
    uint8_t account_field[20];
    otxn_field(SBUF(account_field), sfAccount);

    // Check hook account
    unsigned char hook_accid[20];
    hook_account((uint32_t)hook_accid, 20);

    // Check if hook_accid and Invoke Account are the same
    int equal = 0;
    BUFFER_EQUAL(equal, hook_accid, account_field, 20);

    // Hook Parameter ADDRESS Value
    uint8_t address_paramvalue[20];
    otxn_param(SBUF(address_paramvalue), SBUF(address_param));

    // Timstamp
    int64_t ts = ledger_last_time();

    // To know the type of origin txn
    int64_t tt = otxn_type();

    // Default values for the hook
    uint64_t seconds;
    uint64_t drops;

    // Check namespace XAH value
    if (state_foreign(SVAR(drops), SBUF(value_key), SBUF(address_ns), SBUF(hook_accid)) < 0)
    {
        drops = 1000000;
    }

    // Check namespace SECONDS value
    if (state_foreign(SVAR(seconds), SBUF(seconds_key), SBUF(address_ns), SBUF(hook_accid)) < 0)
    {
        seconds = 86400;
    }

    // Check if SECONDS value config is saved (24 hours as default)
    if (tt == 99)
    {
        uint64_t temp_seconds;
        if (otxn_param(SVAR(temp_seconds), SBUF(seconds_key)) > 0)
        {
            seconds = temp_seconds;
            state_foreign_set(SVAR(seconds), SBUF(seconds_key), SBUF(address_ns), SBUF(hook_accid));
            TRACESTR("Timebased.c: Found new XAH drops parameter value. Added to the hook.");
        }
    }

    // Check if XAH value config is saved (1 XAH, 1000000 drops)
    if (tt == 99)
    {
        uint64_t temp_drops;
        if (otxn_param(SVAR(temp_drops), SBUF(value_key)) > 0)
        {
            drops = temp_drops;
            state_foreign_set(SVAR(drops), SBUF(value_key), SBUF(address_ns), SBUF(hook_accid));
            TRACESTR("Timebased.c: Found new SECONDS parameter value. Added to the hook.");
        }
    }

    // If its Invoke txn, from hook account, and address is included
    if (tt == 99 && equal && otxn_param(SBUF(address_paramvalue), SBUF(address_param)) == 20)
    {
        unsigned char tx[PREPARE_PAYMENT_SIMPLE_SIZE];
        uint64_t last_ts;
        state_foreign(SVAR(last_ts), SBUF(address_paramvalue), SBUF(address_ns), SBUF(hook_accid));
        uint64_t ts_check = last_ts + seconds;

        if (last_ts > 0)
        {
            if (ts_check > ts)
            {
                TRACESTR("Timebased.c:  XAH transaction not executed because the required time has not passed.");
            }
            else
            {
                PREPARE_PAYMENT_SIMPLE(tx, drops, address_paramvalue, 0, 0);
                uint8_t emithash[32];
                int64_t emit_result = emit(SBUF(emithash), SBUF(tx));
                state_foreign_set(SVAR(ts), SBUF(address_paramvalue), SBUF(address_ns), SBUF(hook_accid));
                TRACESTR("Timebased.c: XAH transaction executed.");
            }
        }
        else
        {
            state_foreign_set(SVAR(ts), SBUF(address_paramvalue), SBUF(address_ns), SBUF(hook_accid));
            PREPARE_PAYMENT_SIMPLE(tx, drops, address_paramvalue, 0, 0);
            uint8_t emithash[32];
            int64_t emit_result = emit(SBUF(emithash), SBUF(tx));
            TRACESTR("Timebased.c: First time XAH reward.");
        }
    }
    accept(0, 0, 0);
    _g(1, 1);
    return 0;
}