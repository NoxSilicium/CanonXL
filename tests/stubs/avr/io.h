// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

/**
 * @file io.h
 * @brief Models the external hardware API for deterministic host tests.
 */
#pragma once
#include <stdint.h>
#define _BV(bit) (1U << (bit))

enum
{
    PB2 = 2,
    PB3 = 3,
    PB4 = 4,
    PB5 = 5
};

enum
{
    SPR0 = 0,
    CPHA = 2,
    CPOL = 3,
    MSTR = 4,
    DORD = 5,
    SPE = 6,
    SPIF = 7
};

extern volatile uint8_t PORTB, DDRB, SPCR, SPSR;

class FakeDataRegister
{
    public:

    void operator=(uint8_t value);
    operator uint8_t() const;
};

extern FakeDataRegister SPDR;
