# Time-Based XAH Payment Hook

## Overview
This Xahau hook automatically sends XAH payments to specified addresses when triggered by Invoke transactions, with configurable time delays between payments to prevent spam and abuse.

## How It Works
The hook monitors for Invoke transactions sent to the account where it's installed. When an Invoke transaction includes an `ADDRESS` parameter, the hook will send a payment to that address, but only if enough time has passed since the last payment to the same address.

## Features
- **Time-based rate limiting**: Configurable delay between payments to the same address
- **Configurable payment amount**: Set custom XAH amounts via parameters
- **Persistent state**: Tracks last payment timestamps per address
- **Spam protection**: Prevents rapid successive payments to abuse the system
- **Parameter-driven configuration**: Easy to customize via transaction parameters

## Hook Hash
4B906C46E5E27F398A4577829F44B0790AFD3AF1145A81BC3947367FB4C9417D

## Configuration Parameters

### Required Parameters
- `ADDRESS`: 20-byte account ID of the payment recipient

### Optional Parameters
- `XAH`: Payment amount in drops (default: 1,000,000 drops = 1 XAH)
- `SECONDS`: Minimum time between payments to same address (default: 86,400 seconds = 24 hours)

## Usage

### Installation
1. Deploy the hook to your Xahau account
2. The hook will automatically activate for Invoke transactions

### Parameter Encoding Requirements

**IMPORTANT**: All hook parameters must be encoded in HEX format before sending. Use the [XRPL Hex Visualizer](https://transia-rnd.github.io/xrpl-hex-visualizer/) to convert your values:

- **Parameter Names**: Convert strings to HEX (e.g., "ADDRESS" → HEX string)
- **XRP Addresses**: Use the "xrpAddress" field to convert addresses to 20-byte HEX
- **Numbers**: Use "uint64 LE" (Little Endian) format for numeric values

### Triggering Payments
Send an Invoke transaction to the hook account with the required parameters:

```javascript
// Example transaction with HEX-encoded parameters
{
  "TransactionType": "Invoke",
  "Account": "rYourAccount...",
  "Destination": "rHookAccount...",
  "HookParameters": [
    {
      "HookParameter": {
        "HookParameterName": "41444452455353", // "ADDRESS" in HEX
        "HookParameterValue": "A1B2C3D4E5F6789..." // 20-byte address in HEX (40 chars)
      }
    }
  ]
}
```

### Configuration Updates
Update the default payment amount or time delay by including additional parameters:

```javascript
// Configure 2 XAH payments with 1-hour delay (HEX encoded)
{
  "TransactionType": "Invoke",
  "Account": "rYourAccount...",
  "Destination": "rHookAccount...",
  "HookParameters": [
    {
      "HookParameter": {
        "HookParameterName": "41444452455353", // "ADDRESS" in HEX
        "HookParameterValue": "A1B2C3D4E5F6789..." // 20-byte address in HEX
      }
    },
    {
      "HookParameter": {
        "HookParameterName": "584148", // "XAH" in HEX
        "HookParameterValue": "80841E0000000000" // 2000000 in uint64 LE HEX
      }
    },
    {
      "HookParameter": {
        "HookParameterName": "5345434F4E4453", // "SECONDS" in HEX  
        "HookParameterValue": "100E0000000000000" // 3600 in uint64 LE HEX
      }
    }
  ]
}
```

### How to Encode Parameters

1. **Visit the [XRPL Hex Visualizer](https://transia-rnd.github.io/xrpl-hex-visualizer/)**

2. **For Parameter Names (strings)**:
   - Enter the parameter name (e.g., "ADDRESS", "XAH", "SECONDS")
   - Copy the HEX output for `HookParameterName`

3. **For Addresses**:
   - Use the "xrpAddress" field
   - Enter the recipient address (e.g., "rN7n7otQDd6FczFgLdSqtcsAUxDkw6fzRH")
   - Copy the 40-character HEX output for `HookParameterValue`

4. **For Numbers (XAH amounts, seconds)**:
   - Use the "uint64 LE" field
   - Enter the numeric value (e.g., 1000000 for 1 XAH, 3600 for 1 hour)
   - Copy the 16-character HEX output for `HookParameterValue`

### Example Encoding
```
Parameter Name: "ADDRESS" → HEX: "41444452455353"
Address: "rN7n7otQDd6FczFgLdSqtcsAUxDkw6fzRH" → HEX: "9FD2E46F8EA7CAE3..."
Amount: 2000000 drops → uint64 LE HEX: "80841E0000000000"
Time: 3600 seconds → uint64 LE HEX: "100E000000000000"
```

## State Management
The hook maintains state using a foreign state namespace to track:
- Last payment timestamp for each recipient address
- Configured payment amount (XAH parameter)
- Configured time delay (SECONDS parameter)

## Security Features
- **Rate limiting**: Prevents rapid successive payments
- **Parameter validation**: Ensures ADDRESS parameter is exactly 20 bytes
- **Error handling**: Rolls back on payment emission failures
- **Spam protection**: Time-based delays prevent abuse

## Default Values
- **Payment Amount**: 1 XAH (1,000,000 drops)
- **Time Delay**: 24 hours (86,400 seconds)
- **Transaction Type**: Only responds to Invoke transactions (type 99)

## Error Handling
- Invalid parameters are ignored
- Failed payments trigger transaction rollback
- Missing configuration uses safe defaults
- Comprehensive tracing for debugging

## Use Cases
- **Faucet systems**: Distribute test XAH with rate limiting
- **Reward systems**: Time-gated rewards for user actions
- **Subscription payments**: Regular payments with time controls
- **Gaming rewards**: Prevent reward farming with cooldowns

## Requirements
- Xahau network
- Account with hook installation privileges
- Understanding of Invoke transaction format
- Basic knowledge of hex encoding for addresses

## Possible Improvements for Interested Developers

The current implementation provides a solid foundation, but there are several enhancements that could make this hook even more powerful and flexible:

### 🚫 **Address Blacklist System**
Implement a blacklist mechanism to prevent certain addresses from receiving XAH payments, even if they meet all other conditions.

**Implementation approach:**
- Create a blacklist namespace to store prohibited addresses
- Add `BLACKLIST_ADD` and `BLACKLIST_REMOVE` parameters for management
- Check blacklist before executing any payment
- Provide admin-only access to blacklist modifications

**Use cases:**
- Block known spam or malicious addresses
- Prevent abuse from specific accounts
- Temporary suspension of problematic users

### 🔒 **Global Kill Switch**
Add a master switch to temporarily halt all XAH distributions while maintaining the hook's other functionality.

**Implementation approach:**
- Store a global `ENABLED` flag in the hook's state
- Add `PAUSE` and `RESUME` parameters for control
- Continue tracking timestamps but skip payment execution when disabled
- Include emergency stop functionality for critical situations

**Use cases:**
- Maintenance periods
- Emergency response to detected exploits
- Temporary suspension during investigations
- Controlled testing phases

### 🎯 **Special Address Policies**
Implement custom rules for specific addresses using dedicated namespaces for policy management.

**Implementation approach:**
- Create separate namespaces for different policy types
- Allow per-address custom amounts and time delays
- Support VIP addresses with reduced cooldowns
- Implement tiered reward systems based on address classification

**Policy examples:**
- **VIP addresses**: Reduced cooldown times or higher amounts
- **New user bonus**: First-time users get larger initial payments
- **Loyalty rewards**: Longer-term users receive better rates
- **Partner addresses**: Special rates for integrated services

**Multi-token support:**
- Extend beyond XAH to support other tokens
- Token-specific rate limiting and amounts

Developers interested in implementing these features should consider the trade-offs between functionality and complexity, ensuring that additional features don't compromise the hook's reliability or security.

---

*This hook is designed for educational and development purposes. Test thoroughly before production use.*
