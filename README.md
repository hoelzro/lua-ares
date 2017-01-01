# lua-ares - Lua bindings for c-ares

[c-ares](https://c-ares.haxx.se/) is an asynchronous DNS resolver library
for C - lua-ares is a Lua binding to this library.

# Example

```lua
local ares = require 'ares' -- also calls ares_library_init()
local sock = require 'socket'

local r = ares.init() -- create a resolver object
local count = 0
local domain = 'example.com'
local function make_reply_handler(parser)
  return function(status, timeouts, result)
    count = count + 1
    local reply = parser(result)
    print(reply.addr_list[1])
  end
end

-- fire off queries here
r:query(domain, ares.class.IN, ares.type.A, make_reply_handler(ares.parse_a_reply))
r:query(domain, ares.class.IN, ares.type.AAAA, make_reply_handler(ares.parse_aaaa_reply))

while count < 2 do
  print 'waiting...'
  sock.sleep(1)
  local rfds, wfds, nfd = r:fds()
  -- you can't use socket select here, since it takes socket objects
  r:process(rfds, wfds) -- handle any responses that have come in
end
```

# Functions

## ares.init(opt_options)

Creates and returns a resolver object using `ares_init`.  `opt_options` is optional;
if provided, `ares_init_options` will be used with the given options.  If invalid
options are specified, `error` will be used to throw a hard error.  If resolver
creation fails for some reason, `nil` and an error message will be returned.

## ares.parse_a_reply(bytes)

Parses the bytes given to the DNS response handler as if they are a response for an
A query.  Returns a table with information on the host, which has the following fields
to mirror the information provided by `struct hostent`:

  * name - The host's name (usually what you provided to the query method)
  * aliases - Other names for the host
  * addrtype - `"inet"`
  * addr_list - A table of IPv4 addresses.

If the parse fails, `nil` and an error message are returned.

## ares.parse_aaaa_reply(bytes)

Parses the bytes given to the DNS response handler as if they are a response for an
AAAA query.  Returns a table with information on the host, which has the following fields
to mirror the information provided by `struct hostent`:

  * name - The host's name (usually what you provided to the query method)
  * aliases - Other names for the host
  * addrtype - `"inet6"`
  * addr_list - A table of IPv6 addresses.

If the parse fails, `nil` and an error message are returned.

# Resolver Methods

## resolver:query(domain, class, type, reply_handler)

Sends a query to the DNS resolver.  `class` is a value from `ares.class`; `type` is a value
from `ares.type`.  `reply_handler` is a function that receives a status, timeout count, and
bytes from the DNS reply.  `status` is a value that indicates the status or failure of the
query; `timeouts` indicates how many times the query timed out, and the bytes are raw bytes
that are meant to be decoded with a parser function such as `parse_a_reply`.

## resolver:fds()

Returns a table of readable file descriptors, a table of writable file descriptors, and the
max file descriptor for use in functions like `select`.

## resolver:process(read_fds, write_fds)

Processes the results from queries using the file descriptors in `read_fds` and `write_fds`,
potentially calling the reply handler if the query has been completed.

# Constants

`ares.class` and `ares.type` contain constants for DNS classes and query types, respectively.

# To-do

# License

lua-ares is released under the same license as c-ares and Lua - the MIT/X license,
a copy of which has been included under [COPYING](COPYING).
