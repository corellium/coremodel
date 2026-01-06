int test_i2c_start(void *priv);
int test_i2c_write(void *priv, unsigned len, uint8_t *data);
int test_i2c_read(void *priv, unsigned len, uint8_t *data);
void test_i2c_stop(void *priv);
