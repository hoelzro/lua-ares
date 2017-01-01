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

Since `status` is currently just a raw number value, I wouldn't rely on it until the API
for it gets more refined.

## resolver:fds()

Returns a table of readable file descriptors, a table of writable file descriptors, and the
max file descriptor for use in functions like `select`.

## resolver:process(read_fds, write_fds)

Processes the results from queries using the file descriptors in `read_fds` and `write_fds`,
potentially calling the reply handler if the query has been completed.

# Constants

`ares.class` and `ares.type` contain constants for DNS classes and query types, respectively.

# To-do

## Unbound Functions

The following functions have no binding - contributions adding them are welcome!

  * [ares_cancel](https://c-ares.haxx.se/ares_cancel.html)
  * [ares_create_query](https://c-ares.haxx.se/ares_create_query.html)
  * [ares_dup](https://c-ares.haxx.se/ares_dup.html)
  * [ares_expand_name](https://c-ares.haxx.se/ares_expand_name.html)
  * [ares_expand_string](https://c-ares.haxx.se/ares_expand_string.html)
  * [ares_get_servers](https://c-ares.haxx.se/ares_get_servers.html)
  * [ares_get_servers_ports](https://c-ares.haxx.se/ares_get_servers_ports.html)
  * [ares_gethostbyaddr](https://c-ares.haxx.se/ares_gethostbyaddr.html)
  * [ares_gethostbyname](https://c-ares.haxx.se/ares_gethostbyname.html)
  * [ares_gethostbyname_file](https://c-ares.haxx.se/ares_gethostbyname_file.html)
  * [ares_getnameinfo](https://c-ares.haxx.se/ares_getnameinfo.html)
  * [ares_getsock](https://c-ares.haxx.se/ares_getsock.html)
  * [ares_inet_ntop](https://c-ares.haxx.se/ares_inet_ntop.html)
  * [ares_inet_pton](https://c-ares.haxx.se/ares_inet_pton.html)
  * [ares_mkquery](https://c-ares.haxx.se/ares_mkquery.html)
  * [ares_parse_a_reply](https://c-ares.haxx.se/ares_parse_a_reply.html)
  * [ares_parse_aaaa_reply](https://c-ares.haxx.se/ares_parse_aaaa_reply.html)
  * [ares_parse_mx_reply](https://c-ares.haxx.se/ares_parse_mx_reply.html)
  * [ares_parse_naptr_reply](https://c-ares.haxx.se/ares_parse_naptr_reply.html)
  * [ares_parse_ns_reply](https://c-ares.haxx.se/ares_parse_ns_reply.html)
  * [ares_parse_ptr_reply](https://c-ares.haxx.se/ares_parse_ptr_reply.html)
  * [ares_parse_soa_reply](https://c-ares.haxx.se/ares_parse_soa_reply.html)
  * [ares_parse_srv_reply](https://c-ares.haxx.se/ares_parse_srv_reply.html)
  * [ares_parse_txt_reply](https://c-ares.haxx.se/ares_parse_txt_reply.html)
  * [ares_parse_txt_reply_ext](https://c-ares.haxx.se/ares_parse_txt_reply_ext.html)
  * [ares_process_fd](https://c-ares.haxx.se/ares_process_fd.html)
  * [ares_save_options](https://c-ares.haxx.se/ares_save_options.html)
  * [ares_search](https://c-ares.haxx.se/ares_search.html)
  * [ares_send](https://c-ares.haxx.se/ares_send.html)
  * [ares_set_local_dev](https://c-ares.haxx.se/ares_set_local_dev.html)
  * [ares_set_servers](https://c-ares.haxx.se/ares_set_servers.html)
  * [ares_set_servers_csv](https://c-ares.haxx.se/ares_set_servers_csv.html)
  * [ares_set_servers_ports](https://c-ares.haxx.se/ares_set_servers_ports.html)
  * [ares_set_servers_ports_csv](https://c-ares.haxx.se/ares_set_servers_ports_csv.html)
  * [ares_set_socket_callback](https://c-ares.haxx.se/ares_set_socket_callback.html)
  * [ares_set_socket_configure_callback](https://c-ares.haxx.se/ares_set_socket_configure_callback.html)
  * [ares_set_sortlist](https://c-ares.haxx.se/ares_set_sortlist.html)
  * [ares_timeout](https://c-ares.haxx.se/ares_timeout.html)
  * [ares_version](https://c-ares.haxx.se/ares_version.html)

The following functions are not bound, but don't need a binding because of the nature of Lua:

  * [ares_destroy](https://c-ares.haxx.se/ares_destroy.html)
  * [ares_destroy_options](https://c-ares.haxx.se/ares_destroy_options.html)
  * [ares_strerror](https://c-ares.haxx.se/ares_strerror.html)
  * [ares_free_data](https://c-ares.haxx.se/ares_free_data.html)
  * [ares_free_hostent](https://c-ares.haxx.se/ares_free_hostent.html)
  * [ares_free_string](https://c-ares.haxx.se/ares_free_string.html)
  * [ares_library_cleanup](https://c-ares.haxx.se/ares_library_cleanup.html)
  * [ares_library_init](https://c-ares.haxx.se/ares_library_init.html)
  * [ares_library_init_mem](https://c-ares.haxx.se/ares_library_init_mem.html)
  * [ares_library_initialized](https://c-ares.haxx.se/ares_library_initialized.html)

## No constants table for query statuses

The query statuses provided to the reply handler function used with `resolver:query`
are just the raw number values provided to the handler on the C side - I need to add
a lookup table to be able to handle these values, or perhaps just pass string values
to the reply handler.  I wouldn't rely on the status value until this functionality
gets fleshed out.

## Unsupported options in the init function

Currently only the `servers` option works for init - the groundwork for fleshing out
other options exists, though.  Contributions adding support for new options are most
welcome!

## Parser functions don't return TTL information

`ares.parse_a_reply` and friends should return TTL data, but they currently don't.
Adding it as a second return value won't break existing code.

# License

lua-ares is released under the same license as c-ares and Lua - the MIT/X license,
a copy of which has been included under [COPYING](COPYING).
