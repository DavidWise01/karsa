#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define FACES 6
#define TUNNEL_STEPS 27
#define CARRIER_BYTES 27
#define MAX_NEIGHBORS 6

typedef enum {
    ST_SUBSPACE = 250,   // symbolic S, mapped outside normal ladder
    ST_NEST_NEG2 = 254,  // -2 hermit crab nest
    ST_FLASH_NEG1 = 255, // -1 protected library
    ST_REST_0 = 0,
    ST_SEED_1 = 1,
    ST_OSC_2 = 2,
    ST_WITNESS_3 = 3,
    ST_CORE_5 = 5,
    ST_BLOOM_8 = 8,
    ST_WALL_27 = 27,
    ST_PHASE_28 = 28
} State;

typedef struct {
    uint8_t carrier[CARRIER_BYTES];     // 6x6x6 = 216 bits transport
    uint16_t seed_packet;               // evolving curriculum
    uint32_t tunnel_bits;               // lower 27 bits are sensory path
    uint8_t phase;
    uint8_t step;
    State state;
    uint8_t checksum;
    bool flash_ro;
} CellSeed;

static const char *FACE_NAME[FACES] = {"+X", "-X", "+Y", "-Y", "+Z", "-Z"};

static uint8_t checksum(const CellSeed *c) {
    uint8_t x = 0;
    for (int i = 0; i < CARRIER_BYTES; i++) x ^= c->carrier[i];
    x ^= (uint8_t)c->seed_packet;
    x ^= (uint8_t)(c->seed_packet >> 8);
    x ^= c->phase ^ c->step ^ (uint8_t)c->state;
    return x;
}

static void repack_carrier(CellSeed *c) {
    memset(c->carrier, 0, sizeof(c->carrier));
    // Put seed bits into first 16 positions.
    for (int i = 0; i < 16; i++) {
        if ((c->seed_packet >> i) & 1u) c->carrier[i / 8] |= (uint8_t)(1u << (i % 8));
    }
    // Put 1x0 marker: DIR=ASSIMILATE(1), FACE=+X(0), STEP=0 => bit 36.
    c->carrier[36 / 8] |= (uint8_t)(1u << (36 % 8));
    c->checksum = checksum(c);
}

static uint16_t compress_27_to_16(uint32_t tunnel_bits) {
    uint16_t seed = 0;
    // Direct lower 16 bits.
    seed ^= (uint16_t)(tunnel_bits & 0xFFFFu);
    // Fold upper 11 bits back into the seed.
    seed ^= (uint16_t)((tunnel_bits >> 16) & 0x07FFu);
    // Parity scar: if many disturbances, mark high bit.
    int count = 0;
    for (int i = 0; i < TUNNEL_STEPS; i++) count += (tunnel_bits >> i) & 1u;
    if (count >= 8) seed |= 0x8000u;
    return seed;
}

static void init_cell(CellSeed *c) {
    memset(c, 0, sizeof(*c));
    c->state = ST_FLASH_NEG1;
    c->flash_ro = true;
    c->seed_packet = 0x0000;
    repack_carrier(c);
}

static void push_flash(CellSeed *c) {
    printf("PUSH: -1 library streams upward\n");
    printf("  -1 FLASH -> 0 REST\n"); c->state = ST_REST_0;
    printf("   0 REST  -> 1 SEED\n"); c->state = ST_SEED_1;
    printf("   1 SEED  -> 2 OSCILLATE -> 1 -> 2\n"); c->state = ST_OSC_2;
    printf("   2 OSC   -> 3 WITNESS\n"); c->state = ST_WITNESS_3;
    printf("   3 WIT   -> 5 CORE\n"); c->state = ST_CORE_5;
    printf("   5 CORE  -> 8 BLOOM\n"); c->state = ST_BLOOM_8;
}

static void run_tunnel(CellSeed *c, uint32_t environment_mask) {
    c->tunnel_bits = 0;
    printf("RUN: tunnel phase=%u seed=0x%04x\n", c->phase, c->seed_packet);
    for (int i = 0; i < TUNNEL_STEPS; i++) {
        c->step = (uint8_t)i;
        bool sensed = ((environment_mask >> i) & 1u) != 0;
        bool known_scar = ((c->seed_packet >> (i % 16)) & 1u) != 0;
        bool danger = sensed && !known_scar;
        if (danger) c->tunnel_bits |= (1u << i);
        if (i == 26) c->state = ST_WALL_27;
    }
    printf("  collected tunnel_bits=0x%07x\n", c->tunnel_bits & 0x07FFFFFFu);
}

static void self_teach(CellSeed *c) {
    c->state = ST_FLASH_NEG1;
    c->flash_ro = false;
    uint16_t before = c->seed_packet;
    uint16_t new_seed = compress_27_to_16(c->tunnel_bits);
    c->seed_packet ^= new_seed; // the learning operation
    c->flash_ro = true;
    repack_carrier(c);
    printf("SELF-TEACH: collapse -> -1, seed 0x%04x XOR 0x%04x = 0x%04x\n",
           before, new_seed, c->seed_packet);
}

static void teach_neighbor(const CellSeed *self, CellSeed *other, int face) {
    memcpy(other->carrier, self->carrier, CARRIER_BYTES);
    other->seed_packet = self->seed_packet;
    other->phase = (uint8_t)(self->phase + 1);
    other->step = 0;
    other->state = ST_FLASH_NEG1;
    other->flash_ro = true;
    other->checksum = checksum(other);
    printf("TEACH %s: 1x0 curriculum seed=0x%04x phase %u -> %u checksum=0x%02x\n",
           FACE_NAME[face], self->seed_packet, self->phase, other->phase, other->checksum);
}

static void six_face_teach(const CellSeed *self, CellSeed neighbors[FACES]) {
    printf("BLOOM: six-face propagation from state 8\n");
    for (int f = 0; f < FACES; f++) teach_neighbor(self, &neighbors[f], f);
}

static bool verify(const CellSeed *c) {
    return checksum(c) == c->checksum;
}

int main(void) {
    CellSeed parent;
    CellSeed child[FACES];
    init_cell(&parent);

    printf("=== SPARK SPORE TEACHING KERNEL ===\n");
    printf("Self-teach = XOR at -1. Teach others = six-face 1x0 bloom.\n\n");

    push_flash(&parent);

    // Environment with hazards at steps 15 and 22.
    uint32_t environment = (1u << 15) | (1u << 22);
    run_tunnel(&parent, environment);
    self_teach(&parent);
    push_flash(&parent);

    if (parent.state == ST_BLOOM_8) six_face_teach(&parent, child);

    printf("\nVERIFY parent: %s\n", verify(&parent) ? "PASS" : "FAIL");
    for (int f = 0; f < FACES; f++) {
        printf("VERIFY child %s: %s\n", FACE_NAME[f], verify(&child[f]) ? "PASS" : "FAIL");
    }

    printf("\nRESULT:\n");
    printf("  Parent learned scar tissue: seed_packet=0x%04x\n", parent.seed_packet);
    printf("  Children inherit curriculum, not a clone. Each must walk its own tunnel.\n");
    printf("  1x0 is student and teacher. Flash is library. 6x6x6 is classroom.\n");
    return 0;
}
