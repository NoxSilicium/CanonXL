// SPDX-License-Identifier: LicenseRef-CanonXL-NC-1.0
// Copyright (c) 2026 Sebastien Marchand (NoxSilicium)

/**
 * @file gpio.h
 * @brief Models the external hardware API for deterministic host tests.
 */
#pragma once
#define GPIO_OUT true
#define GPIO_FUNC_SPI 1
#define GPIO_FUNC_SIO 0
void _gpio_init(unsigned pin);
void gpio_put(unsigned pin, bool value);
void gpio_set_dir(unsigned pin, bool output);
void gpio_set_function(unsigned pin, unsigned function);
