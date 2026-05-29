/*
 * CoreModel SPI Mux Example
 *
 * Copyright (c) 2026 Corellium Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

void test_spi_cs(void *priv, unsigned csel);
int test_spi_xfr(void *priv, unsigned len, uint8_t *wrdata, uint8_t *rddata);
