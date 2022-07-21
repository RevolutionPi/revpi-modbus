# Assessment of current implementation
At time of writing there are three open questions

  * Can coils be supported from the slave side of the implementation?
  * Can the problems with "state machine", periods and timouts be solved?
  * Can we make it work with GCC 10?

all with current implementation, or do we need a rewrite and if so: why?

## Support of Slave Coils
This has been evaluated [here](io.md).

The struct for the process image layout already contains sizes of coils

```C
typedef struct
{
    uint32_t u32DiscreteInputsOffset;        // process image offset in bytes
    uint32_t u32DiscreteInputsLength;        // data length in bits
    uint32_t u32CoilsInputOffset;            // process image offset in bytes
    uint32_t u32CoilsLength;                 // data length in bits
    uint32_t u32InputRegistersOffset;        // process image offset in bytes
    uint32_t u32InputRegistersLength;        // data length in bytes
    uint32_t u32HoldingRegistersInputOffset; // process image offset in bytes
    uint32_t u32HoldingRegistersLength;      // data length in bytes
}TProcessImageConfiguration;

```

`parse_modbus_slave_device_process_image_config` in piConfigParser.c does not
recognize these parts, though. I.e. in order to implement coils the image size
slave devices have to be increased, and the configuration by PiCtory must be
extended. Other than that the read/write access is already implemented by
piProcessImageAccess.c.

This ought to be sufficient for handling bits/coils within piControl, as well,
as long as piControl parses the same config.rsc.

## Solve issue with timings
First of all: there is no such thing as a state machine in the modbus code.
There's scheduling for the Modbus master.

As [this forum's post](https://revolutionpi.de/forum/viewtopic.php?p=11870#p11887)
describes with RTU there is more to this than just messed up timings. It's being
proven there that libmodbus itself either has no collison detection or we don't
make use of it. This problem arises because multidrop modbus needs RS-485 which
is half duplex.

[libmodbus therefore respects half duplex communication](https://github.com/stephane/libmodbus/issues/360),
i.e. it handles collision prevention via RTS, i.e. it cannot be expected to work
with just two cable wiring or a slave device that uses the RTS line faulty or not
at all.

### Timings
The electrical problem aside, there also is
[undocumented timing behavior](https://revolutionpi.com/forum/viewtopic.php?t=943#p3862).

The user can set the *action interval* via PiCtory. A problem may occur because
the scheduler determines the timeout for a device's response by the **shortest**
action interval on the bus.

> The half of the smallest action interval is used. If you have an action with an
> action interval of 1000ms and one with 2000ms, the timeout will be 500ms.

The reasoning for this is not documented but can be fixed easily if there is no more
complex change required than using the longest interval or divider other than 2.
Everything else would preferably need a reasoned requirement for the scheduling,
that may or may not be compatible with the current code.

Additionally it is recommended to make the timeouts for device response and in
between messages known to the user by adding read-only fields in PiCtory.

## Porting to GCC 10
Short answer: Yes!

There are only minor issues with the code compiling/linking with GCC 10, that
are purely related to the omission of the `extern` keyword for global variables
which leads to the "multiple definition of" error while linking.

[See GCC porting documentation about the issue.](https://gcc.gnu.org/gcc-10/porting_to.html#common)

The tricky part of the current implementation is that all occurances of the
problem are bound to macros from `<sys/queue.h>`.