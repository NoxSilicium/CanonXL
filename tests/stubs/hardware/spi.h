// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

/**
 * @file spi.h
 * @brief Models the external hardware API for deterministic host tests.
 */
#pragma once
#include <stddef.h>
#include <stdint.h>

struct spi_inst_t
{
};

extern spi_inst_t* const spi0;

enum spi_cpol_t
{
    SPI_CPOL_1 = 1
};

enum spi_cpha_t
{
    SPI_CPHA_1 = 1
};

enum spi_order_t
{
    SPI_MSB_FIRST = 1
};

unsigned _spi_init(spi_inst_t* spi, unsigned hz);
void spi_set_format(
    spi_inst_t* spi, unsigned bits, spi_cpol_t cpol, spi_cpha_t cpha, spi_order_t order);
int spi_write_read_blocking(spi_inst_t* spi, const uint8_t* tx, uint8_t* rx, size_t count);
