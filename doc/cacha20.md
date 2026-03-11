# ChaCha20 related

## HChaCha20
A fixed-input hash function (with limitations!) based on the ChaCha20 permutation.

This is identical to HSalsa20, but with the ChaCha20 permutation and IV.
This is also referred to as the "extended nonce" version of ChaCha20 / Salsa20.

Do not use the following code in production:
```rs
fn hchacha20(key: &[u8;32], nonce: &[u8;16]) -> [u8;32] {
  let mut state = [0_u32; 16];

  // Standard ChaCha20 initialization vector
  state[0] = 0x61707865;
  state[1] = 0x3320646e;
  state[2] = 0x79622d32;
  state[3] = 0x6b206574;

  for i in 0..8 {
    state[4 + i] = u32::from_le_bytes(&key[i * 4]);
  }

  for i in 0..4 {
    state[12 + i] = u32::from_le_bytes(&nonce[i * 4]);
  }

  for i in 0..10 {
    // run the 8 quarter rounds (one column row and one diagonal row)
    // ( see "inner_block" according to https://datatracker.ietf.org/doc/html/rfc7539#section-2.3.1 )
    inner_block(&mut state);
  }

  // prevent reversal of the rounds by revealing only half of the buffer.
  let mut out = [0_u8; 32];

  for i in 0..4 {
    out[(i * 4)..(i * 4 + 4)] = state[i].to_le_bytes();
  }

  for i in 0..4 {
    out[(i * 4 + 16)..(i * 4 + 16 + 4)] = state[i+12].to_le_bytes();
  }

  out
}
```

### Properties
The ChaCha N-round-function is a invertible 512bit -> 512bit permutation, with, according to DJB, "better diffusion than Salsa".

Invertible functions that map domain `X -> X` are always collission-free.

This means, that the N-round-function is a diffusing/hashing collision-free pseudo-random permutation function.

Base ChaCha20 adds the result of the N-round-function to the initial state, to make it effictively irreversible.

However, HChaCha20 takes a different approach: It only returns the bottom half of the state (while the input bytes are the top half, which shouldn't matter if the diffusion was perfect),
making the function irreversible, which is obvious, because if you only know half the information returned from `a+1`, you will have many possible values for `a`.

#### Collision resistance
Because of the birthday paradox, we have optimally `sqrt(2^256)` collision resistance (32B input -> expand to 64B state -> mix -> compress to 32B output),
which is `2^128` collission resistance.

This assumes 20-round ChaCha has a perfect diffusion.


### Test Vector
Key:
```
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
  0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
  0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
```

Nonce:
```
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4a,
  0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x2a, 0x3a,
```

Expected output:
```
  0xAD, 0xA7, 0xC7, 0xE3, 0x56, 0xC3, 0x58, 0xEC, 0x89, 0x85, 0xC0, 0xEA, 0x33, 0xBD, 0xC2, 0x38, 0x43, 0xE1, 0xE4, 0xAF, 0x79, 0xF1, 0x21, 0x62, 0xC4, 0xBD, 0xC5, 0x43, 0xF5, 0x51, 0xEF, 0x10,
```


## KChaCha20 (version 1.0-alpha.1)
Variable-length input hash function based on HChaCha20.

### DISCLAIMER
This function is not tested or verified yet!
DO NOT use this in sensitive applications yet!

### Algorithm
- A protocol constant is passed as parameter.
  Even though this allows up to 16 bytes,
  just using one of those bytes should be good enough.
  However, it is never allowed to be zero,
  and you should use different values for different applications (domain seperation)
- Input data is padded to a multipled of 32 bytes, according to ANSI X9.23 (zeros):
  - filled with null-bytes, except for the last byte, which is always the number of bytes added (including this)
  - this means:
    - if the input data is 31 bytes, a `0x01` is added.
    - if the input data is 30 bytes, `0x00` and `0x02` is added.
    - if the input data is 32 bytes, a new block of zeros, with `0x32` as last byte, is added
- Initialize a state of 32 bytes with:
  - the following 8 magic bytes:
    - `'0x4b', '0x43', '0x68', '0x61'`
    - `'0x43', '0x68', '0x61', '0x31'`
  - 24 zero bytes
- For each chunk of 32 bytes in the input message:
  - XOR the 32-byte state with the 32-byte chunk
  - let the new state be `hchacha20(key: state, nonce: protocol_constant)`
- The result is the output state

### Altrnative Variants
- 12 (instead of 20) round version: 6 column + 6 diagonal rounds: This should still be secure enough for most applications
- 8 (instead of 20) round version: 4 column + 4 diagonal rounds:
  probably still secure enough for most applications, but only recommended for less-secure applications
  (ex: use as hash function inside balloon-hash, for proof-of-work)

### Credits
Based on Loup Vaillant's ChaCha20 ECDH key derivation design: https://loup-vaillant.fr/articles/chacha20-key-derivation


### Pseudocode
Do not use the following code in production:
```rs
fn kchacha20_block(state: &mut [u8; 32], constant: &[u8;16], mut data: [u8;32]) {
  for i in 0..32 {
    data[i] ^= state[i];
  }

  *state = hchacha20(data, constant);
}

fn kchacha20(constant: &[u8;16], data: &[u8]) -> [u8;32] {
  let mut state = [0_u8; 32];
  state[0] = 0x4b; /* <-- IV will probably be removed in a futue version */
  state[1] = 0x43;
  state[2] = 0x68;
  state[3] = 0x61;
  state[4] = 0x43;
  state[5] = 0x68;
  state[6] = 0x61;
  state[7] = 0x31;

  let mut add_trailing_block = true;

  for chunk_i in 0..data.len().div_ceil(32) {
    let rem_data = &data[chunk_i * 32..];
    let chunk = &rem_data[..32.min(rem_data.len())];

    let mut block = [0_u8;32];
    for i in 0..chunk.len() {
      block[i] = chunk[i];
    }

    if chunk.len() != 32 {
      add_trailing_block = false;
      block[31] = 32 - chunk.len();
    }

    kchacha20_block(&mut state, constant, block);
  }

  if add_trailing_block {
    let mut block = [0_u8;32];
    block[31] = 32;
    kchacha20_block(&mut state, constant, block);
  }

  state
}
```


### Test vector
The data is the following ASCII string (without null-terminator):
```
DoNotCurrently-Use-KChaCha-InSensitive-Applications!!NeedingMoreBytes-for-getting-to-three-blocks.
```

With the protocol constant:
```
0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
0x0d, 0x0e, 0x0f, 0xfa
```

Running 20-round KChaCha, should produce:
```
  0x09, 0xff, 0x91, 0x83, 0x19, 0xfa, 0x21, 0xfd, 0x2d, 0x49, 0x13,
  0xf2, 0x7e, 0x00, 0x3e, 0x63, 0x96, 0xf5, 0x69, 0xc4, 0xfc, 0x94,
  0x53, 0xff, 0x31, 0x43, 0x9b, 0x6c, 0xc1, 0x45, 0x22, 0x79,
```


### Properties
To help analyze `KChaCha`, we'll analyze the inner code after the padding has been applied, and we'll also simplify the initialization vector:
```rs
fn inner(blocks: &[[u8;32]]) -> [u8;32] {
  let mut state = [0_u8; 32];
  state[..7] = medium_entropy_constant;

  for mut block in blocks {
    state = hchacha20(key:   block ^ state,
                      nonce: low_entropy_constant);
  }

  state
}
```

The padding adds the following constraints into our inner function:
- After padding, input is always at least 1 block.
- At least the last block of the input contains non-zero data.


TODO: send help


## References
- https://loup-vaillant.fr/tutorials/chacha20-design
- https://loup-vaillant.fr/articles/chacha20-key-derivation
