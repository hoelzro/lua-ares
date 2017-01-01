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

## ares.init()

## ares.parse_a_reply

## ares.parse_aaaa_reply

# Resolver Methods

## resolver:query(domain, class, type, reply_handler)

## resolver:fds()

## resolver:process(read_fds, write_fds)

# Constants

`ares.class` and `ares.type` contain constants for DNS classes and query types, respectively.

# To-do

# License

lua-ares is released under the same license as c-ares and Lua - the MIT/X license,
a copy of which has been included under [COPYING](COPYING).
