#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <ares.h>
#include <ctype.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <netdb.h>
#include <string.h>

#include <errno.h>

#define ARES_CHANNEL_METATABLE "ares_channel"

static ares_channel *
push_ares_channel(lua_State *L)
{
    ares_channel *channel;
    channel = lua_newuserdata(L, sizeof(ares_channel));
    luaL_getmetatable(L, ARES_CHANNEL_METATABLE);
    lua_setmetatable(L, -2);

    return channel;
}

// table with server names is on top of the stack
static int
option_add_servers(lua_State *L, struct ares_options *options)
{
    int i;
    int status;
    struct in_addr *servers;
    int nservers;

    nservers = lua_objlen(L, -1);
    servers = lua_newuserdata(L, sizeof(struct in_addr) * nservers); // XXX how to make sure this gets preserved?

    options->nservers = nservers;
    options->servers  = servers;

    for(i = 1; i <= nservers; i++) {
        lua_rawgeti(L, -2, i);
        status = inet_pton(AF_INET, luaL_checkstring(L, -1), servers + (i - 1));
        if(!status) {
            return 0; // XXX more information?
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1); // pop userdata
    return 1;
}

// XXX The optmask parameter also includes options without a corresponding field in the ares_options structure, as follows: 
static int
setup_options(lua_State *L, struct ares_options *options, int *optmask)
{
    *optmask = 0;

#define HANDLE_OPTION(optname, block)\
    else if(0 == strcmp(optname, lua_tostring(L, -2))) block
#define NYI() luaL_error(L, "unimplemented init option %s", lua_tostring(L, -2))
    lua_pushnil(L);
    while(lua_next(L, 1)) {
        if(0) { // dummy that I need for HANDLE_OPTION
        }
        HANDLE_OPTION("flags", {
            *optmask |= ARES_OPT_FLAGS;
            NYI();
        })
        HANDLE_OPTION("timeout", {
            *optmask |= ARES_OPT_TIMEOUT;
            NYI();
        })
        HANDLE_OPTION("tries", {
            *optmask |= ARES_OPT_TRIES;
            NYI();
        })
        HANDLE_OPTION("ndots", {
            *optmask |= ARES_OPT_NDOTS;
            NYI();
        })
        HANDLE_OPTION("udp_port", {
            *optmask |= ARES_OPT_UDP_PORT;
            NYI();
        })
        HANDLE_OPTION("tcp_port", {
            *optmask |= ARES_OPT_TCP_PORT;
            NYI();
        })
        HANDLE_OPTION("socket_send_buffer_size", {
            *optmask |= ARES_OPT_SOCK_SNDBUF;
            NYI();
        })
        HANDLE_OPTION("socket_receive_buffer_size", {
            *optmask |= ARES_OPT_SOCK_RCVBUF;
            NYI();
        })
        HANDLE_OPTION("servers", {
            int status;
            *optmask |= ARES_OPT_SERVERS;
            status = option_add_servers(L, options);
            if(!status) {
                lua_pushnil(L);
                lua_pushliteral(L, "unable to parse address");
                return 0;
            }
        })
        HANDLE_OPTION("domains", {
            *optmask |= ARES_OPT_DOMAINS;
            NYI();
        })
        HANDLE_OPTION("lookups", {
            *optmask |= ARES_OPT_LOOKUPS;
            NYI();
        })
        HANDLE_OPTION("sock_state_cb", {
            *optmask |= ARES_OPT_SOCK_STATE_CB;
            NYI();
        })
        HANDLE_OPTION("sortlist", {
            *optmask |= ARES_OPT_SORTLIST;
            NYI();
        })
        HANDLE_OPTION("ednspsz", {
            *optmask |= ARES_OPT_EDNSPSZ;
            NYI();
        })

        else { // wrap up the else ifs generated by HANDLE_OPTION
            luaL_error(L, "unknown option %s", lua_tostring(L, -2));
        }

        lua_pop(L, 1);
    }
#undef HANDLE_OPTION
    return 0;
}

static int
handle_error(lua_State *L, int status)
{
    lua_pushnil(L);
    lua_pushstring(L, ares_strerror(status));
    return 2;
}

static int
lua_ares_init(lua_State *L)
{
    ares_channel *channel;
    int status;

    channel = push_ares_channel(L);

    if(lua_type(L, 1) == LUA_TTABLE) {
        struct ares_options options;
        int optmask;
        // XXX handle error
        setup_options(L, &options, &optmask);
        status = ares_init_options(channel, &options, optmask);
    } else {
        status = ares_init(channel);
    }
    if(status == ARES_SUCCESS) {
        return 1;
    } else {
        return handle_error(L, status);
    }
}

static void
push_ipv4_address(lua_State *L, char *rawaddr)
{
    struct in_addr addr;
    const char *dotted_numbers;

    memcpy(&addr, rawaddr, 4);
    dotted_numbers = inet_ntoa(addr);
    lua_pushstring(L, dotted_numbers);
}

static void
push_ipv6_address(lua_State *L, char *rawaddr)
{
    struct in6_addr addr;
    char coloned_numbers[INET6_ADDRSTRLEN];

    memcpy(&addr, rawaddr, 16);
    memset(coloned_numbers, 0, 16);
    if(!inet_ntop(AF_INET6, &addr, coloned_numbers, INET6_ADDRSTRLEN)) {
        // XXX handle error
    }
    lua_pushstring(L, coloned_numbers);
}

static void
push_hostent(lua_State *L, struct hostent *host)
{
    int i;
    lua_newtable(L);

    // h_name
    lua_pushstring(L, host->h_name);
    lua_setfield(L, -2, "name");

    // h_aliases
    lua_newtable(L);
    for(i = 0; host->h_aliases[i] != NULL; i++) {
        lua_pushstring(L, host->h_aliases[i]);
        lua_rawseti(L, -2, lua_objlen(L, -2) + 1);
    }
    lua_setfield(L, -2, "aliases");

    // h_addrtype
    lua_pushstring(L, host->h_addrtype == AF_INET ? "inet" : "inet6");
    lua_setfield(L, -2, "addrtype");

    // h_addr_list
    lua_newtable(L);
    for(i = 0; host->h_addr_list[i]; i++) {
        if(host->h_addrtype == AF_INET) {
            push_ipv4_address(L, host->h_addr_list[i]);
        } else { // AF_INET6
            push_ipv6_address(L, host->h_addr_list[i]);
        }
        lua_rawseti(L, -2, lua_objlen(L, -2) + 1);
    }
    lua_setfield(L, -2, "addr_list");
}

static int
lua_ares_parse_a_reply(lua_State *L)
{
    size_t length;
    struct hostent *host;
    int status;
    const unsigned char *buf = (const unsigned char *) luaL_checklstring(L, 1, &length);

    // XXX handle addrttls too
    status = ares_parse_a_reply(buf, length, &host, NULL, NULL);
    if(status != ARES_SUCCESS) {
        return handle_error(L, status);
    }
    push_hostent(L, host);
    ares_free_hostent(host);
    return 1;
}

static int
lua_ares_parse_aaaa_reply(lua_State *L)
{
    size_t length;
    struct hostent *host;
    int status;
    const unsigned char *buf = (const unsigned char *) luaL_checklstring(L, 1, &length);

    // XXX handle addrttls too
    status = ares_parse_aaaa_reply(buf, length, &host, NULL, NULL);
    if(status != ARES_SUCCESS) {
        return handle_error(L, status);
    }
    push_hostent(L, host);
    ares_free_hostent(host);
    return 1;
}

luaL_Reg ares_functions[] = {
    { "init", lua_ares_init },
    { "parse_a_reply", lua_ares_parse_a_reply },
    { "parse_aaaa_reply", lua_ares_parse_aaaa_reply },
    { NULL, NULL }
};

struct callback_data {
    lua_State *L;
    int callback_ref;
    int self_ref; // XXX should I track which queries are active for a channel instead?
};

// XXX perform callback cleanup in here?
static void
lua_ares_channel_query_callback(void *arg, int status, int timeouts, unsigned char *abuf, int alen)
{
    struct callback_data *callback_data = (struct callback_data *) arg;
    lua_State *L = callback_data->L;

    lua_rawgeti(L, LUA_REGISTRYINDEX, callback_data->callback_ref);
    lua_pushinteger(L, status);
    lua_pushinteger(L, timeouts);
    lua_pushlstring(L, (char *) abuf, alen);
    lua_pcall(L, 3, 0, 0); // XXX handle error?
}

static int
lua_ares_channel_query(lua_State *L)
{
    ares_channel *c = luaL_checkudata(L, 1, ARES_CHANNEL_METATABLE);
    const char *name = luaL_checkstring(L, 2);
    int _class = luaL_checkinteger(L, 3);
    int type = luaL_checkinteger(L, 4);
    int callback;
    struct callback_data *callback_data;

    lua_settop(L, 5);
    callback = luaL_ref(L, LUA_REGISTRYINDEX); // XXX how/when do I clean this up?
    callback_data = lua_newuserdata(L, sizeof(struct callback_data));
    callback_data->L = L; // XXX get global state?
    callback_data->callback_ref = callback;
    callback_data->self_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    ares_query(*c, name, _class, type, lua_ares_channel_query_callback, callback_data);
    return 0;
}

static void
fd_set_to_table(lua_State *L, fd_set *fds, int max)
{
    int i;
    lua_newtable(L);

    for(i = 0; i < max; i++) {
        if(FD_ISSET(i, fds)) {
            lua_pushinteger(L, i);
            lua_rawseti(L, -2, lua_objlen(L, -2) + 1);
        }
    }
}

static void
table_to_fd_set(lua_State *L, int index, fd_set *fds)
{
    int i;
    FD_ZERO(fds);
    for(i = 1; i <= lua_objlen(L, index); i++) {
        lua_rawgeti(L, index, i);
        FD_SET(lua_tointeger(L, -1), fds);
        lua_pop(L, 1);
    }
}

static int
lua_ares_channel_fds(lua_State *L)
{
    fd_set read;
    fd_set write;
    int nfds;
    ares_channel *c;

    c = luaL_checkudata(L, 1, ARES_CHANNEL_METATABLE);

    FD_ZERO(&read);
    FD_ZERO(&write);

    nfds = ares_fds(*c, &read, &write);

    fd_set_to_table(L, &read, nfds);
    fd_set_to_table(L, &write, nfds);
    lua_pushinteger(L, nfds);
    return 3;
}

static int
lua_ares_channel_process(lua_State *L)
{
    ares_channel *c;
    fd_set read_fds;
    fd_set write_fds;

    c = luaL_checkudata(L, 1, ARES_CHANNEL_METATABLE);
    table_to_fd_set(L, 2, &read_fds);
    table_to_fd_set(L, 3, &write_fds);

    ares_process(*c, &read_fds, &write_fds);
    return 0;
}

luaL_Reg ares_channel_methods[] = {
    { "query", lua_ares_channel_query },
    { "fds", lua_ares_channel_fds },
    { "process", lua_ares_channel_process },
    { NULL, NULL }
};

static void
_add_constant(lua_State *L, const char *prefix, const char fullname[16], int value)
{
    char name_copy[16]; // should be longer than any of the names we provide
    int i;
    strncpy(name_copy, fullname + strlen(prefix), sizeof(name_copy));
    for(i = 0; i < strlen(name_copy); i++) {
        name_copy[i] = toupper(name_copy[i]);
    }
    lua_pushstring(L, name_copy);
    lua_pushinteger(L, value);
    lua_settable(L, -3);
}

int
luaopen_ares(lua_State *L)
{
    // initialize c-ares library - sorry, this is going to leak =(
    ares_library_init(ARES_LIB_INIT_ALL);

    // set up metatables

    // channels
    luaL_newmetatable(L, ARES_CHANNEL_METATABLE);
    lua_newtable(L);
    luaL_register(L, NULL, ares_channel_methods);
    lua_setfield(L, -2, "__index");

    // set up exported functions
    lua_newtable(L);
    luaL_register(L, NULL, ares_functions);

    // set up constants

#define ADD_CONSTANT(prefix, symbol) _add_constant(L, #prefix, #symbol, symbol)
    // classes
    lua_newtable(L);
    ADD_CONSTANT(ns_c_, ns_c_invalid);
    ADD_CONSTANT(ns_c_, ns_c_in);
    ADD_CONSTANT(ns_c_, ns_c_2);
    ADD_CONSTANT(ns_c_, ns_c_chaos);
    ADD_CONSTANT(ns_c_, ns_c_hs);
    ADD_CONSTANT(ns_c_, ns_c_none);
    ADD_CONSTANT(ns_c_, ns_c_any);
    ADD_CONSTANT(ns_c_, ns_c_max);
    lua_setfield(L, -2, "class");

    // types
    lua_newtable(L);
    ADD_CONSTANT(ns_t_, ns_t_invalid);
    ADD_CONSTANT(ns_t_, ns_t_a);
    ADD_CONSTANT(ns_t_, ns_t_ns);
    ADD_CONSTANT(ns_t_, ns_t_md);
    ADD_CONSTANT(ns_t_, ns_t_mf);
    ADD_CONSTANT(ns_t_, ns_t_cname);
    ADD_CONSTANT(ns_t_, ns_t_soa);
    ADD_CONSTANT(ns_t_, ns_t_mb);
    ADD_CONSTANT(ns_t_, ns_t_mg);
    ADD_CONSTANT(ns_t_, ns_t_mr);
    ADD_CONSTANT(ns_t_, ns_t_null);
    ADD_CONSTANT(ns_t_, ns_t_wks);
    ADD_CONSTANT(ns_t_, ns_t_ptr);
    ADD_CONSTANT(ns_t_, ns_t_hinfo);
    ADD_CONSTANT(ns_t_, ns_t_minfo);
    ADD_CONSTANT(ns_t_, ns_t_mx);
    ADD_CONSTANT(ns_t_, ns_t_txt);
    ADD_CONSTANT(ns_t_, ns_t_rp);
    ADD_CONSTANT(ns_t_, ns_t_afsdb);
    ADD_CONSTANT(ns_t_, ns_t_x25);
    ADD_CONSTANT(ns_t_, ns_t_isdn);
    ADD_CONSTANT(ns_t_, ns_t_rt);
    ADD_CONSTANT(ns_t_, ns_t_nsap);
    ADD_CONSTANT(ns_t_, ns_t_nsap_ptr);
    ADD_CONSTANT(ns_t_, ns_t_sig);
    ADD_CONSTANT(ns_t_, ns_t_key);
    ADD_CONSTANT(ns_t_, ns_t_px);
    ADD_CONSTANT(ns_t_, ns_t_gpos);
    ADD_CONSTANT(ns_t_, ns_t_aaaa);
    ADD_CONSTANT(ns_t_, ns_t_loc);
    ADD_CONSTANT(ns_t_, ns_t_nxt);
    ADD_CONSTANT(ns_t_, ns_t_eid);
    ADD_CONSTANT(ns_t_, ns_t_nimloc);
    ADD_CONSTANT(ns_t_, ns_t_srv);
    ADD_CONSTANT(ns_t_, ns_t_atma);
    ADD_CONSTANT(ns_t_, ns_t_naptr);
    ADD_CONSTANT(ns_t_, ns_t_kx);
    ADD_CONSTANT(ns_t_, ns_t_cert);
    ADD_CONSTANT(ns_t_, ns_t_a6);
    ADD_CONSTANT(ns_t_, ns_t_dname);
    ADD_CONSTANT(ns_t_, ns_t_sink);
    ADD_CONSTANT(ns_t_, ns_t_opt);
    ADD_CONSTANT(ns_t_, ns_t_apl);
    ADD_CONSTANT(ns_t_, ns_t_tkey);
    ADD_CONSTANT(ns_t_, ns_t_tsig);
    ADD_CONSTANT(ns_t_, ns_t_ixfr);
    ADD_CONSTANT(ns_t_, ns_t_axfr);
    ADD_CONSTANT(ns_t_, ns_t_mailb);
    ADD_CONSTANT(ns_t_, ns_t_maila);
    ADD_CONSTANT(ns_t_, ns_t_any);
    ADD_CONSTANT(ns_t_, ns_t_zxfr);
    ADD_CONSTANT(ns_t_, ns_t_max);
    lua_setfield(L, -2, "type");
#undef ADD_CONSTANT

    return 1;
}
