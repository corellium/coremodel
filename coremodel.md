# CoreModel

The CoreModel API provides over-the-internet support to attach remote peripheral models to a virtual Machine(VM) bus interfaces.
There are multiple standard device interfaces supported including UART, I2C, SPI, CAN, GPIO, and USB Host.

```
  -------------     ------------     -------------------
  | CoreModel | <-> | Internet | <-> | Virtual Machine |
  -------------     ------------     -------------------
```

## General CoreModel Connection and Control

These API's and helper functions provide simple way to connect and interact with a VMs bus interfaces.


### Connection

The cormodel_connect/disconnect functions provide a standard way to connect and disconnect over the network to the VM.
The `coremodel_connect` function takes a `<target>` string formatted as `"ip:port"` for example "10.10.0.3:1900" and returns 0 on success. 

```
/* Connect to a VM. */
int coremodel_connect(const char *target);

/* Close connection to a VM. */
void coremodel_disconnect(void);
```

### Main Loop

The `coremodel_mainloop` helper function provides a simple implementation of the device model main loop.
Primarily the main loop handles processing the connection send and receive between the device model and the VM interface.
`coremodel_mainloop` takes the parameter `usec` setting how much time to spend in the loop. The value of `usec` is specified in microseconds and if set to -1 will run indefinitely.
The main loop will return an error flag of 0 on success. Helper function `coremodel_mainloop` wraps the usage of the file descriptor functions.

```
int coremodel_mainloop(long long usec);
```

### File Descriptor

The file descriptor functions set and process the read and write buffers of the attached device model.

```
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
```

### Detach Device

Detach any device model by handle from the VM.

```
/*  handle      handle of UART/I2C/SPI/GPIO interface */
void coremodel_detach(void *handle);
```

### Device List

Device list functions provide a way to enumerate available bus interfaces of the VM into an array and free that array.
The function `coremodel_list` returns an array of `coremodel_device_list_t` data structure and is terminated by the last element member `type` set to `COREMODEL_INVALID`.

```
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
```


### Attach Interface

All attach functions require a correct `<name>` be provided to generate a `handle` for the VMs interface. 
The correct name can be retrieved from device list.
A proper `<func>` data structure for the specific interface is required.
Any attach function will return `NULL` on failure.
If the device model being attached to one of the interfaces does not need any independent state structure or specific value for operation then NULL can be provided to `<priv>`.

## UART

The CoreModel UART APIs provides the ability to attach a single device to any available virtual UART on the VM.

### UART Data Structure

The following data structure represents required functions that need to be provided to `coremodel_attach_uart` for a device to interface with the VMs UART.

```
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
```

Stub functions for `brk` and `rxrdy` can be provided if they are not required for the device model operation.

### UART Attach

Attach to a virtual UART and returns the handle.

```
void *coremodel_attach_uart(const char *name, const coremodel_uart_func_t *func, void *priv);
```

### UART RX

Attempt to send data over the virtual UART.

```
int coremodel_uart_rx(void *uart, unsigned len, uint8_t *data);
```

Provide `<uart>` the attached handle of the UART interface.
The `<data>` and `<len>` of the array to send to the interface.
Returns a number >0 of how many bytes were accepted if 0 then the interface is stalled. CoreModel will call `func->rxrdy` to un-stall the device.


### UART TX Ready

Unstall a stalled Tx interface of the `<uart>` handle.

```
void coremodel_uart_txrdy(void *uart);
```

## I2C

The CoreModel I2C APIs provides the ability to attach multiple devices to any available virtual I2C bus on the VM. The virtual I2C bus supports a maximum of 128 devices.

### I2C Data Structure

The data structure below is allocated and owned by the device model.

```
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
```

### I2C Attach

The virtual I2C bus only supports 7-bit addresses that are defined `<addr>` when the device is attached.
Depending on the VM there could already be other devices on the bus and those addresses can not be used.
`<flags>` can be set to notify the master of non standard behavior of the device. 

```
#define COREMODEL_I2C_START_ACK 0x0001  /* device must ACK all starts */
#define COREMODEL_I2C_WRITE_ACK 0x0002  /* device must ACK all writes */

void *coremodel_attach_i2c(const char *name, uint8_t addr, const coremodel_i2c_func_t *func, void *priv, uint16_t flags);
```

### I2C Push Read

Provide unsolicited data for lower access latency.

```
int coremodel_i2c_push_read(void *i2c, unsigned len, uint8_t *data);
```

Push unsolicited I2C READ `<data>` of `<len>` in bytes to the `<i2c>` handle.
Returns the number of accepted bytes by the host controller. 

### I2C Ready

Signal CoreModel that the `<i2c>` handle of the interface is unstalled and can call `func->start/write/read` again. 

```
void coremodel_i2c_ready(void *i2c);
```


## SPI

The CoreModel SPI APIs provides the ability to attach multiple devices to any available virtual SPI bus on the VM.
The maximum amount of devices the virtual SPI bus supports can be different between VMs or even instances of SPI masters.
To determine the maximum `<num>` of supported devices for the bus use device list. [described above](#device-list)


### SPI Data Structure

```
typedef struct {
    /* Called by CoreModel to notify of a CS pin change. */
    void (*cs)(void *priv, unsigned csel);
    /* Called by CoreModel to write and read bytes. Return a >0 number to
     * accept (and produce) as many bytes, or 0 to stall interface (it will
     * have to be un-stalled with coremodel_spi_ready). */
    int (*xfr)(void *priv, unsigned len, uint8_t *wrdata, uint8_t *rddata);
} coremodel_spi_func_t;
```

### SPI Attach

Attach a device to the VMs virtual SPI bus. Since multiple devices can be attached to the same SPI interface the chip select index `<csel>` must be >= 0 and < `<num>` max devices.
VMs could have devices already on the SPI bus and their chip select index can not be used as they are hardwired inside the VM.
`<flags>` can be set to notify the master of non standard behavior of the device. 

```
#define COREMODEL_SPI_BLOCK     0x0001  /* device must handle >1 byte transfers */

void *coremodel_attach_spi(const char *name, unsigned csel, const coremodel_spi_func_t *func, void *priv, uint16_t flags);
```

### SPI Ready

Notify CoreModel that the `<spi>` handle of the interface is unstalled and can call `func->xfr` again. 

```
void coremodel_spi_ready(void *spi);
```

## CAN Bus

The CoreModel CAN APIs provides the ability to interface multiple devices to any available virtual CAN bus on the VM.

### CAN Data Structure

```
#define CAN_CTRL_ID_MASK        (0x7FFul << 29)
#define  CAN_CTRL_ID_SHIFT      29
#define CAN_CTRL_RTR            (1ul << 28)
#define CAN_CTRL_IDE            (1ul << 27)
#define CAN_CTRL_EID_MASK       (0x3FFFFul << 9)
#define  CAN_CTRL_EID_SHIFT     9
#define CAN_CTRL_ERTR           (1ul << 8)
#define CAN_CTRL_EDL            (1ul << 7)
#define CAN_CTRL_BRS            (1ul << 5)
#define CAN_CTRL_ESI            (1ul << 4)
#define CAN_CTRL_DLC_MASK       15ul
#define  CAN_CTRL_DLC_SHIFT     0

#define CAN_ACK                 0
#define CAN_NAK                 1
#define CAN_STALL               (-1)
typedef struct {
    int (*tx)(void *priv, uint64_t ctrl, uint8_t *data); /* return one of CAN_ACK, CAN_NAK, CAN_STALL */
    void (*rxcomplete)(void *priv, int nak);
} coremodel_can_func_t;
```

### CAN Attach

Attach a device to a virtual CAN bus.

```
void *coremodel_attach_can(const char *name, const coremodel_can_func_t *func, void *priv);
```

### CAN RX

Send CAN packet over the virtual `<can>` interface. The `<data>` portion of the packet is optional if control word `<ctrl>` DLC != 0.
`coremodel_can_rx` returns 0 on success if the bus is not available 1 will be returned.

```
int coremodel_can_rx(void *can, uint64_t ctrl, uint8_t *data);
```

### CAN Ready

Unstall the stalled virtual interface `<can>` signaling CoreModel that `func->tx` can be called once again.

```
void coremodel_can_ready(void *can);
```

## GPIO

The CoreModel GPIO APIs provides the ability to interact with the VMs GPIO pins logical or voltage values.
Caution should be taken when manipulating GPIO pins that are already being used internally by the VM as this could cause undefined behavior.
The count of GPIO pins can be determined using device list. [described above](#device-list) 

### GPIO Notify Structure

```
typedef struct {
    /* Called by CoreModel to update voltage on a GPIO pin. */
    void (*notify)(void *priv, int mvolt);
} coremodel_gpio_func_t;
```

### GPIO Attach

Attach to a specified `<pin>` index in the bank of GPIO pins.

```
void *coremodel_attach_gpio(const char *name, unsigned pin, const coremodel_gpio_func_t *func, void *priv);
```

### GPIO Set

Set a tri-state driver on a GPIO `<pin>` interface, enabling or disabling the `<drven>` `<mvolt>` value in millivolts.

```
void coremodel_gpio_set(void *pin, unsigned drven, int mvolt);
```

## USB Host

The CoreModel USBH APIs provides the ability to interface multiple devices to any available virtual USB Bus.

### USBH Data Structure

```
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
```

### USBH Attach

Attach to a virtual USB host `<port>` index at a requested connection `<speed>`.
VMs could have devices already on the USB bus and the port index those device are attached to should not be used as they are hardwired inside the VM.

```
#define USB_SPEED_LOW           0
#define USB_SPEED_FULL          1
#define USB_SPEED_HIGH          2
#define USB_SPEED_SUPER         3
void *coremodel_attach_usbh(const char *name, unsigned port, const coremodel_usbh_func_t *func, void *priv, unsigned speed);
```

### USBH Ready

Unstall a stalled virtual USB interface end point `<ep>` with the token `<tkn>`  notifying CoreModel can call `func->xfr` once again.

```
/* Unstall a stalled interface (signal that CoreModel can once again call
 * func->xfr).
 *  usb         handle of USB interface
 *  ep          endpoint to signal as ready
 *  tkn         token to signal as ready
 */
void coremodel_usbh_ready(void *usb, uint8_t ep, uint8_t tkn);
```
