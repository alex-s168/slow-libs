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
    // run the 8 quarter rounds
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

## KChaCha20
Variable-length input hash function based on HChaCha20.

Algorithm:
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
  state[0] = 0x4b;
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

Based on Loup Vaillant's ChaCha20 ECDH key derivation design: https://loup-vaillant.fr/articles/chacha20-key-derivation

## References
- https://loup-vaillant.fr/tutorials/chacha20-design
- https://loup-vaillant.fr/articles/chacha20-key-derivation
