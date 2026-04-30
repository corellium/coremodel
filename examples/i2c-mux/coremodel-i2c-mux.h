/*
 * CoreModel I2C Mux Example
 *
 * Copyright (c) 2026 Corellium Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

int test_i2c_start(void *priv);
int test_i2c_write(void *priv, unsigned len, uint8_t *data);
int test_i2c_read(void *priv, unsigned len, uint8_t *data);
void test_i2c_stop(void *priv);
