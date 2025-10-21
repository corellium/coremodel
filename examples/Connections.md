# Connecting to CoreModel

There are several different ways to connect your CoreModel example to a VM depending on how you access Corellium products. If you access a Corellium product in the public cloud or in a virtual private cloud then you cannot directly access the VMs directly and instead will neither either a VPN or SSH tunnel to the virtual network. If you are running a Corellium product locally, on a server or desktop appliance, then the default is that VMs will be directly accessible on your local network. Your IT department may have configured an isolated network with an SSH jump server. While possible, it is uncommon for IT departments to setup VPN configurations for isolated networks. The following procedure will walk you through determining how to connect to the CoreModel interface.

## Determining VM IP Addresses

Each VM will have at lest two IP addresses (more if the device had more than one network connection enabled) that are documented on the connections tab once the device is booted. Scrolling to the bottom of the page will reveal the *LAN* and *Services IP* addresses. When accessing Corellium in a public or managed cloud configuration these will generally be of the form `10.11.0.x` for the *LAN* ip address and `10.11.1.x` for the *Services IP*. When accessing Corellium on a local appliance the addresses are likely to be sequentially assigned by your DHCP server. You can check if these addresses are accessible by trying to `ssh` to the addresses. If your VM is running a Linux-like operating system an ssh connection to the `LAN` ip will result in a login prompt if the address is accessible or a very long time out if not accessible. An ssh connection to the `Services IP` will similarly result in an almost immediate "Connection refused" error or very long time out. In either case make note of both addresses which will be referred to as `<lan ip>` and `<services ip>` in the next steps.

## Accessing the VM IP Addresses

In most cases the `<lan ip>` and `<services ip>` will not be directly accessible from your workstation and will require a VPN or SSH Tunnel.

### Option VPN Connection

When connecting to a public or managed cloud Corellium instance the *Connections* tab will include a *Connect via VPN* section that provides a button to download an OpenVPN configuration file and references to documentation for setting up a connection. Once connected you should be able to directly connect to the `<lan ip>` and `<services ip>` addresses. For more details see the [VPN section of the product documentation](https://support.avh.corellium.com/features/connect/vpn).

### Option Quick Connect

When connecting to a public or managed cloud Corellium instance, and using a VM with a Linux-like operating system, the *Connections* tab may show a *Quick Connect* section that provides an example *SSH* command of the form `ssh -J <UUID>@proxy.app.avh.corellium.com root@<lan ip>`. The `<UUID>` uniquely identifies your project's proxy session. To use this proxy your ssh id_ed25519.pub key must be added to the *Project* by a project admin as documented in the [Quick Connect section of the product documentation](https://support.avh.corellium.com/features/connect/quick-connect). If your device does not have a *Quick Connect* section the `<UUID>` can be found by querying the project node in the REST API, or more easily by instantiating a device that does have a quick connect section; such as the i.MX8m, i.MX93, or RaspberryPI 4; in the same project.

### Option Corporate Connection

While it is impossible to document how every customer's IT department will have configured their corporate network, in most cases they will have setup an SSH jump or proxy server. They will generally then tell you to connect to your appliance via a command similar to `ssh -J <user>@<proxy> <appliance IP>` or `ssh -J <user>@<proxy> -L 443:<appliance IP>:443 -fN`. You will not be using this `<appliance ip>` for CoreModel but may need it to access the Corellium user interface.

## Recommendation SSH Port Forwarding

No matter how you access `<lan ip>` and `<services ip>` addresses we recommend setting up port forwarding from the local host to the `<services ip>` so that the command used in testing your CoreModel example can always use `127.0.0.1:1900` as the `<address:port>` to access the VM. This becomes especially important when testing the same example against multiple VMs where the testing sequence can become quite complex. It is much simpler to change the port forward than to rewrite dozens of commands for the test case.

If you have direct access to the `<services ip>` the most simple port forward command is

```bash
ssh -L 1900:<services ip>:1900 -N
```

which forwards any connection from port 1900 on the local host to port 1900 on the `<services ip>`. The `-N` tells ssh not to run any command on the remote end and simply wait for you to kill the connection with `<ctrl>-c`. Optionally you can add a `-f` to the command line to put `ssh` in the background to keep the prompt active for other uses. More often when testing a CoreModel example you will want a connection to the VM command line so the combined `ssh` command

```bash
ssh -L 1900:<services ip>:1900 <user>@<lan>
```

is more useful as you end up with a shell on the VM. Adding your `~/.ssh/id_ed25519.pub` to `~/.ssh/authorized_keys` on the VM will make this command much more efficient.

If you need to use a proxy to access the `<services ip>` these commands become

```bash
ssh <user>@<proxy> -L 1900:<services ip>:1900 -N
```

and

```bash
ssh -J <user>@<proxy> <user>@<lan>
```

where in the case of public or managed cloud `<user>` is the UUID for the project. Note the addition of the `-J` when connecting to the VM prompt. Note also that the "obvious" combination of proxy/jump server, port forwarding, and console connection

```bash
ssh -J <user>@<proxy> -L 1900:<services ip>:1900 <user>@<lan>
```

**Does NOT work** as rather than asking the proxy/jump sever to perform the port forwarding it asks the OS running on the VM to perform the port forwarding which can cause the VM to become unresponsive. Use the two commands in separate terminals.
