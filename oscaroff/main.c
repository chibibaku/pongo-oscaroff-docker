#include <pongo.h>

static void (*old_preboot_hook)(void) = NULL;
static int oscar_patched = 0;

static int
count_token(char *buf, uint32_t len, const char *token)
{
    uint32_t off = 0;
    size_t tlen = strlen(token);
    int count = 0;

    while (off < len) {
        uint32_t slen = 0;

        while ((off + slen) < len && buf[off + slen] != '\0')
            slen++;

        if ((off + slen) >= len)
            return -1;

        if (slen == tlen && memcmp(buf + off, token, tlen) == 0)
            count++;

        off += slen + 1;
    }

    return count;
}

static int
replace_token(char *buf, uint32_t len, const char *from, const char *to)
{
    uint32_t off = 0;
    size_t flen = strlen(from);
    size_t tlen = strlen(to);
    int count = 0;

    if (flen != tlen) {
        puts("[oscaroff] BUG: token lengths differ");
        return -1;
    }

    while (off < len) {
        uint32_t slen = 0;

        while ((off + slen) < len && buf[off + slen] != '\0')
            slen++;

        if ((off + slen) >= len)
            return -1;

        if (slen == flen && memcmp(buf + off, from, flen) == 0) {
            memcpy(buf + off, to, flen);
            count++;
        }

        off += slen + 1;
    }

    return count;
}

static dt_node_t *
find_oscar_node(void)
{
    dt_node_t *node = dt_find(gDeviceTree, "oscar");

    if (node != NULL)
        return node;

    return dt_find(gDeviceTree, "xscar");
}

static void
oscar_status(void)
{
    dt_node_t *node;
    uint32_t len;
    char *name;
    char *dtype;
    char *compat;
    uint32_t off;

    node = find_oscar_node();

    if (node == NULL) {
        puts("[oscaroff] Oscar node not found");
        return;
    }

    name = dt_prop(node, "name", &len);
    if (name != NULL)
        printf("[oscaroff] name        = %s\n", name);
    else
        puts("[oscaroff] name        = <missing>");

    dtype = dt_prop(node, "device_type", &len);
    if (dtype != NULL)
        printf("[oscaroff] device_type = %s\n", dtype);
    else
        puts("[oscaroff] device_type = <missing>");

    compat = dt_prop(node, "compatible", &len);

    if (compat == NULL) {
        puts("[oscaroff] compatible  = <missing>");
        return;
    }

    printf("[oscaroff] compatible (%u bytes):\n", len);

    off = 0;
    while (off < len) {
        uint32_t slen = 0;

        while ((off + slen) < len && compat[off + slen] != '\0')
            slen++;

        if ((off + slen) >= len) {
            puts("  <malformed>");
            break;
        }

        printf("  - %s\n", compat + off);
        off += slen + 1;
    }
}

static int
patch_oscar(void)
{
    dt_node_t *node;
    uint32_t name_len = 0;
    uint32_t dtype_len = 0;
    uint32_t compat_len = 0;
    char *name;
    char *dtype;
    char *compat;
    int c_apple;
    int c_oscar1;
    int c_oscar;

    if (oscar_patched) {
        puts("[oscaroff] already patched");
        return 0;
    }

    node = dt_find(gDeviceTree, "oscar");

    if (node == NULL) {
        node = dt_find(gDeviceTree, "xscar");

        if (node != NULL) {
            oscar_patched = 1;
            puts("[oscaroff] already appears disabled");
            return 0;
        }

        puts("[oscaroff] ERROR: Oscar node not found");
        return -1;
    }

    name = (char *)dt_prop(node, "name", &name_len);
    dtype = (char *)dt_prop(node, "device_type", &dtype_len);
    compat = (char *)dt_prop(node, "compatible", &compat_len);

    if (name == NULL || name_len < 6 || memcmp(name, "oscar", 6) != 0) {
        puts("[oscaroff] ERROR: unexpected name");
        return -1;
    }

    if (dtype == NULL || dtype_len < 6 || memcmp(dtype, "oscar", 6) != 0) {
        puts("[oscaroff] ERROR: unexpected device_type");
        return -1;
    }

    if (compat == NULL) {
        puts("[oscaroff] ERROR: compatible missing");
        return -1;
    }

    c_apple = count_token(compat, compat_len, "apple-oscar");
    c_oscar1 = count_token(compat, compat_len, "oscar1");
    c_oscar = count_token(compat, compat_len, "oscar");

    printf("[oscaroff] compatible check: "
           "apple-oscar=%d oscar1=%d oscar=%d\n",
           c_apple, c_oscar1, c_oscar);

    if (c_apple != 1 || c_oscar1 != 1 || c_oscar != 1) {
        puts("[oscaroff] ERROR: unexpected compatible layout; refusing patch");
        return -1;
    }

    if (replace_token(compat, compat_len, "apple-oscar", "apple-xscar") != 1) {
        puts("[oscaroff] ERROR patching apple-oscar");
        return -1;
    }

    if (replace_token(compat, compat_len, "oscar1", "xscar1") != 1) {
        puts("[oscaroff] ERROR patching oscar1");
        return -1;
    }

    if (replace_token(compat, compat_len, "oscar", "xscar") != 1) {
        puts("[oscaroff] ERROR patching oscar");
        return -1;
    }

    memcpy(name, "xscar", 5);
    memcpy(dtype, "xscar", 5);

    cache_clean_and_invalidate(name, name_len);
    cache_clean_and_invalidate(dtype, dtype_len);
    cache_clean_and_invalidate(compat, compat_len);

    oscar_patched = 1;

    puts("[oscaroff] ==================================");
    puts("[oscaroff] Oscar/M8 DeviceTree node disabled");
    puts("[oscaroff] ==================================");

    return 0;
}

static void
cmd_oscarstatus(const char *cmd, char *args)
{
    (void)cmd;
    (void)args;
    oscar_status();
}

static void
cmd_oscaroff(const char *cmd, char *args)
{
    (void)cmd;
    (void)args;
    patch_oscar();
    oscar_status();
}

static void
oscar_preboot_hook(void)
{
    puts("[oscaroff] preboot hook");
    patch_oscar();

    if (old_preboot_hook != NULL)
        old_preboot_hook();
}

void
module_entry(void)
{
    puts("[oscaroff] loading module");

    old_preboot_hook = preboot_hook;
    preboot_hook = oscar_preboot_hook;

    command_register(
        "oscarstatus",
        "Show Oscar DeviceTree status",
        cmd_oscarstatus
    );

    command_register(
        "oscaroff",
        "Disable Oscar/M8 in DeviceTree",
        cmd_oscaroff
    );

    puts("[oscaroff] commands registered:");
    puts("  oscarstatus");
    puts("  oscaroff");
}

char *module_name = "oscaroff";

struct pongo_exports exported_symbols[] = {
    {
        .name = 0,
        .value = 0
    }
};
