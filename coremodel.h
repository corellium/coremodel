/*
 *  CoreModel C API
 *
 *  Copyright Corellium 2022-2023
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef _COREMODEL_H
#define _COREMODEL_H

#include <stdint.h>
#include <sys/select.h>

/* Connect to a VM.
 *  target      string like "10.10.0.3:1900"
 * Returns error flag.
 */
int coremodel_connect(const char *target);

/* Enumerates devices available in VM.
 * Returns invalid-terminated array of device structs. The array, as well as
 * names in it, is allocated by malloc(3). */
#define COREMODEL_UART          0
#define COREMODEL_I2C           1
#define COREMODEL_SPI           2
#define COREMODEL_GPIO          3
#define COREMODEL_USBH          4
#define COREMODEL_CAN           5
#define COREMODEL_INVALID       (-1)
typedef struct {
    int type; /* one of COREMODEL_* constants */
    char *name; /* name used to attach to the device */
    unsigned num; /* number of chip selects (SPI) or pins (GPIO) */
} coremodel_device_list_t;
coremodel_device_list_t *coremodel_list(void);

/* Frees a device list */
void coremodel_free_list(coremodel_device_list_t *list);

/* UART */

typedef struct {
    /* Called by CoreModel to transmit bytes. Return a >0 number to accept as
     * many bytes, or 0 to stall Tx interface (it will have to be un-stalled
     * with coremodel_uart_txrdy). */
    int (*tx)(void *priv, unsigned len, uint8_t *data);
    /* Called by CoreModel to signal a BREAK condition on UART line. */
    void (*brk)(void *priv);
    /* Called by CoreModel to unstall Rx interface. */
    void (*rxrdy)(void *priv);
} coremodel_uart_func_t;

/* Attach to a virtual UART.
 *  name        name of the UART interface, depends on the VM
 *  func        set of function callbacks to attach
 *  priv        priv value to pass to each callback
 * Returns handle of UART, or NULL on failure. */
void *coremodel_attach_uart(const char *name, const coremodel_uart_func_t *func, void *priv);

/* Try to push data into the virtual UART.
 *  uart        handle of UART
 *  len         number of bytes to send to the Rx interface
 *  data        data to send
 * Returns a >0 number when this many bytes were accepted, or 0 to signal stall
 * of the Rx interface (CoreModel will call func->rxrdy to un-stall it). */
int coremodel_uart_rx(void *uart, unsigned len, uint8_t *data);

/* Unstall a stalled Tx interface (signal that CoreModel can once again call
 * func->tx to push data).
 *  uart        handle of UART
 */
void coremodel_uart_txrdy(void *uart);

/* I2C */

typedef struct {
    /* Called by CoreModel to notify of a START to a device; return -1 to NAK,
     * 0 to stall, 1 to accept. A stalled interface will have to be un-stalled
     * with coremodel_i2c_ready. */
    int (*start)(void *priv);
    /* Called by CoreModel to WRITE bytes. Return a >0 number to accept as
     * many bytes, -1 to NAK, or 0 to stall interface (it will have to be
     * un-stalled with coremodel_i2c_ready). */
    int (*write)(void *priv, unsigned len, uint8_t *data);
    /* Called by CoreModel to READ bytes. Return a >0 number to produce as
     * many bytes, or 0 to stall interface (it will have to be un-stalled
     * with coremodel_i2c_ready). */
    int (*read)(void *priv, unsigned len, uint8_t *data);
    /* Called by CoreModel to notify of a STOP to a device. */
    void (*stop)(void *priv);
} coremodel_i2c_func_t;

/* Attach to a virtual I2C bus.
 *  name        name of the I2C bus, depends on the VM
 *  addr        7-bit address to attach
 *  func        set of function callbacks to attach
 *  priv        priv value to pass to each callback
 *  flags       behavior flags of device
 * Returns handle of I2C interface, or NULL on failure. */
#define COREMODEL_I2C_START_ACK 0x0001  /* device must ACK all starts */
#define COREMODEL_I2C_WRITE_ACK 0x0002  /* device must ACK all writes */
void *coremodel_attach_i2c(const char *name, uint8_t addr, const coremodel_i2c_func_t *func, void *priv, uint16_t flags);

/* Push unsolicited I2C READ data. Used to lower access latency.
 *  i2c         handle of I2C interface
 *  len         number of bytes to send to the Rx interface
 *  data        data to send
 * Returns number of bytes accepted. */
int coremodel_i2c_push_read(void *i2c, unsigned len, uint8_t *data);

/* Unstall a stalled interface (signal that CoreModel can once again call
 * func->start/write/read).
 *  i2c         handle of I2C interface
 */
void coremodel_i2c_ready(void *i2c);

/* SPI */

typedef struct {
    /* Called by CoreModel to notify of a CS pin change. */
    void (*cs)(void *priv, unsigned csel);
    /* Called by CoreModel to write and read bytes. Return a >0 number to
     * accept (and produce) as many bytes, or 0 to stall interface (it will
     * have to be un-stalled with coremodel_spi_ready). */
    int (*xfr)(void *priv, unsigned len, uint8_t *wrdata, uint8_t *rddata);
} coremodel_spi_func_t;

/* Attach to a virtual SPI bus.
 *  name        name of the SPI bus, depends on the VM
 *  csel        chip select index
 *  func        set of function callbacks to attach
 *  priv        priv value to pass to each callback
 *  flags       behavior flags of device
 * Returns handle of SPI interface, or NULL on failure. */
#define COREMODEL_SPI_BLOCK     0x0001  /* device must handle >1 byte transfers */
void *coremodel_attach_spi(const char *name, unsigned csel, const coremodel_spi_func_t *func, void *priv, uint16_t flags);

/* Unstall a stalled interface (signal that CoreModel can once again call
 * func->xfr).
 *  spi         handle of SPI interface
 */
void coremodel_spi_ready(void *spi);

/* GPIO */

typedef struct {
    /* Called by CoreModel to update voltage on a GPIO pin. */
    void (*notify)(void *priv, int mvolt);
} coremodel_gpio_func_t;

/* Attach to a virtual GPIO pin.
 *  name        name of the GPIO bank, depends on the VM
 *  pin         pin index within bank
 *  func        set of function callbacks to attach
 *  priv        priv value to pass to each callback
 * Returns handle of GPIO interface, or NULL on failure. */
void *coremodel_attach_gpio(const char *name, unsigned pin, const coremodel_gpio_func_t *func, void *priv);

/* Set a tri-state driver on a GPIO pin.
 *  pin         handle of GPIO interface
 *  drven       driver enable
 *  mvolt       voltage to drive (if enabled) in mV */
void coremodel_gpio_set(void *pin, unsigned drven, int mvolt);

/* USB Host (connect a local USB Device to a Host inside VM) */

#define USB_TKN_OUT             0
#define USB_TKN_IN              1
#define USB_TKN_SETUP           2
#define USB_XFR_NAK             (-1)
#define USB_XFR_STALL           (-2)
typedef struct {
    /* Called by CoreModel on USB bus reset. */
    void (*rst)(void *priv);
    /* Called by CoreModel to perform a USB transfer. Return a >0 number to
     * accept / produce as many bytes, or USB_XFR_NAK to pause interface, or
     * USB_XFR_STALL to stall interface (create error condition). A paused
     * interface will have to be un-paused with coremodel_usbh_ready. */
    int (*xfr)(void *priv, uint8_t dev, uint8_t ep, uint8_t tkn, uint8_t *buf, unsigned size, uint8_t end);
} coremodel_usbh_func_t;

/* Attach to a virtual USB host.
 *  name        name of the USB host, depends on the VM
 *  port        USB port index
 *  func        set of function callbacks to attach
 *  priv        priv value to pass to each callback
 *  speed       requested connection speed
 * Returns handle of USB interface, or NULL on failure. */
#define USB_SPEED_LOW           0
#define USB_SPEED_FULL          1
#define USB_SPEED_HIGH          2
#define USB_SPEED_SUPER         3
void *coremodel_attach_usbh(const char *name, unsigned port, const coremodel_usbh_func_t *func, void *priv, unsigned speed);

/* Unstall a stalled interface (signal that CoreModel can once again call
 * func->xfr).
 *  usb         handle of USB interface
 *  ep          endpoint to signal as ready
 *  tkn         token to signal as ready
 */
void coremodel_usbh_ready(void *usb, uint8_t ep, uint8_t tkn);

/* CAN node */
#define CAN_CTRL1_SEC           (1ul << 59)
#define CAN_CTRL1_SDT_MASK      (0xFFul << CAN_CTRL1_SDT_SHIFT)
#define CAN_CTRL1_SDT_SHIFT      51
#define CAN_CTRL1_VCID_MASK     (0xFFul << CAN_CTRL1_VCID_SHIFT)
#define CAN_CTRL1_VCID_SHIFT     43
#define CAN_CTRL1_PRIO_MASK     (0x7FFul << CAN_CTRL1_PRIO_SHIFT)
#define CAN_CTRL1_PRIO_SHIFT     32
#define CAN_CTRL1_AF_MASK       (0xFFFFFFFFul << CAN_CTRL1_AF_SHIFT)
#define CAN_CTRL1_AF_SHIFT       0

#define CAN_CTRL_XLF            (1ul << 49)
#define CAN_CTRL_FDF            (1ul << 48)
#define CAN_CTRL_ID_MASK        (0x7FFul << CAN_CTRL_ID_SHIFT)
#define CAN_CTRL_ID_SHIFT        36
#define CAN_CTRL_RTR            (1ul << 35)
#define CAN_CTRL_IDE            (1ul << 34)
#define CAN_CTRL_EID_MASK       (0x3FFFFul << CAN_CTRL_EID_SHIFT)
#define CAN_CTRL_EID_SHIFT       16
#define CAN_CTRL_ERTR           (1ul << 15)
#define CAN_CTRL_EDL            (1ul << 14)
#define CAN_CTRL_BRS            (1ul << 12)
#define CAN_CTRL_ESI            (1ul << 11)
#define CAN_CTRL_DLC_MASK       (0x7FFul << CAN_CTRL_DLC_SHIFT)
#define CAN_CTRL_DLC_SHIFT       0

#define CAN_ACK                 0
#define CAN_NAK                 1
#define CAN_STALL               (-1)
typedef struct {
    int (*tx)(void *priv, uint64_t *ctrl, uint8_t *data); /* return one of CAN_ACK, CAN_NAK, CAN_STALL */
    void (*rxcomplete)(void *priv, int nak);
} coremodel_can_func_t;

/* Attach to a virtual CAN bus.
 *  name        name of the CAN bus, depends on the VM
 *  func        set of function callbacks to attach
 *  priv        priv value to pass to each callback
 * Returns handle of CAN interface, or NULL on failure. */
void *coremodel_attach_can(const char *name, const coremodel_can_func_t *func, void *priv);

/* Send a packet to CAN bus.
 *  can         handle of CAN interface
 *  ctrl        control word
 *  data        optional data (if ctrl.DLC != 0)
 * Returns 0 on success, 1 if bus is not available because previous packet hasn't been completed yet. */
int coremodel_can_rx(void *can, uint64_t *ctrl, uint8_t *data);

/* Unstall a stalled interface (signal that CoreModel can once again call func->tx).
 *  can         handle of CAN interface
 */
void coremodel_can_ready(void *can);

/* Other functions */

/* Prepare fd_sets for select(2).
 *  nfds        current index of maximum fd in sets + 1
 *  readfds     readfds to update
 *  writefds    writefds to update
 * Returns new nfds.
 */
int coremodel_preparefds(int nfds, fd_set *readfds, fd_set *writefds);

/* Process fd_sets after select(2).
 *  readfds     readfds to process
 *  writefds    writefds to process
 * Returns error flag.
 */
int coremodel_processfds(fd_set *readfds, fd_set *writefds);

/* Simple implementation of a main loop.
 *  usec        time to spend in loop, in microseconds; negative means forever
 * Returns error flag.
 */
int coremodel_mainloop(long long usec);

/* Detach any interface.
 *  handle      handle of UART/I2C/SPI/GPIO interface */
void coremodel_detach(void *handle);

/* Close connection to a VM. */
void coremodel_disconnect(void);

#endif
